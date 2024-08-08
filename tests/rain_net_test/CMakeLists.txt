cmake_minimum_required(VERSION 3.20)

add_executable(rain_net_test "main.cpp")

target_link_libraries(rain_net_test PRIVATE rain_net_client rain_net_server)

set_warnings_and_standard(rain_net_test)
