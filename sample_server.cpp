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
    std::cout << "ktos sie podlaczyl" << std::endl;
    s.on(DATA,[&s,&continue_serving](std::string str){
      std::cout << "MAM:" << str << std::endl;
      if (str.substr(0,3) == "end") {
        s.end("goodbye");
        continue_serving = false;
      }
      else s.write(std::string("Dzieki za info: ") + str);
    });    
  });
  srv.on(LISTENING, [](int port, std::string addr) {
      std::cout << "nasluchujemy: " << port << " on addr " << addr << std::endl;
  });
  srv.listen(2212);
  while (continue_serving)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

}