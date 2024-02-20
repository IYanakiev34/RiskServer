#include "include/server.h"
#include <iostream>

void usage() {
  char const *usage = R"(
    ./build/server <buy limit> <sell limit>
  )";
  std::cerr << usage << std::endl;
}

int main(int argc, char **argv) {
  ServerConfig Config;
  if (argc == 0 || argc != 2) {
    Config.BuyLimit = 100;
    Config.SellLimit = 100;
    usage();
  } else {
    Config.BuyLimit = std::stoull(argv[1]);
    Config.SellLimit = std::stoull(argv[2]);
  }
  std::string g_host{"127.0.0.1"};
  std::string g_port{"4000"};

  Server srv{g_host, g_port, Config};
  srv.listen();
  srv.run();

  return 0;
}
