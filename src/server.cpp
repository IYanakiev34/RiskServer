#include "include/server.h"
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <unistd.h>

#include <util/util.h>

Server::Server(std::string &&host, std::string &&port)
    : m_requestBuffer(), m_clientName(), m_toDel(), m_currDelIdx(0),
      m_clientAddr(), m_sinSize(0), m_fds(), m_host(host), m_port(port),
      m_listenerfd(INVALID_FD) {
  m_fds.reserve(BACK_LOG);

  std::optional<int> listener_opt = get_listener_fd();
  if (!listener_opt.has_value()) {
    exit(1);
  }
  m_listenerfd = listener_opt.value();
}

Server::~Server() {
  close(m_listenerfd);
  for (auto &fd : m_fds) {
    close(fd.fd);
  }
}

void Server::listen() {
  if (::listen(m_listenerfd, BACK_LOG) == -1) {
    std::perror("server listen: ");
    close(m_listenerfd);
    exit(1);
  }

  pollfd listfd;
  listfd.fd = m_listenerfd;
  listfd.events = POLLIN;
  m_fds.emplace_back(std::move(listfd)); // add listener to poll set
  std::cout << "Server started listening on port: " << m_port << "\n";
}

void Server::run() {
  while (true) {
    int poll_num = poll(m_fds.data(), m_fds.size(), -1);
    if (poll_num == -1) {
      std::perror("poll");
      exit(1);
    }

    std::cout << "Request Nums: " << poll_num << std::endl;
    m_currDelIdx = 0;

    for (auto it = m_fds.begin(); it != m_fds.end(); ++it) {
      if ((*it).revents & POLLIN) {
        if ((*it).fd == m_listenerfd) {
          handle_new_connection();
        } else {
          // Read from client
          handle_client_request(it);
        }
      }
    }

    // Remove dead connections
    for (int i = 0; i < m_currDelIdx; ++i) {
      m_fds.erase(m_fds.begin() + m_toDel[i]);
    }
  }
}

template <typename It> void Server::handle_client_request(It it) {
  int client_fd = (*it).fd;
  ssize_t nbytes =
      recv(client_fd, m_requestBuffer.data(), m_requestBuffer.size(), 0);
  std::cout << "Server got: " << nbytes << " bytes\n\n";

  if (nbytes <= 0) {
    if (nbytes == 0) {
      std::cout << "pollserver: socket " << client_fd << " hung up"
                << std::endl;
      shutdown(client_fd, SHUT_RDWR);
    } else {
      std::perror("recv:");
    }
    close(client_fd);
    m_toDel[m_currDelIdx++] = std::distance(m_fds.begin(), it);
    return;
  }

  // Create a message from the bytes received
  switch (nbytes) {
  case NEWO_MSG_SIZE: {
    Message<NewOrder> msg;
    std::memset(&msg, 0, sizeof(Message<NewOrder>));
    std::memcpy(&msg, m_requestBuffer.data(), nbytes);
    deserialize(msg);
    std::cout << msg.header << std::endl;
    std::cout << msg.data << std::endl;
    break;
  }
  case DELO_MSG_SIZE: {
    Message<DeleteOrder> msg;
    std::memset(&msg, 0, sizeof(Message<DeleteOrder>));
    std::memcpy(&msg, m_requestBuffer.data(), nbytes);
    deserialize(msg);
    std::cout << msg.header << std::endl;
    std::cout << msg.data << std::endl;
    break;
  }
  case MODO_MSG_SIZE: {
    Message<ModifyOrderQuantity> msg;
    std::memset(&msg, 0, sizeof(Message<ModifyOrderQuantity>));
    std::memcpy(&msg, m_requestBuffer.data(), nbytes);
    deserialize(msg);
    std::cout << msg.header << std::endl;
    std::cout << msg.data << std::endl;
    break;
  }
  case TRO_MSG_SIZE: {
    Message<Trade> msg;
    std::memset(&msg, 0, sizeof(Message<Trade>));
    std::memcpy(&msg, m_requestBuffer.data(), nbytes);
    deserialize(msg);
    std::cout << msg.header << std::endl;
    std::cout << msg.data << std::endl;
    break;
  }
  default:
    std::cerr << "Cannot handle this message!\n";
    exit(1);
  };

  Message<OrderResponse> rsp{.header{.version = 1,
                                     .payloadSize = sizeof(OrderResponse),
                                     .sequenceNumber = 2,
                                     .timestamp = 10},
                             .data{.messageType = OrderResponse::MESSAGE_TYPE,
                                   .orderId = 10,
                                   .status = OrderResponse::Status::ACCEPTED}};

  serialize(rsp);
  size_t toSend = ORDR_MSG_SIZE;
  std::memcpy(m_requestBuffer.data(), &rsp, toSend);

  int actuallySent = send(client_fd, &rsp, toSend, 0);
  if (actuallySent == -1) {
    std::cerr << "Some err\n";
  }
}

void Server::handle_new_connection() {
  m_sinSize = sizeof(struct sockaddr_storage);
  int new_fd = accept(m_listenerfd, reinterpret_cast<sockaddr *>(&m_clientAddr),
                      &m_sinSize);
  if (new_fd == -1) {
    std::perror("server accept:");
    return;
  }

  pollfd conn_fd;
  conn_fd.fd = new_fd;
  conn_fd.events = POLLIN;
  m_fds.emplace_back(std::move(conn_fd));

  inet_ntop(m_clientAddr.ss_family,
            get_addr_in(reinterpret_cast<struct sockaddr *>(&m_clientAddr)),
            m_clientName.data(), INET6_ADDRSTRLEN);
  std::cout << "Server got connection from: " << m_clientName.data()
            << std::endl;
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

  if ((rv = getaddrinfo(NULL, m_port.data(), &hints, &out)) != 0) {
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
