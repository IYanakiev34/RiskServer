#ifndef SERVER_INCLUDED_H
#define SERVER_INCLUDED_H

#include "server_util.h"
#include <arpa/inet.h>
#include <array>
#include <memory>
#include <optional>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

class Connection;

/**
 * @brief Holds the main server resources. These resources should be shared by
 * connections. Connections should perform operation on the product map when the
 * recieve a valid order.
 */
struct ServerResources {
  ServerResources()
      : ListenerFd(INVALID_FD), Fds(), Connections(), ProductMap() {}
  int ListenerFd{INVALID_FD};
  std::vector<pollfd> Fds; /// Vector of the active file descriptors
  std::unordered_map<int, std::shared_ptr<Connection>>
      Connections; /// Map of the connections
  std::unordered_map<uint64_t, ProductInfo>
      ProductMap; // Map of the products and their total positions
};

/**
 * @brief A Server class that represents a set of client connections that are
 * connected on a TCP/IP socket. The server will handle client requests and
 * respond to them on the proper clients.
 */
class Server final {
  using NameBuf = std::array<char, INET6_ADDRSTRLEN>;

  NameBuf m_clientName; // stores the hostname of the client
  ServerResources m_resources;
  ServerInfo m_info;
  struct sockaddr_storage
      m_clientAddr;    // stores the sockaddr_in or sockaddr_in6 of the client
  socklen_t m_sinSize; // stores the size of the sockaddr struct

public:
  /**
   * @brief The function will create a TCP server on a given host and port. The
   * server will store internal information about the host and will setup the
   * infrastructure for accepting connections. However, it will NOT start
   * listening immediately.
   * @param host - the name of the host
   * @param port - the port number
   */
  Server(std::string host, std::string port, ServerConfig config);

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

  /**
   * @brief Deregister a connection from the server. The connection should be
   * removed from the map of connections and we should also remove the pollfd
   * associated with the connection.
   */
  void deregister_connection(std::shared_ptr<Connection> conn);

  /// Reutrn a reference to the server products should be used only by
  /// connections
  [[nodiscard]] inline std::unordered_map<uint64_t, ProductInfo> &
  get_products() noexcept {
    return m_resources.ProductMap;
  }

  /// Return a reference to the server information should be used only by
  /// connections
  [[nodiscard]] inline ServerInfo &get_info() noexcept { return m_info; }

  /**
   * @brief Print the state of the risk server. This involves printing how many
   * assets we have and what are the current limits and positions for them.
   */
  void print_system_state();

private:
  /**
   * @brief Accept an incoming connection and return the new file descriptor.
   * If an error occurs print it and return invalid FD.
   * @return new fd for the connection or INVALID_FD
   */
  int accept_connection();

  /**
   * @brief Print information about the incoming client that has connected to
   * the server.
   */
  void print_new_connection();

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
