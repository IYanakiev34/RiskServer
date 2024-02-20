#ifndef CONNECTION_INCLUDED_H
#define CONNECTION_INCLUDED_H

#include "orders.h"
#include "server_util.h"
#include <array>
#include <memory>
#include <optional>
#include <unordered_map>

class Server;

class Connection : public std::enable_shared_from_this<Connection> {
  static uint32_t s_sequenceNumber;
  enum { buf_size = 256 };
  std::array<char, buf_size> m_reqBuf;
  std::unordered_map<uint64_t, Order> m_orders;
  Message<OrderResponse> m_resBuf;
  ssize_t m_nbytes;
  int m_traderSock;
  Server *m_server;

public:
  Connection(int sockfd, Server *owner);
  ~Connection();
  Connection(Connection const &) = delete;
  Connection &operator=(Connection const &) = delete;

  /**
   * @brief Handle a client request and send back an appropriate response.
   * The method can handle any Sendable type of client request.
   */
  void handle_client_request();

  /**
   * @brief Get the underlying socket for communication.
   * @return the socket for communication
   */
  inline int get_socket() const noexcept { return m_traderSock; }

private:
  /**
   * @brief Helper method to extract the client request from the network stream
   * into a local connection buffer.
   */
  void fill_request_buffer();

  /**
   * @brief Shutdown the communication with the client. It will shutdown the
   * socket and then close it.
   */
  void shutdown_connection();

  /**
   * @brief Generate response message to be sent to the client.
   */
  void generate_response_msg();

  /**
   * @brief Send the response message to the client
   */
  void send_message();

  /**
   * @brief Handle arbitrary order of some Sendable type. The method will
   * deserialize the local buffer into a message print it out and then handle
   * the request.
   */
  template <Sendable T> void handle_order();

  /**
   * @brief Handle new order request from a client. It should insert the new
   * order into the trader map of orders and update the system state if the
   * order passes the requirements of the risk server
   * @param msg - the new order message from the traderSock
   * @return OrderResponse - message to be sent back to the client
   */
  OrderResponse handle_order(Message<NewOrder> const &msg);

  /**
   * @brief Handle delete order request from a client. It should delete an order
   * if it exists for the specific trader. If not simply reject the order.
   * @param msg - the delete order message from the client
   * @return OrderResponse - message to be sent back to the client
   */
  OrderResponse handle_order(Message<DeleteOrder> const &msg);

  /**
   * @brief Handle modify order quantity request from a client. It should modify
   * an order for a specific trader if it exists and if it does not break any of
   * the requirements of the risk server.
   * @param msg - the modify quantity order message from the client
   * @return OrderResponse - message to be sent back to the client
   */
  OrderResponse handle_order(Message<ModifyOrderQuantity> const &msg);

  /**
   * @brief Handle trade message from the client. It should execute the trade
   * and update the status of the server.
   * @param msg - the trade order message from the client
   * @return OrderResponse - message to be sent back to the client
   */
  OrderResponse handle_order(Message<Trade> const &msg);

  /**
   * @brief Find an order with a specific ID for the trader. If the order does
   * not exist simply return a nullopt.
   * @param orderId - the id of the order
   * @return nullopt if the order does not exist otherwise an optional with the
   * order value inside.
   */
  std::optional<Order> find_order_by_id(int orderId);
};

#endif
