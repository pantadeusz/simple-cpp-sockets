#include <tepsoc.hpp>

#include "mock_server.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

#include <catch2/catch.hpp>

#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

using namespace tp;
using namespace tp::net;

auto barrier = [](int n) {
  static std::condition_variable barrier_cv;
  static std::mutex m;
  static int x = 0;
  int notif = 0;
  {
    std::unique_lock<std::mutex> lk(m);
    x++;
    if (x >= n)
      notif = 1;
  }
  if (notif == 1) {
    barrier_cv.notify_all();
  } else {
    std::unique_lock<std::mutex> lk(m);
    // barrier_cv.wait(lk, [n]() { return x >= n; });
    if (!(barrier_cv.wait_for(lk, std::chrono::milliseconds(1000),
                              [n]() { return x >= n; })))
      throw std::invalid_argument("BARRIER TIMED OUT!");
    x = 0;
  }
};

TEST_CASE("create simple server", "[server]") {
  using namespace tp::net;
  SECTION("create server") {
    tp::net::server server;
    REQUIRE(0 == 0);
  }
  SECTION("create server and set callback") {
    tp::net::server srv([](tp::net::socket &) {});
    REQUIRE(0 == 0);
  }

  SECTION("listen method is present") {
    tp::net::server srv([](tp::net::socket &) {});
    srv.listen(1237, "127.0.0.1");
    REQUIRE(0 == 0);
  }
  SECTION("listen on every interface method is present") {
    tp::net::server srv([](tp::net::socket &) {});
    srv.listen(1337);
    REQUIRE(0 == 0);
  }
  SECTION("on listening event is present") {
    tp::net::server srv([](tp::net::socket &) {});
    REQUIRE_NOTHROW(srv.on(LISTENING, []() {}));
  }
  SECTION("when server listens then it should call callback") {
    tp::net::server srv([](tp::net::socket &) {});
    bool ok = false;
    srv.on(LISTENING, [&ok]() { ok = true; });
    srv.listen(2221);
    REQUIRE(ok);
  }

  SECTION("we should be able to accept connection") {
    bool ok = false;
    tp::net::server srv([&](tp::net::socket &) {
      ok = true;
      barrier(2);
    });
    srv.on(LISTENING, [&ok]() {});
    srv.listen(2212);

    // connect to :D
    tp::net::socket client;
    client.on(CONNECT, [&]() { client.end(); }).connect(2212, "127.0.0.1");

    barrier(2);
    REQUIRE(ok);
  }
  SECTION("we should be able to accept connection and send data") {

    std::string result = "";
    tp::net::server srv;
    srv.on(CONNECTION,[&](tp::net::socket_p s) {
      s->write("hi from server");
      s->end();
    });
    srv.on(LISTENING, [&]() {});
    srv.listen(7755);

    // connect to :D
    tp::net::socket client;
    client.on(CONNECT, [&]() { client.end(); })
        .on(DATA,
            [&](std::string s) {
              result = s;
              client.end();
              barrier(2);
            })
        .connect(7755, "127.0.0.1");

    barrier(2);
    REQUIRE(result == "hi from server");
  }
}
