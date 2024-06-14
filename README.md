# Rain Net

It is a bare-bones networking library built on top of Asio, for use in any type of game that requires
communication with a server over the reliable and connection-oriented **TCP** protocol.

It is used to build both the client and the server side. Check out the header files for some
documentation and see the `client_server` example from `tests`.

It requires `C++17`. I tested it on `GCC 13.2` and `MSVC 19.36`.

## Usage

Add this repository as a git submodule:

```text
git submodule add https://github.com/SimonMaracine/Rain-Net <path/to/submodule>
```

and then to write this in `CMakeLists.txt`:

```cmake
add_subdirectory(<path/to/submodule>)
target_link_libraries(<target> PRIVATE rain_net)
```

To build the tests, set this variable before `add_subdirectory(...)`:

```cmake
set(RAIN_NET_BUILD_TESTS ON)
```

Development happens on the dev branch.
