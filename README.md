# simple-cpp-sockets

[![Build Status](https://travis-ci.org/pantadeusz/simple-cpp-sockets.svg?branch=master)](https://travis-ci.org/pantadeusz/simple-cpp-sockets)

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
