template <Sendable T> inline void serialize(T &) {}

template <> void inline serialize<Header>(Header &type) {
  SERIALIZE_16(type.version);
  SERIALIZE_16(type.payloadSize);
  SERIALIZE_32(type.sequenceNumber);
  SERIALIZE_64(type.timestamp);
}

template <> inline void serialize<NewOrder>(NewOrder &order) {
  SERIALIZE_16(order.messageType);
  SERIALIZE_64(order.listingId);
  SERIALIZE_64(order.orderId);
  SERIALIZE_64(order.orderQuantity);
  SERIALIZE_64(order.orderPrice);
  // Do no serialize char no need
}

template <> inline void serialize<DeleteOrder>(DeleteOrder &order) {
  SERIALIZE_16(order.messageType);
  SERIALIZE_64(order.orderId);
}

template <>
inline void serialize<ModifyOrderQuantity>(ModifyOrderQuantity &order) {
  SERIALIZE_16(order.messageType);
  SERIALIZE_64(order.orderId);
  SERIALIZE_64(order.newQuantity);
}

template <> inline void serialize<Trade>(Trade &trade) {
  SERIALIZE_16(trade.messageType);
  SERIALIZE_64(trade.listingId);
  SERIALIZE_64(trade.tradeId);
  SERIALIZE_64(trade.tradeQuantity);
  SERIALIZE_64(trade.tradePrice);
}

template <> inline void serialize<OrderResponse>(OrderResponse &response) {
  SERIALIZE_16(response.messageType);
  SERIALIZE_64(response.orderId);
  uint16_t casted = static_cast<uint16_t>(response.status);
  SERIALIZE_16(casted);
  response.status = static_cast<OrderResponse::Status>(casted);
}

template <Sendable T> inline void serialize(Message<T> &msg) {
  serialize(msg.header);
  serialize(msg.data);
}

template <Sendable T> inline void deserialize(T &) {}

template <> inline void deserialize<Header>(Header &type) {
  type.version = ntohs(type.version);
  type.payloadSize = ntohs(type.payloadSize);
  type.sequenceNumber = ntohl(type.sequenceNumber);
  type.timestamp = be64toh(type.timestamp);
}

template <> inline void deserialize<NewOrder>(NewOrder &order) {
  DESERIALIZE_16(order.messageType);
  DESERIALIZE_64(order.listingId);
  DESERIALIZE_64(order.orderId);
  DESERIALIZE_64(order.orderQuantity);
  DESERIALIZE_64(order.orderPrice);
}

template <> inline void deserialize<DeleteOrder>(DeleteOrder &order) {
  DESERIALIZE_16(order.messageType);
  DESERIALIZE_64(order.orderId);
}

template <>
inline void deserialize<ModifyOrderQuantity>(ModifyOrderQuantity &order) {
  DESERIALIZE_16(order.messageType);
  DESERIALIZE_64(order.orderId);
  DESERIALIZE_64(order.newQuantity);
}

template <> inline void deserialize<Trade>(Trade &trade) {
  DESERIALIZE_16(trade.messageType);
  DESERIALIZE_64(trade.listingId);
  DESERIALIZE_64(trade.tradeId);
  DESERIALIZE_64(trade.tradeQuantity);
  DESERIALIZE_64(trade.tradePrice);
}

template <> inline void deserialize<OrderResponse>(OrderResponse &response) {
  DESERIALIZE_16(response.messageType);
  DESERIALIZE_64(response.orderId);
  uint16_t status = static_cast<uint16_t>(response.status);
  DESERIALIZE_16(status);
  response.status = static_cast<OrderResponse::Status>(status);
}

template <Sendable T> inline void deserialize(Message<T> &msg) {
  deserialize(msg.header);
  deserialize(msg.data);
}
