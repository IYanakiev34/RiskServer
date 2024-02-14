#include "include/orders.h"
#include <iomanip>

// Printing to ostream
std::ostream &operator<<(std::ostream &out, Header const &h) {
  out << "Header:\n";
  out << "Version: " << h.version << "\nPayload size: " << h.payloadSize
      << "\nSequence number: " << h.sequenceNumber
      << "\nTimestamp: " << h.timestamp << std::endl;
  return out;
}

std::ostream &operator<<(std::ostream &out, NewOrder const &h) {
  out << "NewOrder:\n";
  out << "Message Type: " << NewOrder::MESSAGE_TYPE
      << "\nListingId: " << h.listingId << "\nOrderId: " << h.orderId
      << "\nOrderQuantity: " << h.orderQuantity
      << "\nOrderPrice: " << std::setprecision(4) << std::fixed
      << static_cast<double>(h.orderPrice) / 10000.00 << std::endl;
  return out;
}

std::ostream &operator<<(std::ostream &out, DeleteOrder const &h) {
  out << "DeleteOrder:\n";
  out << "MessageType: " << DeleteOrder::MESSAGE_TYPE
      << "\nOrderId: " << h.orderId << std::endl;
  return out;
}

std::ostream &operator<<(std::ostream &out, ModifyOrderQuantity const &h) {
  out << "ModifyOrderQuantity:\n";
  out << "MessageType: " << ModifyOrderQuantity::MESSAGE_TYPE
      << "\nOrderId: " << h.orderId << "\nNewQuantity: " << h.newQuantity
      << std::endl;
  return out;
}

std::ostream &operator<<(std::ostream &out, Trade const &h) {
  out << "Trade:\n";
  out << "MessageType: " << Trade::MESSAGE_TYPE
      << "\nListingId: " << h.listingId << "\nTradeId: " << h.tradeId
      << "\nTradeQuantity: " << h.tradeQuantity
      << "\nTradePrice: " << std::setprecision(4) << std::fixed
      << static_cast<double>(h.tradePrice) / 10000.00 << std::endl;
  return out;
}

std::ostream &operator<<(std::ostream &out, OrderResponse const &h) {
  out << "MesageType: " << OrderResponse::MESSAGE_TYPE
      << "\nOrderId: " << h.orderId << "\nStatus: "
      << (h.status == OrderResponse::Status::ACCEPTED ? "ACCEPTED"
                                                      : "REJECTED");
  return out;
}
