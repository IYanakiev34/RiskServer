#include "include/client.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <util/util.h>

TCPClient::TCPClient(std::string const &host, std::string const &port)
    : m_sockfd(0), m_buffer() {
  struct addrinfo hints, *out, *ptr;
  std::memset(&hints, 0, sizeof(struct addrinfo));
  int rv;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (rv = getaddrinfo(host.data(), port.data(), &hints, &out); rv != 0) {
    std::cerr << "client getaddrinfo error: " << gai_strerror(rv) << std::endl;
    exit(1);
  }

  for (ptr = out; ptr != NULL;
       ptr = ptr->ai_next) { // find proper connect point
    if ((m_sockfd = socket(ptr->ai_family, ptr->ai_socktype,
                           ptr->ai_protocol)) == -1) {
      std::perror("clien socket: ");
      continue;
    }

    if (connect(m_sockfd, ptr->ai_addr, ptr->ai_addrlen) ==
        -1) { // try to connect
      std::perror("client connect: ");
      shutdown(m_sockfd, SHUT_RDWR);
      close(m_sockfd);
      continue;
    }

    break;
  }

  if (ptr == nullptr) {
    std::cerr << "Client: failed to connect to " << host << " on port: " << port
              << "\n";
    exit(1);
  }

  std::array<char, INET6_ADDRSTRLEN> servName;
  inet_ntop(ptr->ai_family, get_addr_in(ptr->ai_addr), servName.data(),
            servName.size());
  std::cout << "Client: connected to " << servName.data() << "\n";
  freeaddrinfo(out);
}

TCPClient::~TCPClient() {
  shutdown(m_sockfd, SHUT_RDWR);
  close(m_sockfd);
}
