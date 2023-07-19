cmake_minimum_required(VERSION 3.20)

add_executable(client "client.cpp")
add_executable(server "server.cpp")

target_link_libraries(client PRIVATE rain_net)
target_link_libraries(server PRIVATE rain_net)

set_warnings(client)
set_warnings(server)
