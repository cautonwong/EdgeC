#!/bin/bash
set -e
./scripts/format.sh
mkdir -p build
mkdir -p build/logs
mkdir -p build/tests/logs
mkdir -p build/tests
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j4
ctest --output-on-failure
