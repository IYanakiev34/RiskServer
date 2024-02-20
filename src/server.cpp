#include "include/server.h"
#include "include/connection.h"
#include "include/server_util.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <unistd.h>

#include <util/util.h>

Server::Server(std::string host, std::string port, ServerConfig info)
    : m_clientName(), m_resources(), m_info(), m_clientAddr(), m_sinSize() {

  m_info.Host = std::move(host);
  m_info.Port = std::move(port);
  m_info.BuyLimit = std::move(info.BuyLimit);
  m_info.SellLimit = std::move(info.SellLimit);

  std::optional<int> listener_opt = get_listener_fd();
  if (!listener_opt.has_value()) {
    exit(1);
  }
  m_resources.ListenerFd = listener_opt.value();
}

Server::~Server() {}

void Server::listen() {
  if (::listen(m_resources.ListenerFd, BACK_LOG) == -1) {
    std::perror("server listen: ");
    close(m_resources.ListenerFd);
    exit(1);
  }

  pollfd listfd;
  listfd.fd = m_resources.ListenerFd;
  listfd.events = POLLIN;
  m_resources.Fds.emplace_back(std::move(listfd)); // add listener to poll set
  std::cout << "Server started listening on port: " << m_info.Port << "\n";
}

void Server::run() {
  while (true) {
    int poll_num = poll(m_resources.Fds.data(), m_resources.Fds.size(), -1);
    if (poll_num == -1) {
      std::perror("poll");
      exit(1);
    }

    std::cout << "Request Nums: " << poll_num << std::endl;

    std::cout << "Fds size: " << m_resources.Fds.size() << std::endl;
    for (auto const &fd : m_resources.Fds) {
      std::cout << "it->fd: " << fd.fd << std::endl;
      if (fd.revents & POLLIN) {
        if (fd.fd == m_resources.ListenerFd) {
          handle_new_connection();
        } else {
          std::cout << "New request\n";
          std::cout << "conn fd: " << fd.fd << std::endl;
          m_resources.Connections[fd.fd]->handle_client_request();
        }
      }
    }
  }
}

int Server::accept_connection() {
  m_sinSize = sizeof(struct sockaddr_storage);
  int new_fd = accept(m_resources.ListenerFd,
                      reinterpret_cast<sockaddr *>(&m_clientAddr), &m_sinSize);
  if (new_fd == -1) {
    std::perror("server accept:");
    return INVALID_FD;
  }
  return new_fd;
}

void Server::print_new_connection() {
  inet_ntop(m_clientAddr.ss_family,
            get_addr_in(reinterpret_cast<struct sockaddr *>(&m_clientAddr)),
            m_clientName.data(), INET6_ADDRSTRLEN);
  std::cout << "Server got connection from: " << m_clientName.data()
            << std::endl;
}

void Server::handle_new_connection() {
  int new_fd = accept_connection(); // get fd for connection
  if (new_fd == INVALID_FD) {
    return;
  }

  pollfd conn_fd;
  conn_fd.fd = new_fd;
  conn_fd.events = POLLIN;
  m_resources.Fds.push_back(conn_fd);

  m_resources.Connections.insert_or_assign(
      new_fd, std::make_shared<Connection>(new_fd, this));
  print_new_connection();
}

void Server::deregister_connection(std::shared_ptr<Connection> conn) {
  int socket = conn->get_socket();
  auto pos =
      std::find_if(m_resources.Fds.begin(), m_resources.Fds.end(),
                   [socket](pollfd const &pfd) { return socket == pfd.fd; });
  if (pos == m_resources.Fds.end()) {
    std::cerr << "No such connection in the server\n";
    return; // No such connection exists
  }

  m_resources.Connections.erase(socket); // remove connection for this socket
  m_resources.Fds.erase(pos); // remove the socket from the pollfd list
}

void Server::print_system_state() {
  for (auto const &[k, v] : m_resources.ProductMap) {
    std::cout << "\nProductId: " << k << std::endl;
    std::cout << v << std::endl;
  }
}

std::optional<int> Server::get_listener_fd() {
  int listenerfd;
  struct addrinfo hints, *out, *it;
  std::memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  int rv;
  int yes = 1;

  if ((rv = getaddrinfo(NULL, m_info.Port.data(), &hints, &out)) != 0) {
    std::cerr << "Server getaddrinfo error: " << gai_strerror(rv) << std::endl;
    return std::nullopt;
  }

  // iterate over possible connections
  for (it = out; it != NULL; it = it->ai_next) {
    if ((listenerfd =
             socket(it->ai_family, it->ai_socktype, it->ai_protocol)) == -1) {
      std::perror("server socket: ");
      continue;
    }

    if (setsockopt(listenerfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      std::perror("server setsockopt: ");
      ::close(listenerfd);
      continue;
    }

    if (::bind(listenerfd, it->ai_addr, it->ai_addrlen) == -1) {
      std::perror("server bind: ");
      close(listenerfd);
      continue;
    }

    break; // successfully binded the socket
  }

  freeaddrinfo(out);
  if (it == NULL) {
    std::cerr << "Could not bind to any IP endpoint\n";
    return std::nullopt;
  }

  return std::optional<int>(listenerfd);
}
