cmake_minimum_required(VERSION 3.20)

option(BUILD_TESTS OFF "Enable building tests or not")

project("Rain-Net" LANGUAGES CXX)

add_subdirectory("rain_net")

if(${BUILD_TESTS})
    add_subdirectory("tests")
endif()
