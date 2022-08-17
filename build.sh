#!/bin/sh


mkdir -p ./build
g++ src/tls.cpp src/http.cpp src/test.cpp -o build/a.out && ./build/a.out

