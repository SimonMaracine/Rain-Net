cmake_minimum_required(VERSION 3.20)

add_executable(client "client.cpp")
add_executable(server "server.cpp")

target_link_libraries(client PRIVATE "rain_net")
target_link_libraries(server PRIVATE "rain_net")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(client PRIVATE /W2)
    target_compile_options(server PRIVATE /W2)
else()
    message(FATAL_ERROR "Compiler is not GCC or MSVC")
endif()
