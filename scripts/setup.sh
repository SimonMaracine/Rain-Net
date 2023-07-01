#! /bin/bash

clear

cd ..
mkdir -p build
cd build
cmake .. -D CMAKE_BUILD_TYPE=Debug -D RAIN_NET_BUILD_TESTS=ON
