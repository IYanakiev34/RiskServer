# Simple Risk Server
The goal of this risk server is to simulate a communication between a client (**trader**)
and an intermediary server (**risk server**), which will calculate the risk of the trade
through some predetermined thresholds. If the trade violates these tresholds it will not
be sent to an exchage and it will be rejected. A message will be send back to the trader
to let them know. If accepted the order should be send to an exchange for processing (**this is
not part of the project**). The client will receive a message with the status of accepeted.
For efficiency the communication between the client and the server will be in binary format.
This will make the transmistion of data faster since the packets will be smaller. The
serialization should work on both little and big-endian machines.

## Functional and Non-Functional Requirements
1. The application should be a TCP server / client communication.
2. Calculate worst net position for every trader.
3. Compare worst net position for incoming order and compare it to some threshold
4. When order is rejected the system state will not change (only the client will be infromed
about it)
5. When an order is accepted the system state will be update, a new hypothetical worst net
position will be calculated.
6. When a trader is disconnected all stored state about him should be discarded.
7. Trades with positive quantity are **Long Trades**
8. Trade with negative quantity are  **Short Trades**
9. Each message will contain exactly one header and one message.

## Hypothetical Worst Net Position
For every financial instrument that has been traded and information is stored on the server
we need to calculate the wors possible positions, which will be calculated as follows:

1. Sum of all received trades (Net Position)
2. Sum of all buy orders quantity (Buy Quantity)
3. Sum of all sell orders quantity (Sell Quantity)
4. Calculate buy side as max(Buy Quantity, Buy Quantity + NetPosition)
5. Calculate sell side as max(Sell Quantity, Sell Quantity + NetPosition)

## Outline of the message spec
```cpp
    struct Header {
      uint16_t version;        // protocol version
      uint16_t payloadSize;    // payload size in bytes
      uint32_t sequenceNumber; // sequence number for this package
      uint64_t timestamp;      // number of nanosecods from Unix epoch
                               //
    } __attribute__((__packed__));
    static_assert(sizeof(Header) == 16, "The Header size is not correct!");

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

    struct DeleteOrder {
      static constexpr uint16_t MESSAGE_TYPE = 2;
      uint16_t messageType; // type of message
      uint64_t orderId;     // the id of the order to delete
    } __attribute__((__packed__));
    static_assert(sizeof(DeleteOrder) == 10,
                  "The DeleteOrder size is not correct!");

    struct ModifyOrderQuantity {
      static constexpr uint16_t MESSAGE_TYPE = 3;

      uint16_t messageType; // the type of the message
      uint64_t orderId;     // the id of the order to modify
      uint64_t newQuantity; // the new quantity to be set
    } __attribute__((__packed__));
    static_assert(sizeof(ModifyOrderQuantity) == 18,
                  "The ModifyOrderQuantity size is not correct!");

    struct Trade {
      static constexpr uint16_t MESSAGE_TYPE = 4;
      uint16_t messageType;   // type of message
      uint64_t listingId;     // Financial instrument id associated with this id
      uint64_t tradeId;       // Order id that refers to the original order id
      uint64_t tradeQuantity; // Trade quantity
      uint64_t tradePrice;    // Trade price, the price contains 4 implicit decimals
    } __attribute__((__packed__));
    static_assert(sizeof(Trade) == 34, "The Trade size is not correct!");

    struct OrderResponse {
      static constexpr uint16_t MESSAGE_TYPE = 5;
      enum class Status : uint16_t { ACCEPTED = 0, REJECTED = 1 };

      uint16_t messageType; // the type of the message
      uint64_t orderId;     // the id of the order that we will send status to
      Status status;        // status of order
    } __attribute__((__packed__));
    static_assert(sizeof(OrderResponse) == 12,
                  "The OrderResponse size is not correct!");
```

