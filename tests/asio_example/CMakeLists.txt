cmake_minimum_required(VERSION 3.20)

add_executable(asio_example "main.cpp")

target_link_libraries(asio_example PRIVATE rain_net)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(asio_example PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(asio_example PRIVATE /W2)
else()
    message(FATAL_ERROR "Compiler is not GCC or MSVC")
endif()
