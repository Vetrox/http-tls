#!/bin/sh


mkdir -p ./build
g++ -g -std=c++2a src/SHA256.cpp src/rsa.cpp src/numutils.cpp src/certparser.cpp src/tls.cpp src/http.cpp src/test.cpp -O3 -o build/a.out && ./build/a.out

