#!/bin/sh


mkdir -p ./build
g++ -g -std=c++2a src/certparser.cpp src/tls.cpp src/http.cpp src/test.cpp -lcrypto -o build/a.out && ./build/a.out

