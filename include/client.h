#ifndef CLIENT_INCLUDED_H
#define CLIENT_INCLUDED_H

#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>

#include "include/orders.h"

/**
 * @brief The class represents a TCP client connection with a TCP server.
 * The client can send orders to the server in a binary format and read
 * responses from the server. The client is configurable to connect to a
 * supplied host on a supplied port.
 */
class TCPClient {
  enum { max_size = 1024 };
  using ClientBuf = std::array<char, max_size>;

  int m_sockfd;
  ClientBuf m_buffer;

public:
  /**
   * @brief Create TCP client which will connect to a host on a given port.
   * @param host - the name of the server
   * @param port - the port on which we want to connect to
   */
  TCPClient(std::string const &host, std::string const &port);

  // cannot copy construct or assign a client due to socket ownership
  TCPClient(TCPClient const &) = delete;
  TCPClient &operator=(TCPClient const &) = delete;
  ~TCPClient();

  void run();

  /**
   * @brief Send a message to the server. The message will contain a header
   * and a payload packet of some sort.
   */
  template <Sendable T> void sendData(Message<T> &message);
};

#endif
