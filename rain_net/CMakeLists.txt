cmake_minimum_required(VERSION 3.20)

set(RAIN_NET "rain_net")

add_library(${RAIN_NET} INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/message.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/queue.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/connection.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/client.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/server.hpp"
)

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/asio-1.28.0/include/")

target_include_directories(${RAIN_NET} INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/asio-1.28.0/include/"
)
