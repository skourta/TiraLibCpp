# TiraLibCPP: A server for tiramisu functions to build and execute schedules from strings

This library provides a server for tiramisu functions to build and execute schedules from strings. It is mainly used for the TirLib Python library.

## Installation

To install the library, you need to have the following dependencies installed:

- CMake
- A C++ compiler
- The Tiramisu library

Then, you can install the library by running the following commands:

```bash
git clone
cd TiraLibCPP
mkdir build
cd build
cmake .. -DTIRAMISU_INSTALL=/path/to/tiramisu -DCMAKE_INSTALL_PREFIX=/path/to/install
make install
```

## Tests
To build the tests, you need to pass the `-DBUILD_TESTS=ON` flag to the `cmake` command. Then, you can run the tests by running the following command:

```bash
cd build
cmake .. -DTIRAMISU_INSTALL=/path/to/tiramisu -DCMAKE_INSTALL_PREFIX=/path/to/install -DBUILD_TESTS=ON
make
ctest -C Release --test-dir ./tests
```

