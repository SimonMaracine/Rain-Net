echo off

cd ..
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DRAIN_NET_BUILD_TESTS=ON -A x64
cd ..\scripts

pause
