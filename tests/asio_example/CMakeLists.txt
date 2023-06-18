cmake_minimum_required(VERSION 3.20)

set(TEST "asio_example")

add_executable(${TEST} "main.cpp")

target_link_libraries(${TEST} PRIVATE "rain_net")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${TEST} PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${TEST} PRIVATE /W2)
else()
    message(FATAL_ERROR "Compiler is not GCC or MSVC")
endif()
