cmake_minimum_required(VERSION 3.20)

set(TEST "test")

project("test" LANGUAGES CXX)

add_executable(${TEST} "main.cpp")

target_include_directories(${TEST} PRIVATE "../src" "../dependencies/asio-1.28.0/include")  # TODO this maybe should be public

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${TEST} PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${TEST} PRIVATE /W2)
else()
    message(FATAL_ERROR "Compiler is not GCC or MSVC")
endif()
