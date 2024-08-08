cmake_minimum_required(VERSION 3.20)

add_library(rain_net_client
    "include/rain_net/internal/server_connection.hpp"
    "include/rain_net/client.hpp"
    "src/client.cpp"
    "src/server_connection.cpp"
)

target_include_directories(rain_net_client PUBLIC "include")

target_link_libraries(rain_net_client PUBLIC rain_net_base)

configure_library(rain_net_client)
