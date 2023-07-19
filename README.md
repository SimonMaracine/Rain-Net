# Rain Net

It is a bare-bones networking library for use in any type of game that requires communication with a
server over the reliable and connection-oriented **TCP** protocol.

It is used to build both the client and the server side.

If you use CMake in your project, then that's good! It's best to add this repository as a submodule:

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

My intention is to continue working on this project and make it at least usable for small, hobby
applications.
