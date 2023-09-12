cmake_minimum_required(VERSION 3.20)

option(RAIN_NET_BUILD_TESTS "Enable building tests or not" OFF)

project("Rain-Net" LANGUAGES CXX)

function(set_warnings_and_standard target)
    if(UNIX)
        target_compile_options(${target} PRIVATE "-Wall" "-Wextra" "-Wpedantic")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE "/W3")
    else()
        message(WARNING "Compiler is not MSVC or platform is not supported yet, warnings are not enabled")
    endif()

    target_compile_features(${target} PRIVATE cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
endfunction()

add_subdirectory("rain_net")

if(RAIN_NET_BUILD_TESTS)
    add_subdirectory(tests)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

message(STATUS "Rain-Net: Building tests: ${RAIN_NET_BUILD_TESTS}")
