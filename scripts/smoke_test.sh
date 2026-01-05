#!/bin/bash
# EdgeOS Multi-Product Smoke Test (Data-Driven Mode V2)
# Usage: ./scripts/smoke_test.sh <product_name> [protocol]

PRODUCT=$1
TEST_PROTO=${2:-"AUTO"} # 允许外部指定协议

if [ -z "$PRODUCT" ]; then
    echo "Usage: $0 <product_name> [protocol]"
    exit 1
fi

echo "--- Starting Smoke Test for [$PRODUCT] ($TEST_PROTO) ---"

# 1. 动态参数映射
CHECK_TAG=1001
case $PRODUCT in
    iems)      API_PORT=8081;  DEF_PROTO="DLT645-2007" ;;
    dcu)       API_PORT=18081; DEF_PROTO="DLT645-2007" ;;
    evse)      API_PORT=28081; DEF_PROTO="ModbusRTU" ;;
    sg)        API_PORT=38081; DEF_PROTO="ModbusRTU" ;;
    hems)      API_PORT=28081; DEF_PROTO="ModbusRTU" ;;
    *)         echo "[ERROR] Unknown product $PRODUCT"; exit 1 ;;
esac

# 如果外部指定了协议，则覆盖默认值
[ "$TEST_PROTO" != "AUTO" ] && DEF_PROTO=$TEST_PROTO

# 2. 路径定义
BIN="./build/products/${PRODUCT}/bin/${PRODUCT}"
if [ "$PRODUCT" == "sg" ]; then BIN="./build/products/sg_el2025/bin/sg_el2025"; fi

SIM="./build/products/device_simulator/bin/device_simulator"
VPORT="/tmp/edge_smoke_${PRODUCT}.sock"
LOG_FILE="/tmp/edge_smoke_${PRODUCT}.log"

rm -f $VPORT $LOG_FILE *.db *.db-shm *.db-wal

# 3. 启动模拟器 (双协议自动识别模式)
$SIM --product $PRODUCT --port $VPORT --uds &
SIM_PID=$!

# [确定性增强] 轮询等待模拟器 Socket 就绪，移除不可靠的 sleep
echo "[WAIT] Waiting for simulator socket $VPORT..."
for i in {1..50}; do
    if [ -S "$VPORT" ]; then break; fi
    sleep 0.1
done

if [ ! -S "$VPORT" ]; then
    echo "[FAIL] Simulator failed to start socket in 5 seconds."
    kill $SIM_PID 2>/dev/null
    exit 1
fi

# 4. 运行产品 (Data-Driven Auto-Exit)
echo "[APP] Running $PRODUCT with $DEF_PROTO (waiting for auto-exit)..."
# 注意：在真实部署中，协议配置在数据库；冒烟测试通过 --port 强制绑定
$BIN --port $VPORT --smoke-test > $LOG_FILE 2>&1
APP_RET=$?

# 5. 判定
if [ $APP_RET -eq 0 ]; then
    echo "[PASS] Product exited gracefully after receiving data."
    SUCCESS=1
else
    echo "[FAIL] Product crashed or timed out (Ret: $APP_RET)."
    SUCCESS=0
fi

# 6. 清理
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