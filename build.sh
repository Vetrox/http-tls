#!/bin/sh


mkdir -p ./build
g++ -std=c++2a src/tls.cpp src/http.cpp src/test.cpp -o build/a.out && ./build/a.out

