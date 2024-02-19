#ifndef CONNECTION_INCLUDED_H
#define CONNECTION_INCLUDED_H

#include "orders.h"

class Server;

class Connection {
  int m_traderSock;
  Server const *m_server;

public:
  // TODO: implement all of them (copy from server and audjust)
  Connection(int sockfd, Server owner);
  Connection(Connection const &) = delete;
  Connection &operator=(Connection const &) = delete;

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
};

#endif
