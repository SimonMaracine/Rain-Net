cmake_minimum_required(VERSION 3.20)

add_executable(asio_example "main.cpp")

target_link_libraries(asio_example PRIVATE asio)

set_warnings_and_standard(asio_example)

target_compile_definitions(asio_example PRIVATE "_WIN32_WINNT=0x0601")
