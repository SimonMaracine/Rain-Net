cmake_minimum_required(VERSION 3.20)

add_library(rain_net_base
    "include/rain_net/internal/connection.hpp"
    "include/rain_net/internal/error.hpp"
    "include/rain_net/internal/message.hpp"
    "include/rain_net/internal/queue.hpp"
    "include/rain_net/conversion.hpp"
    "include/rain_net/version.hpp"
    "src/connection.cpp"
    "src/message.cpp"
)

target_include_directories(rain_net_base PUBLIC "include")

target_link_libraries(rain_net_base PUBLIC asio)

target_compile_definitions(rain_net_base PUBLIC
    "ASIO_NO_DEPRECATED"
    "_WIN32_WINNT=0x0601"
)

configure_library(rain_net_base)
