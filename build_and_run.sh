#!/bin/bash
set -e
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j4
ctest --output-on-failure
timeout 10s ./edge_app || true
timeout 10s ./hems || true
timeout 10s ./iems || true
timeout 10s ./dcu || true
timeout 10s ./evse || true
