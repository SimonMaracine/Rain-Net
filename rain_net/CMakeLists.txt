cmake_minimum_required(VERSION 3.20)

add_library(rain_net INTERFACE
    "include/rain_net/message.hpp"
    "include/rain_net/queue.hpp"
    "include/rain_net/connection.hpp"
    "include/rain_net/client.hpp"
    "include/rain_net/server.hpp"
    "include/rain_net/version.hpp"
)

target_include_directories(rain_net INTERFACE
    "include/"
    "dependencies/asio-1.28.0/include/"
)
