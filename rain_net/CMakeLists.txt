cmake_minimum_required(VERSION 3.20)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE "dependencies/asio-1.30.2/include")

function(configure_library target)
    set_warnings_and_standard(${target})

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_definitions(${target} PRIVATE "NDEBUG")
    endif()
endfunction()

add_subdirectory(base)
add_subdirectory(client)
add_subdirectory(server)
