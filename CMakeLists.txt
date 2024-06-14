cmake_minimum_required(VERSION 3.20)

project("Rain-Net" LANGUAGES CXX)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Variable used to control the build type")

option(RAIN_NET_BUILD_TESTS "Enable building tests or not" OFF)

function(set_warnings_and_standard target)
    if(UNIX)
        target_compile_options(${target} PRIVATE "-Wall" "-Wextra" "-Wpedantic" "-Wconversion")
    elseif(MSVC)
        target_compile_options(${target} PRIVATE "/W4")
    else()
        message(WARNING "Warnings are not enabled")
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
