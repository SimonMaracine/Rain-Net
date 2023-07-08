cmake_minimum_required(VERSION 3.20)

option(RAIN_NET_BUILD_TESTS "Enable building tests or not" OFF)

project("Rain-Net" LANGUAGES CXX)

add_subdirectory("rain_net")

if(RAIN_NET_BUILD_TESTS)
    add_subdirectory(tests)
endif()

message(STATUS "Rain-Net: Building tests: ${RAIN_NET_BUILD_TESTS}")
