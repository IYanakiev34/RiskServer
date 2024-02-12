#include "include/orders.h"
template <typename T> void serialize(T &) {}

template <> void serialize<Header>(Header &type) {
  SERIALIZE_16(type.version);
  SERIALIZE_16(type.payloadSize);
  SERIALIZE_32(type.sequenceNumber);
  SERIALIZE_64(type.timestamp);
}

template <> void serialize<NewOrder>(NewOrder &order) {
  SERIALIZE_16(order.messageType);
  SERIALIZE_64(order.listingId);
  SERIALIZE_64(order.orderId);
  SERIALIZE_64(order.orderQuantity);
  SERIALIZE_64(order.orderPrice);
  // Do no serialize char no need
}

template <> void serialize<DeleteOrder>(DeleteOrder &order) {
  SERIALIZE_16(order.messageType);
  SERIALIZE_64(order.orderId);
}

template <> void serialize<ModifyOrderQuantity>(ModifyOrderQuantity &order) {
  SERIALIZE_16(order.messageType);
  SERIALIZE_64(order.orderId);
  SERIALIZE_64(order.newQuantity);
}

template <> void serialize<Trade>(Trade &trade) {
  SERIALIZE_16(trade.messageType);
  SERIALIZE_64(trade.listingId);
  SERIALIZE_64(trade.tradeId);
  SERIALIZE_64(trade.tradeQuantity);
  SERIALIZE_64(trade.tradePrice);
}

template <> void serialize<OrderResponse>(OrderResponse &response) {
  SERIALIZE_16(response.messageType);
  SERIALIZE_64(response.orderId);
  uint16_t casted = static_cast<uint16_t>(response.status);
  SERIALIZE_16(casted);
  response.status = static_cast<OrderResponse::Status>(casted);
}

template <typename T> void deserialize(T &) {}

template <> void deserialize<Header>(Header &type) {
  type.version = ntohs(type.version);
  type.payloadSize = ntohs(type.payloadSize);
  type.sequenceNumber = ntohl(type.sequenceNumber);
  type.timestamp = be64toh(type.timestamp);
}

template <> void deserialize<NewOrder>(NewOrder &order) {
  DESERIALIZE_16(order.messageType);
  DESERIALIZE_64(order.listingId);
  DESERIALIZE_64(order.orderId);
  DESERIALIZE_64(order.orderQuantity);
  DESERIALIZE_64(order.orderPrice);
}

template <> void deserialize<DeleteOrder>(DeleteOrder &order) {
  DESERIALIZE_16(order.messageType);
  DESERIALIZE_64(order.orderId);
}

template <> void deserialize<ModifyOrderQuantity>(ModifyOrderQuantity &order) {
  DESERIALIZE_16(order.messageType);
  DESERIALIZE_64(order.orderId);
  DESERIALIZE_64(order.newQuantity);
}

template <> void deserialize<Trade>(Trade &trade) {
  DESERIALIZE_16(trade.messageType);
  DESERIALIZE_64(trade.listingId);
  DESERIALIZE_64(trade.tradeId);
  DESERIALIZE_64(trade.tradeQuantity);
  DESERIALIZE_64(trade.tradePrice);
}

template <> void deserialize<OrderResponse>(OrderResponse &response) {
  DESERIALIZE_16(response.messageType);
  DESERIALIZE_64(response.orderId);
  uint16_t status = static_cast<uint16_t>(response.status);
  DESERIALIZE_16(status);
  response.status = static_cast<OrderResponse::Status>(status);
}

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
      << "\nOrderPrice: " << static_cast<double>(h.orderPrice) / 10000.00
      << std::endl;
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
      << "\nTradePrice: " << static_cast<double>(h.tradePrice) / 10000.00
      << std::endl;
  return out;
}

std::ostream &operator<<(std::ostream &out, OrderResponse const &h) {
  out << "MesageType: " << OrderResponse::MESSAGE_TYPE
      << "\nOrderId: " << h.orderId << "\nStatus: "
      << (h.status == OrderResponse::Status::ACCEPTED ? "ACCEPTED"
                                                      : "REJECTED");
  return out;
}
