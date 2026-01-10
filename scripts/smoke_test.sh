#!/bin/bash
# EdgeOS Black-Box Integration Test
# IEMS vs DeviceSimulator (The Battle of Omnis)

IEMS="./build/products/iems/bin/iems"
SIM="./build/products/device_simulator/bin/device_simulator"
VBUS="/tmp/vbus0.sock"

rm -f $VBUS iems.db*

echo "--- Starting Omnis Integration Battle ---"

# 1. 启动 IEMS 并将其置于后台
# 在真实场景中，IEMS 会监听虚拟总线
# 注意：目前 iems/main.c 还需要支持 UDS 监听，我马上改
$IEMS &
IEMS_PID=$!

sleep 1

# 2. 启动模拟器运行剧本
$SIM $VBUS
SIM_RET=$?

# 3. 清理并判定
kill $IEMS_PID 2>/dev/null
rm -f $VBUS

if [ $SIM_RET -eq 0 ]; then
    echo "--- [PASS] Omnis Integration Successful ---"
    exit 0
else
    echo "--- [FAIL] Omnis Integration Failed ---"
    exit 1
fi