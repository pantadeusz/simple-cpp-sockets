# simple-cpp-sockets

[![Build Status](https://travis-ci.org/pantadeusz/simple-cpp-sockets.svg?branch=master)](https://travis-ci.org/pantadeusz/simple-cpp-sockets)

verry simple c++ sockets wrapper example

## Compilation of the library

```bash
cmake -Bbuild -H.
cmake --build build
cmake --build build --target package
sudo dpkg -i build/*.deb
```

Examples can be compiled in this way (after library is installed):

```bash
g++ -std=c++17 `pkg-config tepsoc --libs --cflags` sample_server.cpp
```

## Example server

The API is very similar to the NodeJS Socket API. That was my inspiration.

```c++
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
  srv.listen(2212);
  while (continue_serving)
    this_thread::sleep_for(chrono::milliseconds(500));
```

### How to compile with tepsoc? This is how

```bash
g++ -std=c++17 `pkg-config tepsoc --libs --cflags` sample_server.cpp
```

## Contribution

You can send pull requests. You agree that the submitted patches will be thoroughly (but slowly) reviewed. The license must be the same as the rest of the application.
