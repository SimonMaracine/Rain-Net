cmake_minimum_required(VERSION 3.20)

add_executable(client "client.cpp")
add_executable(server "server.cpp")

target_link_libraries(client PRIVATE rain_net)
target_link_libraries(server PRIVATE rain_net)

set_warnings_and_standard(client)
set_warnings_and_standard(server)

if(UNIX AND RAIN_NET_ASAN)
    target_compile_options(client PRIVATE "-fsanitize=address" "-fsanitize=undefined" "-g" "-fno-omit-frame-pointer")
    target_link_options(client PRIVATE "-fsanitize=address" "-fsanitize=undefined")

    target_compile_options(server PRIVATE "-fsanitize=address" "-fsanitize=undefined" "-g" "-fno-omit-frame-pointer")
    target_link_options(server PRIVATE "-fsanitize=address" "-fsanitize=undefined")
endif()
