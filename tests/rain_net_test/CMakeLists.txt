cmake_minimum_required(VERSION 3.20)

add_executable(rain_net_test "main.cpp")

target_link_libraries(rain_net_test PRIVATE rain_net)

set_warnings(rain_net_test)
