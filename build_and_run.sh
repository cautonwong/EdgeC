#!/bin/bash
set -e
./scripts/format.sh
mkdir -p build
rm -rf config/iems/www
cp -rL config build/
mkdir -p build/logs
mkdir -p build/tests/logs
mkdir -p build/tests
cp -rL config build/tests/
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j4
ctest --output-on-failure
