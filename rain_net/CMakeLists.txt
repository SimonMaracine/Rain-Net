cmake_minimum_required(VERSION 3.20)

add_library(rain_net INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/message.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/queue.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/connection.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/client.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/server.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/rain_net/version.hpp"
)

target_include_directories(rain_net INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/asio-1.28.0/include/"
)
