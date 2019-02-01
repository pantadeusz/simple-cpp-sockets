# simple-cpp-sockets
verry simple c++ sockets wrapper example

## Installing testing dependencies

Just install catch2 from git source. This example works for sure with:

```bash
wget https://codeload.github.com/catchorg/Catch2/tar.gz/v2.5.0 -O v2.5.0.tar.gz
tar -xvf v2.5.0.tar.gz
cd Catch2-2.5.0
cmake -Bbuild -H. -DBUILD_TESTING=OFF
sudo cmake --build build --target install
```

## Compilation

```bash
cmake -Bbuild -H.
cmake --build build
```

