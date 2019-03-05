/**
 * @file server.cpp
 * @author Tadeusz Puźniakowski
 * @brief This is verry basic server implementation in C++17
 * @version 0.1
 * @date 2019-01-26
 *
 * @copyright Copyright (c) 2019 Tadeusz Puźniakowski
 * @license MIT
 */

// g++ -std=c++17 server.cpp -Lbuild -lsimplecppsockets -Iinclude -pthread

#include <simplecppsockets.hpp>
#include <vector>
#include <iostream>

#include <unistd.h>

using namespace tp;
int main(int argc, char **argv) {
  std::vector<char *> args(argv, argv + argc);
  std::vector< int > listen_sockets = accept_connections(
      [](int sockfd, std::string from_addr, std::string from_port) {
        std::cout << "Connection from " << from_addr << ":" << from_port << std::endl;
      },
      "*", (args.size() > 1) ? args[1] : "9921", 2,
      [&listen_sockets](auto s) { listen_sockets.push_back(s); },
      [](auto err) { std::cout << "Error during server initialization: " << err << std::endl; });
  std::string waiting;
  std::cout << "Type some text and press enter to finish...: " << std::endl;
  std::cin >> waiting;
  for (auto &s: listen_sockets) {
    ::close(s);
  }
  return 0;
}
