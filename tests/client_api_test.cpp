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

TEST_CASE("create simple socket", "[client]") {
  using namespace tp::net;
  std::mutex sync_mutex;
  SECTION("create socket") {
    tp::net::socket client;
    REQUIRE(0 == 0);
  }
}

TEST_CASE("create simple connection and handles", "[client]") {
  using namespace tp::net;
  std::vector<int> lsockets;
  auto on_listen = [&lsockets](int listening_socket) {
    lsockets.push_back(listening_socket);
  };
  std::vector<std::future<void>> mock_threads;

  SECTION("connect to should connect") {
    bool on_connection_called_back = false;
    auto on_connection = [&on_connection_called_back](int, char const *,
                                                      char const *) {
      on_connection_called_back = true;
      barrier(2);
    };
    mock_threads = mock_server(1337, on_connection, on_listen);

    tp::net::socket client;
    client.connect(1337, "127.0.0.1");
    barrier(2);
    for (auto s : lsockets)
      ::close(s);
    for (auto &e : mock_threads)
      e.get();

    REQUIRE(on_connection_called_back);
  }

  SECTION("connect to should connect and call callback properly") {
    auto on_connection = [](int, char const *, char const *) { barrier(2); };
    mock_threads = mock_server(1337, on_connection, on_listen);

    bool callback_called = false;

    tp::net::socket client;
    client.on(CONNECT, [&callback_called]() { callback_called = true; })
        .connect(1337, "127.0.0.1");

    barrier(2);
    for (auto s : lsockets)
      ::close(s);
    for (auto &e : mock_threads)
      e.get();
    REQUIRE(callback_called);
  }

  SECTION("send data after connection") {
    std::string ret_val = "";
    auto on_connection = [&ret_val](int s, char const *, char const *) {
      char data[100];
      int size = ::recv(s, data, 100, 0);
      ret_val = std::string(data, data + size);
      barrier(2);
    };
    mock_threads = mock_server(1337, on_connection, on_listen);

    bool callback_called = false;

    tp::net::socket client;
    client
        .on(CONNECT, [&callback_called, &client]() { client.write("hello"); })
        .connect(1337, "127.0.0.1")
        .on(DATA, [](const std::string) {});

    barrier(2);
    // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    for (auto s : lsockets)
      ::close(s);
    for (auto &e : mock_threads)
      e.get();
    CHECK(ret_val == "hello");
  }

  SECTION("recv data after connection") {
    std::string ret_val = "";
    auto on_connection = [&ret_val](int s, char const *, char const *) {
      std::string data = "hello";
      ::send(s, data.data(), data.size(), 0);
    };
    mock_threads = mock_server(1337, on_connection, on_listen);

    bool callback_called = false;

    tp::net::socket client;
    client
        .on(CONNECT, [&callback_called, &client]() {})

        .connect(1337, "127.0.0.1")
        .on(DATA, [&ret_val](const std::string data) {
          ret_val = data;
          barrier(2);
        });

    barrier(2);

    for (auto s : lsockets)
      ::close(s);
    for (auto &e : mock_threads)
      e.get();
    CHECK(ret_val == "hello");
  }
  SECTION("recv data after connection with callback getting socket reference") {
    std::string ret_val = "";
    auto on_connection = [&ret_val](int s, char const *, char const *) {
      std::string data = "hello";
      ::send(s, data.data(), data.size(), 0);
    };
    mock_threads = mock_server(1337, on_connection, on_listen);

    bool callback_called = false;

    tp::net::socket client;
    client
        .on(CONNECT,
            [&callback_called, &client, &ret_val](tp::net::socket &s) {
              s.on(DATA, [&ret_val](const std::string data) {
                ret_val = data;
                barrier(2);
              });
            })

        .connect(1337, "127.0.0.1");

    barrier(2);

    for (auto s : lsockets)
      ::close(s);
    for (auto &e : mock_threads)
      e.get();
    CHECK(ret_val == "hello");
  }

  SECTION("recv data after connection with callback getting socket reference "
          "and recv vector") {
    std::string ret_val = "";
    auto on_connection = [&ret_val](int s, char const *, char const *) {
      std::string data = "hello";
      ::send(s, data.data(), data.size(), 0);
    };
    mock_threads = mock_server(1337, on_connection, on_listen);

    bool callback_called = false;

    tp::net::socket client;
    client
        .on(CONNECT,
            [&callback_called, &client, &ret_val](tp::net::socket &s) {
              s.on(DATA, [&ret_val](const std::vector<char> data) {
                ret_val = std::string(data.begin(), data.end());
                barrier(2);
              });
            })

        .connect(1337, "127.0.0.1");

    barrier(2);

    for (auto s : lsockets)
      ::close(s);
    for (auto &e : mock_threads)
      e.get();
    CHECK(ret_val == "hello");
  }

  SECTION("event handler for END event") {
    std::string ret_val = "";
    auto on_connection = [&ret_val](int s, char const *, char const *) {
      std::string data = "hello";
      ::send(s, data.data(), data.size(), 0);
      ::shutdown(s, SHUT_WR); // gracefully close connection
    };
    mock_threads = mock_server(1337, on_connection, on_listen);

    bool callback_called = false;

    tp::net::socket client;
    client.on(END, [&client]() { client.end(); })
        .on(CONNECT,
            [&callback_called, &client, &ret_val](tp::net::socket &s) {
              s.on(DATA, [&ret_val](const std::vector<char> data) {
                ret_val = std::string(data.begin(), data.end());
                barrier(2);
              });
            })

        .connect(1337, "127.0.0.1");

    barrier(2);

    for (auto s : lsockets)
      ::close(s);
    for (auto &e : mock_threads)
      e.get();
    CHECK(ret_val == "hello");
  }

  SECTION("event handler for END event and send data to finish") {
    std::string ret_val = "";
    std::string ret_val2 = "";
    auto on_connection = [&ret_val, &ret_val2](int s, char const *,
                                               char const *) {
      std::string data = "hello";
      ::send(s, data.data(), data.size(), 0);
      ::shutdown(s, SHUT_WR); // gracefully close connection
      char datac[100];
      int size = ::recv(s, datac, 100, 0);
      ret_val2 = std::string(datac, datac + size);
      barrier(2);
    };
    mock_threads = mock_server(1337, on_connection, on_listen);

    bool callback_called = false;

    tp::net::socket client;
    client.on(END, [&client]() { client.end("bye"); })
        .on(CONNECT,
            [&callback_called, &client, &ret_val](tp::net::socket &s) {
              s.on(DATA, [&ret_val](const std::vector<char> data) {
                ret_val = std::string(data.begin(), data.end());
              });
            })

        .connect(1337, "127.0.0.1");

    barrier(2);

    for (auto s : lsockets)
      ::close(s);
    CHECK(ret_val == "hello");
    CHECK(ret_val2 == "bye");
    for (auto &e : mock_threads)
      e.get();
  }

  SECTION("connect to impossible will produce error") {
    bool callback_called = false;

    tp::net::socket client;
    client
        .on(ERROR,
            [&callback_called](std::string) {
              callback_called = true;
              barrier(2);
            })
        .connect(0, "127.0.0.1");
    barrier(2);
    REQUIRE(callback_called);
    for (auto &e : mock_threads)
      e.get();
  }
}
