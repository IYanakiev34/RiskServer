#include "include/server.h"

int main() {
  std::string g_host{"127.0.0.1"};
  std::string g_port{"4000"};

  Server srv{std::move(g_host), std::move(g_port)};
  srv.listen();
  srv.run();

  return 0;
}
