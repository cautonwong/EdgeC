#!/bin/bash
# EdgeOS Multi-Product Smoke Test (Data-Driven Mode)
# Usage: ./scripts/smoke_test.sh <product_name>

PRODUCT=$1
if [ -z "$PRODUCT" ]; then
    echo "Usage: $0 <product_name>"
    exit 1
fi

echo "--- Starting Smoke Test for [$PRODUCT] ---"

# 1. 路径定义
BIN="./build/products/${PRODUCT}/bin/${PRODUCT}"
if [ "$PRODUCT" == "sg" ]; then BIN="./build/products/sg_el2025/bin/sg_el2025"; fi

SIM="./build/products/device_simulator/bin/device_simulator"
VPORT="/tmp/edge_smoke_${PRODUCT}.sock"
LOG_FILE="/tmp/edge_smoke_${PRODUCT}.log"

rm -f $VPORT $LOG_FILE *.db *.db-shm *.db-wal

# 2. 启动模拟器
$SIM --product $PRODUCT --port $VPORT --uds &
SIM_PID=$!
sleep 2

# 3. 运行产品 (Data-Driven Exit + Hard Timeout)
echo "[APP] Running $PRODUCT (waiting for auto-exit)..."
# 使用 timeout 命令包装，防止 shell 脚本卡死
timeout 15s $BIN --port $VPORT --smoke-test > $LOG_FILE 2>&1
APP_RET=$?

# 4. 判定
if [ $APP_RET -eq 0 ]; then
    echo "[PASS] Product exited gracefully after receiving data."
    SUCCESS=1
else
    echo "[FAIL] Product crashed or timed out (Ret: $APP_RET)."
    SUCCESS=0
fi

# 5. 清理
kill $SIM_PID 2>/dev/null
rm -f $VPORT

if [ $SUCCESS -eq 1 ]; then
    echo "[RESULT] Smoke Test for [$PRODUCT]: PASSED"
    exit 0
else
    [ -f "$LOG_FILE" ] && tail -n 20 "$LOG_FILE"
    echo "[RESULT] Smoke Test for [$PRODUCT]: FAILED"
    exit 1
fi
