#include "include/server.h"
#include <algorithm>
#include <bits/chrono.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <unistd.h>

#include <util/util.h>

Server::Server(std::string &&host, std::string &&port)
    : m_orders(), m_products(), m_serverConfig(), m_requestBuffer(),
      m_clientName(), m_toDel(), m_currDelIdx(0), m_clientAddr(), m_sinSize(0),
      m_fds(), m_host(host), m_port(port), m_listenerfd(INVALID_FD) {
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

  Message<OrderResponse> rsp;

  // Create a message from the bytes received

  switch (nbytes) {
  case NEWO_MSG_SIZE: {
    Message<NewOrder> msg =
        create_msg_from_type<NewOrder>(m_requestBuffer, nbytes);
    deserialize(msg);
    std::cout << msg;
    rsp.data = handle_order(msg, client_fd);
    break;
  }
  case DELO_MSG_SIZE: {
    Message<DeleteOrder> msg =
        create_msg_from_type<DeleteOrder>(m_requestBuffer, nbytes);
    deserialize(msg);
    std::cout << msg;
    rsp.data = handle_order(msg, client_fd);
    break;
  }
  case MODO_MSG_SIZE: {
    Message<ModifyOrderQuantity> msg =
        create_msg_from_type<ModifyOrderQuantity>(m_requestBuffer, nbytes);
    deserialize(msg);
    std::cout << msg;
    rsp.data = handle_order(msg, client_fd);
    break;
  }
  case TRO_MSG_SIZE: {
    Message<Trade> msg = create_msg_from_type<Trade>(m_requestBuffer, nbytes);
    deserialize(msg);
    std::cout << msg;
    rsp.data = handle_order(msg, client_fd);
    break;
  }
  default:
    std::cerr << "Cannot handle this message!\n";
    exit(1);
  };

  // Create header and send response
  uint64_t time = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
  Header responseHeader{.version = 1,
                        .payloadSize = sizeof(OrderResponse),
                        .sequenceNumber = 10,
                        .timestamp = time};
  rsp.header = std::move(responseHeader);
  serialize(rsp);

  size_t toSend = ORDR_MSG_SIZE;
  ssize_t actuallySent = send(client_fd, &rsp, toSend, 0);
  if (actuallySent == -1) {
    std::cerr << "Some err\n";
  }

  print_system_state();
}

OrderResponse Server::handle_order(Message<NewOrder> const &msg, int clientFd) {
  Order ord;
  ord.m_traderFd = clientFd;
  ord.m_id = msg.data.orderId;
  ord.m_productId = msg.data.listingId;
  ord.m_quantity = msg.data.orderQuantity;
  ord.m_price = static_cast<double>(msg.data.orderPrice) / 10000.0;
  ord.m_side = msg.data.side;

  m_orders[clientFd].insert(std::make_pair(ord.m_id, ord));

  // update server state
  auto prod_it = m_products.find(ord.m_productId);
  if (prod_it == m_products.end()) { // insert default if it does not exist
    prod_it =
        m_products.insert(std::make_pair(ord.m_productId, ProductInfo())).first;
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

  // If we violate server do not add new order
  if (prod.MBuy > m_serverConfig.buy && prod.MSell > m_serverConfig.sell) {
    resp.status = OrderResponse::Status::REJECTED;
    return resp;
  }

  m_products[ord.m_productId] = std::move(prod); // update the product
  resp.status = OrderResponse::Status::ACCEPTED;
  return resp;
}

OrderResponse Server::handle_order(Message<DeleteOrder> const &msg,
                                   int clientFd) {
  OrderResponse resp;
  resp.orderId = msg.data.orderId;
  resp.messageType = OrderResponse::MESSAGE_TYPE;
  std::optional<Order> order(find_order_by_id(clientFd, msg.data.orderId));
  if (!order.has_value()) {
    resp.status = OrderResponse::Status::REJECTED;
    return resp;
  }

  Order &ord_v = order.value();
  auto prod_it = m_products.find(ord_v.m_productId);
  ProductInfo prod = prod_it->second;

  if (ord_v.m_side == 'B') {
    prod.BuyQty -= ord_v.m_quantity;
  } else {
    prod.SellQty -= ord_v.m_quantity;
  }

  prod.MBuy = std::max(prod.MBuy, prod.MBuy + prod.NetPos);
  prod.MSell = std::max(prod.SellQty, prod.SellQty - prod.NetPos);
  m_products[ord_v.m_productId] = std::move(prod); // update value

  // erase the order
  m_orders[clientFd].erase(order.value().m_id);
  resp.status = OrderResponse::Status::ACCEPTED;
  return resp;
}
OrderResponse Server::handle_order(Message<ModifyOrderQuantity> const &msg,
                                   int clientFd) {

  OrderResponse resp;
  resp.messageType = OrderResponse::MESSAGE_TYPE;
  resp.orderId = msg.data.orderId;
  // Find the order and if it exists modify quantity
  std::optional<Order> order(find_order_by_id(clientFd, msg.data.orderId));
  if (!order.has_value()) {
    resp.status = OrderResponse::Status::REJECTED;
    return resp;
  }

  // Update state

  // send response
  resp.status = OrderResponse::Status::ACCEPTED;
  return resp;
}
OrderResponse Server::handle_order(Message<Trade> const &msg, int clientFd) {
  OrderResponse resp;
  resp.messageType = OrderResponse::MESSAGE_TYPE;
  resp.orderId = msg.data.tradeId;

  std::optional<Order> order(find_order_by_id(clientFd, msg.data.tradeId));
  if (!order.has_value()) {
    resp.status = OrderResponse::Status::REJECTED;
    return resp;
  }

  // Find the order and execute a trade

  // send response
  resp.status = OrderResponse::Status::ACCEPTED;
  return resp;
}

std::optional<Order> Server::find_order_by_id(int clientFd, int orderId) {
  auto pos = m_orders.find(clientFd);
  if (pos == m_orders.end()) { // no orders for trader or wrong order
    return std::nullopt;
  }
  auto &trader_orders = pos->second;
  auto order_it = trader_orders.find(orderId);
  if (order_it == trader_orders.end()) {
    return std::nullopt;
  }

  return std::optional<Order>(order_it->second);
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

void Server::print_system_state() {
  for (auto const &[k, v] : m_products) {
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
