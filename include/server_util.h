#ifndef SERVER_UTIL_INCLUDED_H
#define SERVER_UTIL_INCLUDED_H

#include <cstdint>
#include <ostream>

static constexpr size_t BACK_LOG = 20;
static constexpr int INVALID_FD = -1000;
static constexpr int IMPLICIT_DEC = 10000;

struct ProductInfo {
  uint64_t NetPos{0};
  uint64_t BuyQty{0};
  uint64_t SellQty{0};
  uint64_t MBuy{0};
  uint64_t MSell{0};

  friend std::ostream &operator<<(std::ostream &out, ProductInfo const &pr) {
    out << "NetPos: " << pr.NetPos << ", BuyQty: " << pr.BuyQty
        << ", SellQty: " << pr.SellQty << ", MaxBuy: " << pr.MBuy
        << ", MSell: " << pr.MSell;
    return out;
  }
};

struct Order {
  int m_traderFd;
  uint64_t m_id;
  uint64_t m_productId;
  uint64_t m_quantity;
  double m_price;
  char m_side;
};

struct ServerConfig {
  uint64_t BuyLimit;
  uint64_t SellLimit;
};

struct ServerInfo {
  uint64_t BuyLimit{100};
  uint64_t SellLimit{100};
  std::string Host{"localhost"};
  std::string Port{"4000"};
};

#endif
