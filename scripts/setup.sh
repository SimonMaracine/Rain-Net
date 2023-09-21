#! /bin/bash

clear
cd ..
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DRAIN_NET_BUILD_TESTS=ON
