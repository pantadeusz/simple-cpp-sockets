/**
 * Basic example of server. You can check it using
 *
 * netcat localhost 2212
 *
 * Documentation license
 * **/

#include <tepsoc.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

int main() {
  using namespace tp::net;
  bool continue_serving = true;
  tp::net::server srv([&](tp::net::socket &s) {
    s.on(DATA, [&s, &continue_serving](std::string str) {
      std::cout << "<<" << str << std::endl;
      if (str.substr(0, 3) == "end") {
        s.end("goodbye");
        continue_serving = false;
      } else
        s.write(std::string(">>") + str);
    });
  });
  srv.on(LISTENING,
         [](int port, std::string addr) {
           std::cout << "listening: " << port << " on addr " << addr
                     << std::endl;
         })
      .listen(2212);
  while (continue_serving)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}