cmake_minimum_required(VERSION 3.20)

add_library(rain_net
    "include/rain_net/client.hpp"
    "include/rain_net/connection.hpp"
    "include/rain_net/message.hpp"
    "include/rain_net/queue.hpp"
    "include/rain_net/server.hpp"
    "include/rain_net/version.hpp"
    "src/client.cpp"
    "src/connection.cpp"
    "src/message.cpp"
    "src/server.cpp"
)

target_include_directories(rain_net PUBLIC
    "include"
    "dependencies/asio-1.28.0/include"
)

set_warnings_and_standard(rain_net)
