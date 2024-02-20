#include "include/connection.h"
#include "include/orders.h"
#include "include/server.h"
#include <chrono>
#include <iostream>

uint32_t Connection::s_sequenceNumber = 0;

Connection::Connection(int sockfd, Server *owner)
    : m_reqBuf(), m_orders(), m_resBuf(), m_nbytes(0), m_traderSock(sockfd),
      m_server(owner) {}

Connection::~Connection() { shutdown_connection(); }

void Connection::handle_client_request() {
  fill_request_buffer(); // extract client order

  // Handle client order
  switch (m_nbytes) {
  case NEWO_MSG_SIZE: {
    handle_order<NewOrder>();
    break;
  }
  case DELO_MSG_SIZE: {
    handle_order<DeleteOrder>();
    break;
  }
  case MODO_MSG_SIZE: {
    handle_order<ModifyOrderQuantity>();
    break;
  }
  case TRO_MSG_SIZE: {
    handle_order<Trade>();
    break;
  }
  default:
    std::cerr << "Cannot handle this message!\n";
    exit(1);
  };

  generate_response_msg(); // create full response message
  send_message();          // send to client

  m_server->print_system_state(); // print system state
}

void Connection::fill_request_buffer() {

  m_nbytes = recv(m_traderSock, m_reqBuf.data(), m_reqBuf.size(), 0);
  std::cout << "Connection [ " << m_traderSock << "] got: " << m_nbytes
            << " bytes\n\n";

  if (m_nbytes <= 0) { // close the conection
    if (m_nbytes == 0) {
      std::cout << "pollconnection: " << m_traderSock << " hung up\n";
    }
    std::perror("recv");
    // TODO: deregister
  }
}

void Connection::shutdown_connection() {
  shutdown(m_traderSock, SHUT_RDWR);
  close(m_traderSock);
}

void Connection::generate_response_msg() {
  // Create header and send response
  uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
  Header responseHeader{.version = 1,
                        .payloadSize = sizeof(OrderResponse),
                        .sequenceNumber = s_sequenceNumber++,
                        .timestamp = time};
  m_resBuf.header = std::move(responseHeader);
}

void Connection::send_message() {
  serialize(m_resBuf);

  size_t toSend = ORDR_MSG_SIZE;
  ssize_t actuallySent = send(m_traderSock, &m_resBuf, toSend, 0);
  if (actuallySent == -1) {
    std::cerr << "Some err\n";
  }
}

template <Sendable T> void Connection::handle_order() {
  Message<T> msg = create_msg_from_type<T>(m_reqBuf, m_nbytes);
  deserialize(msg);
  std::cout << msg;
  m_resBuf.data = handle_order(msg);
}

OrderResponse Connection::handle_order(Message<NewOrder> const &msg) {
  // Create order from message
  Order ord;
  ord.m_traderFd = m_traderSock;
  ord.m_id = msg.data.orderId;
  ord.m_productId = msg.data.listingId;
  ord.m_quantity = msg.data.orderQuantity;
  ord.m_price = static_cast<double>(msg.data.orderPrice) / IMPLICIT_DEC;
  ord.m_side = msg.data.side;

  m_orders.insert(std::make_pair(ord.m_id, ord));

  // update server state
  auto &ProductMap = m_server->get_products();
  auto prod_it = ProductMap.find(ord.m_productId);
  if (prod_it == ProductMap.end()) { // insert default if it does not exist
    prod_it =
        ProductMap.insert(std::make_pair(ord.m_productId, ProductInfo())).first;
  }

  ProductInfo prod = prod_it->second;
  if (ord.m_side == 'B') {
    prod.BuyQty += ord.m_quantity;
  } else {
    prod.SellQty += ord.m_quantity;
  }

  prod.MBuy = std::max(prod.BuyQty, prod.NetPos + prod.BuyQty);
  prod.MSell = std::max(prod.SellQty, prod.SellQty - prod.NetPos);

  OrderResponse resp;
  resp.orderId = ord.m_id;
  resp.messageType = OrderResponse::MESSAGE_TYPE;

  ServerInfo &info = m_server->get_info();
  // If we violate server do not add new order
  if (prod.MBuy > info.BuyLimit && prod.MSell > info.SellLimit) {
    resp.status = OrderResponse::Status::REJECTED;
    return resp;
  }

  ProductMap[ord.m_productId] = std::move(prod); // update the product
  resp.status = OrderResponse::Status::ACCEPTED;
  return resp;
}

OrderResponse Connection::handle_order(Message<DeleteOrder> const &msg) {
  OrderResponse resp;
  resp.orderId = msg.data.orderId;
  resp.messageType = OrderResponse::MESSAGE_TYPE;
  std::optional<Order> order(find_order_by_id(msg.data.orderId));
  if (!order.has_value()) {
    resp.status = OrderResponse::Status::REJECTED;
    return resp;
  }

  Order &ord_v = order.value();
  auto &ProductMap = m_server->get_products();
  auto prod_it = ProductMap.find(ord_v.m_productId);
  ProductInfo prod = prod_it->second;

  if (ord_v.m_side == 'B') {
    prod.BuyQty -= ord_v.m_quantity;
  } else {
    prod.SellQty -= ord_v.m_quantity;
  }

  prod.MBuy = std::max(prod.MBuy, prod.MBuy + prod.NetPos);
  prod.MSell = std::max(prod.SellQty, prod.SellQty - prod.NetPos);
  ServerInfo &info = m_server->get_info();
  if (prod.MBuy > info.BuyLimit || prod.MSell > info.SellLimit) {
    resp.status = OrderResponse::Status::REJECTED;
    return resp;
  }
  ProductMap[ord_v.m_productId] = std::move(prod); // update value

  // erase the order
  ProductMap.erase(ord_v.m_id);
  resp.status = OrderResponse::Status::ACCEPTED;
  return resp;
}

OrderResponse
Connection::handle_order(Message<ModifyOrderQuantity> const &msg) {

  OrderResponse resp;
  resp.messageType = OrderResponse::MESSAGE_TYPE;
  resp.orderId = msg.data.orderId;
  // Find the order and if it exists modify quantity
  std::optional<Order> order(find_order_by_id(msg.data.orderId));
  if (!order.has_value()) {
    resp.status = OrderResponse::Status::REJECTED;
    return resp;
  }

  /**
   * 1. Get the order
   * 2. Check for changes in hypothetical worst pos
   *    - get abs(ord.quat - newQuant)
   *    - add diff to either sell or buy
   *    - calculate pos
   *    - either accept or reject
   *
   */

  // send response
  resp.status = OrderResponse::Status::ACCEPTED;
  return resp;
}

OrderResponse Connection::handle_order(Message<Trade> const &msg) {
  OrderResponse resp;
  resp.messageType = OrderResponse::MESSAGE_TYPE;
  resp.orderId = msg.data.tradeId;

  std::optional<Order> order(find_order_by_id(msg.data.tradeId));
  if (!order.has_value()) {
    resp.status = OrderResponse::Status::REJECTED;
    return resp;
  }

  // Find the order and execute a trade

  // send response
  resp.status = OrderResponse::Status::ACCEPTED;
  return resp;
}

std::optional<Order> Connection::find_order_by_id(int orderId) {
  auto pos = m_orders.find(orderId);
  if (pos == m_orders.end()) { // no orders for trader or wrong order
    return std::nullopt;
  }
  return std::optional<Order>(pos->second);
}
