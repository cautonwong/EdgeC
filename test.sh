#!/bin/bash
# ==============================================================================
# EdgeC Omnis Unified Test & Simulation Runner
# ==============================================================================
set -e

PROJECT_ROOT=$(pwd)
PRODUCT="iems"
BIN_DIR="$PROJECT_ROOT/build/products/$PRODUCT/bin"
export PYTHONPATH=$PYTHONPATH:$PROJECT_ROOT

usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --omnis     [Recommended] Launch full interactive simulation (Server + Engine + UI)"
    echo "  --unit      Run C unit tests only (cmocka)"
    echo "  --product   Specify product (default: iems)"
    echo "  --clean     Stop all running simulation processes"
    exit 1
}

stop_all() {
    echo "🛑 Stopping all simulation processes..."
    pkill -9 -f "tools/matrix" || true
    pkill -9 -f "$PRODUCT" || true
    rm -f /tmp/*.sock /tmp/*.tmp
}

if [[ "$1" == "--clean" ]]; then
    stop_all
    exit 0
fi

# 处理参数
MODE="omnis"
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --omnis) MODE="omnis" ;;
        --unit) MODE="unit" ;;
        --product) PRODUCT="$2"; BIN_DIR="$PROJECT_ROOT/build/products/$PRODUCT/bin"; shift ;;
        -h|--help) usage ;;
    esac
    shift
done

if [ "$MODE" == "unit" ]; then
    echo ">> Running C Unit Tests..."
    cd "$PROJECT_ROOT/build"
    ctest --output-on-failure
    exit 0
fi

# --- OMNIS 交互式仿真模式 ---
echo "=========================================================="
echo "🚀 Starting OMNIS Full-Chain Simulation for [$PRODUCT]"
echo "=========================================================="

# 1. 清理旧进程
stop_all

# 2. 启动 Matrix Server (物理世界模拟器)
echo ">> [1/3] Launching Matrix Server (World: cmd/$PRODUCT/world.yaml)..."
python3 tools/matrix/server.py --product "$PRODUCT" > build/matrix_server.log 2>&1 &
sleep 3

# 3. 启动 EdgeC 主引擎 (带入虚拟路径)
echo ">> [2/3] Launching EdgeOS Core Engine..."
export OMNIS_VBUS_PATH="/tmp/vbus0.sock"
export OMNIS_ADC_PATH="/tmp/v_adc_sim.sock"
export LD_LIBRARY_PATH="$PROJECT_ROOT/.local/linux-x86_64/lib"

"$BIN_DIR/$PRODUCT" > build/iems_engine.log 2>&1 &
ENGINE_PID=$!
sleep 2

# 4. 启动 UI 面板 (交互终端)
echo ">> [3/3] Launching UI Dashboard Console..."
echo "----------------------------------------------------------"
echo "Tips: Use F1/F2/F3 to switch pages, 'Q' to quit."
echo "----------------------------------------------------------"
sleep 1

# 运行 UI (这会接管当前终端)
"$BIN_DIR/$PRODUCT-cli"

# 退出 UI 后自动清理
stop_all
echo "🏁 Simulation finished."
