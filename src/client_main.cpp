#include "include/client.h"

int main() {
  std::string host{"localhost"};
  std::string port{"4000"};
  TCPClient client{std::move(host), std::move(port)};

  Message<NewOrder> msg{
      .header{
          .version = 1, .payloadSize = 35, .sequenceNumber = 1, .timestamp = 1},
      .data{.messageType = NewOrder::MESSAGE_TYPE,
            .listingId = 1,
            .orderId = 10,
            .orderQuantity = 10,
            .orderPrice = 10000000,
            .side = 'B'}};

  // new order message
  client.sendData(msg);

  return 0;
}
