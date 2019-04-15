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

int main()
{
  using namespace tp::net;
  using namespace std;
  bool continue_serving = true;
  server srv([&](socket &s) {
    s.on(DATA, [&s, &continue_serving](string str) {
      cout << "<<" << str << endl;
      if (str.substr(0, 3) == "end")
      {
        s.end("goodbye");
        continue_serving = false;
      }
      else
        s.write(string(">>") + str);
    });
  });
  srv.on(LISTENING,
         [](int port, string addr) {
           cout << "listening: " << port << " on addr " << addr
                << endl;
         })
      .listen(2212);
  while (continue_serving)
    this_thread::sleep_for(chrono::milliseconds(500));
}