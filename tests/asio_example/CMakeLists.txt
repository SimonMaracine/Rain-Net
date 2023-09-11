cmake_minimum_required(VERSION 3.20)

add_executable(asio_example "main.cpp")

target_link_libraries(asio_example PRIVATE rain_net)

set_warnings_and_standard(asio_example)
