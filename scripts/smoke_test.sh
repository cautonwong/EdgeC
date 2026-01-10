#!/bin/bash
# EdgeOS Product-Level Smoke Test
# Usage: ./scripts/smoke_test.sh

BIN="./build/products/iems/bin/iems"

if [ ! -f "$BIN" ]; then
    echo "[ERROR] IEMS binary not found. Please build the project first."
    exit 1
fi

echo "--- Starting IEMS Smoke Test ---"
# 设置本地动态库路径
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/.local/linux-x86_64/lib

# 运行冒烟测试，设置 10s 的 shell 超时以防万一
timeout 10s $BIN --smoke-test
RET=$?

if [ $RET -eq 0 ]; then
    echo "--- [PASS] IEMS Smoke Test Successful ---"
    exit 0
else
    echo "--- [FAIL] IEMS Smoke Test Failed with code $RET ---"
    exit $RET
fi
