#ifndef ORDERS_INCLUDED_H
#define ORDERS_INCLUDED_H
#include <cstdint>
#include <endian.h>
#include <netinet/in.h>
#include <ostream>

#define SERIALIZE_16(prop) (prop = htons(prop))
#define SERIALIZE_32(prop) (prop = htonl(prop))
#define SERIALIZE_64(prop) (prop = htobe64(prop))

#define DESERIALIZE_16(prop) (prop = ntohs(prop))
#define DESERIALIZE_32(prop) (prop = ntohs(prop))
#define DESERIALIZE_64(prop) (prop = be64toh(prop))

/**
 * @brief Header packet that will be attached to each message. Contains the
 * version of the protocol being used. The size of the actual payload, the
 * sequence of the packet and a timestamp in the format of a unix epoch.
 */
struct Header {
  uint16_t version;        // protocol version
  uint16_t payloadSize;    // payload size in bytes
  uint32_t sequenceNumber; // sequence number for this package
  uint64_t timestamp;      // number of nanosecods from Unix epoch
                           //
} __attribute__((__packed__));
static_assert(sizeof(Header) == 16, "The Header size is not correct!");

/**
 * @brief Payload packet for NewOrder type of request. This request will try to
 * place a new order on the exchange for a given financial instrument.
 */
struct NewOrder {
  static constexpr uint16_t MESSAGE_TYPE = 1;
  uint16_t messageType;   // type of message
  uint64_t listingId;     // instrument id associated with the message
  uint64_t orderId;       // order id used for further changes
  uint64_t orderQuantity; // quantity
  uint64_t orderPrice;    // order price, contains 4 implicit decimals
  char side;              // side of the order 'B' for BUY, 'S' for SELL
} __attribute__((__packed__));
static_assert(sizeof(NewOrder) == 35, "The NewOrder size is not correct!");

/**
 * @brief Payload packet for DeleteOrder type of request. This request aims to
 * delete an order from being sent to the exchnage. It uses the orderId to match
 * against the current orders.
 */
struct DeleteOrder {
  static constexpr uint16_t MESSAGE_TYPE = 2;
  uint16_t messageType; // type of message
  uint64_t orderId;     // the id of the order to delete
} __attribute__((__packed__));
static_assert(sizeof(DeleteOrder) == 10,
              "The DeleteOrder size is not correct!");

/**
 * @brief Payload packet for the ModifyOrderQuantity type of request. This
 * request aims to modify the quantity of a certain order.
 */
struct ModifyOrderQuantity {
  static constexpr uint16_t MESSAGE_TYPE = 3;

  uint16_t messageType; // the type of the message
  uint64_t orderId;     // the id of the order to modify
  uint64_t newQuantity; // the new quantity to be set
} __attribute__((__packed__));
static_assert(sizeof(ModifyOrderQuantity) == 18,
              "The ModifyOrderQuantity size is not correct!");

/**
 * @brief Payload packet for the Trade type of request. This request aims to
 * trade some volume of a financial instrument at some price.
 */
struct Trade {
  static constexpr uint16_t MESSAGE_TYPE = 4;
  uint16_t messageType;   // type of message
  uint64_t listingId;     // Financial instrument id associated with this id
  uint64_t tradeId;       // Order id that refers to the original order id
  uint64_t tradeQuantity; // Trade quantity
  uint64_t tradePrice;    // Trade price, the price contains 4 implicit decimals
} __attribute__((__packed__));
static_assert(sizeof(Trade) == 34, "The Trade size is not correct!");

/**
 * @brief Payload packet for the OrderResponse type of message. This message
 * will be sent to the client in response to their most current order.
 */
struct OrderResponse {
  static constexpr uint16_t MESSAGE_TYPE = 5;
  enum class Status : uint16_t { ACCEPTED = 0, REJECTED = 1 };

  uint16_t messageType; // the type of the message
  uint64_t orderId;     // the id of the order that we will send status to
  Status status;        // status of order
} __attribute__((__packed__));
static_assert(sizeof(OrderResponse) == 12,
              "The OrderResponse size is not correct!");

// Serialization of orders
template <typename T> void serialize(T &);
template <> void serialize<Header>(Header &type);
template <> void serialize<NewOrder>(NewOrder &order);
template <> void serialize<DeleteOrder>(DeleteOrder &order);
template <> void serialize<ModifyOrderQuantity>(ModifyOrderQuantity &order);
template <> void serialize<Trade>(Trade &trade);
template <> void serialize<OrderResponse>(OrderResponse &response);

// Deserialization of orders
template <typename T> void deserialize(T &);
template <> void deserialize<Header>(Header &type);
template <> void deserialize<NewOrder>(NewOrder &order);
template <> void deserialize<DeleteOrder>(DeleteOrder &order);
template <> void deserialize<ModifyOrderQuantity>(ModifyOrderQuantity &order);
template <> void deserialize<Trade>(Trade &trade);
template <> void deserialize<OrderResponse>(OrderResponse &response);

std::ostream &operator<<(std::ostream &out, Header const &h);
std::ostream &operator<<(std::ostream &out, NewOrder const &h);
std::ostream &operator<<(std::ostream &out, DeleteOrder const &h);
std::ostream &operator<<(std::ostream &out, ModifyOrderQuantity const &h);
std::ostream &operator<<(std::ostream &out, Trade const &h);
std::ostream &operator<<(std::ostream &out, OrderResponse const &h);

template <typename T>
using remove_cv_ref_ptr = typename std::remove_cv<typename std::remove_pointer<
    typename std::remove_reference<T>::type>::type>::type;

/**
 * Example use could be
 * template <Sendable type>
 * void send(Sendable&& data);
 */
template <typename T>
concept Sendable = requires {
  std::is_same_v<remove_cv_ref_ptr<T>, Header> ||
      std::is_same_v<remove_cv_ref_ptr<T>, NewOrder> ||
      std::is_same_v<remove_cv_ref_ptr<T>, DeleteOrder> ||
      std::is_same_v<remove_cv_ref_ptr<T>, ModifyOrderQuantity> ||
      std::is_same_v<remove_cv_ref_ptr<T>, Trade> ||
      std::is_same_v<remove_cv_ref_ptr<T>, OrderResponse>;
};

template <Sendable T> struct Message {
  Header header;
  T data;
} __attribute__((__packed__));

#endif
