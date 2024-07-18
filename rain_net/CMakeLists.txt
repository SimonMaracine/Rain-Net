cmake_minimum_required(VERSION 3.20)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE "dependencies/asio-1.30.2/include")

add_library(rain_net
    "include/rain_net/internal/connection.hpp"
    "include/rain_net/internal/error.hpp"
    "include/rain_net/internal/message.hpp"
    "include/rain_net/internal/queue.hpp"
    "include/rain_net/client.hpp"
    "include/rain_net/conversion.hpp"
    "include/rain_net/server.hpp"
    "include/rain_net/version.hpp"
    "src/client.cpp"
    "src/connection.cpp"
    "src/message.cpp"
    "src/server.cpp"
)

target_include_directories(rain_net PUBLIC "include")

target_link_libraries(rain_net PUBLIC asio)

set_warnings_and_standard(rain_net)

target_compile_definitions(rain_net PUBLIC
    "ASIO_NO_DEPRECATED"
    "_WIN32_WINNT=0x0601"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(rain_net PRIVATE "NDEBUG")
endif()
