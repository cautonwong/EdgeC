#!/bin/bash
set -e
./scripts/format.sh
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j4
ctest --output-on-failure
