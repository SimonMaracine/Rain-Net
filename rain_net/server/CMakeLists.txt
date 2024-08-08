cmake_minimum_required(VERSION 3.20)

add_library(rain_net_server
    "include/rain_net/internal/client_connection.hpp"
    "include/rain_net/internal/pool_clients.hpp"
    "include/rain_net/server.hpp"
    "src/client_connection.cpp"
    "src/pool_clients.cpp"
    "src/server.cpp"
)

target_include_directories(rain_net_server PUBLIC "include")

target_link_libraries(rain_net_server PUBLIC rain_net_base)

configure_library(rain_net_server)
