#include "include/client.h"

int main() {
  std::string host{"localhost"};
  std::string port{"4000"};
  TCPClient client{std::move(host), std::move(port)};

  client.run();
  return 0;
}
