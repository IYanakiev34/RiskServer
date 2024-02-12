#ifndef SERVER_INCLUDED_H
#define SERVER_INCLUDED_H

#include <arpa/inet.h>
#include <array>
#include <optional>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

static constexpr size_t BACK_LOG = 20;
static constexpr size_t HEADER_SIZE = 16;

/**
 * @brief A Server class that represents a set of client connections that are
 * connected on a TCP/IP socket. The server will handle client requests and
 * respond to them on the proper clients.
 */
class Server final {
  static constexpr int INVALID_FD = -1000;

  // Convinience types for internal use
  using VecFds = std::vector<pollfd>;
  using NameBuf = std::array<char, INET6_ADDRSTRLEN>;
  enum { max_size = 2048 };
  using ReqBuf = std::array<char, max_size>;

  ReqBuf m_requestBuffer; // Holds the client request and the serve response
  NameBuf m_clientName;   // stores the hostname of the client
  std::array<int, 256> m_toDel; // connections to delete;
  int m_currDelIdx;             // the current index in the m_toDel arr
  struct sockaddr_storage
      m_clientAddr;    // stores the sockaddr_in or sockaddr_in6 of the client
  socklen_t m_sinSize; // stores the size of the sockaddr struct

  VecFds m_fds;       // vector that stores the connections to the server
  std::string m_host; // the name of the host
  std::string m_port; // the port of the host
  int m_listenerfd;   // the listenerfd used for accepting client connections

public:
  /**
   * @brief The function will create a TCP server on a given host and port. The
   * server will store internal information about the host and will setup the
   * infrastructure for accepting connections. However, it will NOT start
   * listening immediately.
   * @param host - the name of the host
   * @param port - the port number
   */
  Server(std::string &&host, std::string &&port);

  // cannot copy construct of assign a server
  Server(Server const &) = delete;
  Server &operator=(Server const &) = delete;

  // Close all connections and the accepting socket
  ~Server();

  /**
   * @brief Start listening for client connections in the listener_fd socket. If
   * there is an error the method will log it and exit the application.
   * Otherwise it will print that the server has started listening for
   * connections on port <port> and add the listener_fd to the set of fds.
   */
  void listen();

  /**
   * @brief Starts running the server and polling for connections and reads from
   * the clients. The server will handle new clients by adding their fds to the
   * vector or server existing ones when calling the poll method.
   */
  void run();

private:
  /**
   * @brief Handle client requests and messages on a specific client socket.
   */
  template <typename It> void handle_client_request(It it);

  /**
   * @brief Handle new incoming connection. The new connection will be included
   * in the set of fds.
   */
  void handle_new_connection();

  /**
   * @brief It will create a new socket try to bind it and then return it to the
   * user. This socket is the listener socket. If we encounter critical error we
   * will return a nullopt.
   * @return the listener fd if exists or nullopt
   */
  std::optional<int> get_listener_fd();
};

#endif