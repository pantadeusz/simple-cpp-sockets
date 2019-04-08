# simple-cpp-sockets

![Build Status](https://travis-ci.org/pantadeusz/simple-cpp-sockets.svg?branch=master)

verry simple c++ sockets wrapper example

## Compilation

```bash
cmake -Bbuild -H.
cmake --build build
```

Examples can be compiled in this way:

```bash
export LD_LIBRARY_PATH=./build
g++ client.cpp -Lbuild -lsimplecppsockets -Iinclude -pthread -o client
g++ server.cpp -Lbuild -lsimplecppsockets -Iinclude -pthread -o server
```

## Example server

The API is very similar to the NodeJS Socket API. That was my inspiration.

```c++
  tp::net::server srv([&](tp::net::socket &s) {
    s.on(DATA, [&s, &continue_serving](std::string str) {
      std::cout << "<<" << str << std::endl;
      s.write(std::string(">>") + str);
    });
  });
  srv.on(LISTENING,
         [](int port, std::string addr) {
           std::cout << "listening: " << port << " on addr " << addr
                     << std::endl;
         })
      .listen(2212);
```

## Contribution

You can send pull requests. You agree that the submitted patches will be thoroughly (but slowly) reviewed. The license must be the same as the rest of the application.
