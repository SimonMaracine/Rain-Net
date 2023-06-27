#! /bin/bash

clear

cd ..
mkdir -p build
cd build
cmake .. -D RAIN_NET_BUILD_TESTS=ON
