#include "include/client.h"
#include <arpa/inet.h>
#include <chrono>
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

void TCPClient::run() {
  static long long seq = 1;
  std::cout << "If you wish to exit enter:  EXIT\n";
  size_t msg_type = 0;
  Header header;
  Message<OrderResponse> rsp;
  while (true) {
    // clear vars
    std::memset(&header, 0, sizeof(Header));
    std::memset(&rsp, 0, sizeof(Message<OrderResponse>));

    // Preset fields
    header.version = 1;
    header.sequenceNumber = seq++;
    header.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

    // Build input
    std::cout << "Enter Message Type:\n";
    std::cin >> msg_type;
    switch (msg_type) {
    case NewOrder::MESSAGE_TYPE: {
      Message<NewOrder> msg;
      std::memset(&msg, 0, NEWO_MSG_SIZE);
      header.payloadSize = sizeof(NewOrder);
      msg.header = std::move(header);
      msg.data.messageType = NewOrder::MESSAGE_TYPE;

      uint64_t listing_id, order_id, order_q, order_p;
      std::cin >> listing_id >> order_id >> order_q >> order_p;
      msg.data.listingId = listing_id;
      msg.data.orderId = order_id;
      msg.data.orderQuantity = order_q;
      msg.data.orderPrice = order_p;
      sendData(msg);
      break;
    }
    case DeleteOrder::MESSAGE_TYPE: {
      Message<DeleteOrder> msg;
      std::memset(&msg, 0, DELO_MSG_SIZE);
      header.payloadSize = sizeof(DeleteOrder);
      msg.header = std::move(header);
      msg.data.messageType = DeleteOrder::MESSAGE_TYPE;
      uint64_t ord_id;
      std::cin >> ord_id;
      msg.data.orderId = ord_id;
      sendData(msg);
      break;
    }
    case ModifyOrderQuantity::MESSAGE_TYPE: {
      Message<ModifyOrderQuantity> msg;
      std::memset(&msg, 0, MODO_MSG_SIZE);
      msg.header = std::move(header);
      msg.data.messageType = ModifyOrderQuantity::MESSAGE_TYPE;
      uint64_t order_id, new_quant;
      std::cin >> order_id >> new_quant;
      msg.data.orderId = order_id;
      msg.data.newQuantity = new_quant;
      sendData(msg);
      break;
    }
    case Trade::MESSAGE_TYPE: {
      Message<Trade> msg;
      std::memset(&msg, 0, TRO_MSG_SIZE);
      msg.header = std::move(header);
      msg.data.messageType = Trade::MESSAGE_TYPE;
      uint64_t listing_id, trade_id, trade_q, trade_p;
      std::cin >> listing_id >> trade_id >> trade_q >> trade_p;
      msg.data.listingId = listing_id;
      msg.data.tradeId = trade_id;
      msg.data.tradeQuantity = trade_q;
      msg.data.tradePrice = trade_p;
      sendData(msg);
      break;
    }
    default:
      std::cerr << "Illegal type try again: (1,2,3,4)\n";
    }

    // Receive data from server and print it
    ssize_t nbytes = recv(m_sockfd, &rsp, sizeof(Message<OrderResponse>), 0);
    if (nbytes <= 0) {
      std::cerr << "Something went wrong on the server side\n";
    }
    deserialize(rsp);
    std::cout << rsp.header << std::endl;
    std::cout << rsp.data << std::endl;
  }
}

template <Sendable T> void TCPClient::sendData(Message<T> &message) {
  serialize(message);
  ssize_t sendSize = sizeof(Message<T>);
  std::cout << "Send size: " << sendSize << std::endl;

  // TODO: should make sure everything is sent
  ssize_t nsent = send(m_sockfd, &message, sendSize, 0);
  if (nsent != sendSize) {
    std::cout << "Client send: We sent " << nsent << " bytes out of "
              << sendSize << "\n";
  }
}
