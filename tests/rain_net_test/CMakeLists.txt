cmake_minimum_required(VERSION 3.20)

add_executable(rain_net_test "main.cpp")

target_link_libraries(rain_net_test PRIVATE rain_net)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(rain_net_test PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(rain_net_test PRIVATE /W2)
else()
    message(FATAL_ERROR "Compiler is not GCC or MSVC")
endif()
