#ifndef SERVER_INCLUDED_H
#define SERVER_INCLUDED_H

#include "orders.h"
#include <arpa/inet.h>
#include <array>
#include <optional>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

struct Order {
  int m_traderFd;
  uint64_t m_id;
  uint64_t m_productId;
  uint64_t m_quantity;
  double m_price;
  char m_side;
};

/**
 * TODO: Should make proper abstract over a connection. The connection should
 * handle the requests. The requests should be handled by a requestHandler
 * object. Each requests can be encapsulated into a command. The server should
 * only be concenred with the connections and perhaps shared resources by
 * traders. Create proper abstractions for products info and orders.
 */

static constexpr size_t BACK_LOG = 20;

/**
 * @brief A Server class that represents a set of client connections that are
 * connected on a TCP/IP socket. The server will handle client requests and
 * respond to them on the proper clients.
 */
class Server final {
  static constexpr int INVALID_FD = -1000;

  struct ProductInfo {
    uint64_t NetPos{0};
    uint64_t BuyQty{0};
    uint64_t SellQty{0};
    uint64_t MBuy{0};
    uint64_t MSell{0};

    friend std::ostream &operator<<(std::ostream &out, ProductInfo const &pr) {
      out << "NetPos: " << pr.NetPos << ", BuyQty: " << pr.BuyQty
          << ", SellQty: " << pr.SellQty << ", MaxBuy: " << pr.MBuy
          << ", MSell: " << pr.MSell;
      return out;
    }
  };

  struct Thresholds {
    uint64_t buy;
    uint64_t sell;
  };

  // Convinience types for internal use
  using VecFds = std::vector<pollfd>;
  using NameBuf = std::array<char, INET6_ADDRSTRLEN>;
  enum { max_size = 2048 };
  using ReqBuf = std::array<char, max_size>;
  using TraderMap =
      std::unordered_map<int, std::unordered_map<uint64_t, Order>>;
  using ProductMap = std::unordered_map<uint64_t, ProductInfo>;

  TraderMap m_orders;        // traders orders
  ProductMap m_products;     // hyp pos for products
  Thresholds m_serverConfig; // config
  ReqBuf m_requestBuffer;    // Holds the client request and the serve response
  NameBuf m_clientName;      // stores the hostname of the client
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
   * @brief Handle new order request from a client. It should insert the new
   * order into the trader map of orders and update the system state if the
   * order passes the requirements of the risk server
   * @param msg - the new order message from the client
   * @param clientFd - the connection between the client and the server (socket
   * fd)
   * @return OrderResponse - message to be sent back to the client
   */
  OrderResponse handle_order(Message<NewOrder> const &msg, int clientFd);

  /**
   * @brief Handle delete order request from a client. It should delete an order
   * if it exists for the specific trader. If not simply reject the order.
   * @param msg - the delete order message from the client
   * @param clientFd - the connection between the client and the server (socket
   * fd)
   * @return OrderResponse - message to be sent back to the client
   */
  OrderResponse handle_order(Message<DeleteOrder> const &msg, int clientFd);

  /**
   * @brief Handle modify order quantity request from a client. It should modify
   * an order for a specific trader if it exists and if it does not break any of
   * the requirements of the risk server.
   * @param msg - the modify quantity order message from the client
   * @param clientFd - the connection between the client and the server (socket
   * fd)
   * @return OrderResponse - message to be sent back to the client
   */
  OrderResponse handle_order(Message<ModifyOrderQuantity> const &msg,
                             int clientFd);

  /**
   * @brief Handle trade message from the client. It should execute the trade
   * and update the status of the server.
   * @param msg - the trade order message from the client
   * @param clientFd - the connection between the client and the server (socket
   * fd)
   * @return OrderResponse - message to be sent back to the client
   */
  OrderResponse handle_order(Message<Trade> const &msg, int clientFd);

  /**
   * @brief Finds an order by an id for a specific trader given the connection
   * and the order of the trader.
   * @param orderId - the id of the order that should be found.
   * @param clientFd - the connection socket between the client and the server
   * (traderId)
   * @return optional<Order> - return an order if it exists and otherwise return
   * nullopt.
   */
  std::optional<Order> find_order_by_id(int clientFd, int orderId);

  /**
   * @brief Handle new incoming connection. The new connection will be included
   * in the set of fds.
   */
  void handle_new_connection();

  void print_system_state();

  /**
   * @brief It will create a new socket try to bind it and then return it to the
   * user. This socket is the listener socket. If we encounter critical error we
   * will return a nullopt.
   * @return the listener fd if exists or nullopt
   */
  std::optional<int> get_listener_fd();
};

#endif
