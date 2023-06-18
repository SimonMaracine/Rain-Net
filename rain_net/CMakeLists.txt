cmake_minimum_required(VERSION 3.20)

set(RAIN_NET "rain_net")

add_library(${RAIN_NET} INTERFACE
    "include/rain_net/message.hpp"
)

target_include_directories(${RAIN_NET} INTERFACE
    "include/"
    "dependencies/asio-1.28.0/include/"
)
