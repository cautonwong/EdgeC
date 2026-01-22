#!/bin/bash

PROD="iems"
GEN_DIR="build/products/$PROD/generated"
BIN_DIR="build/products/$PROD/bin"

# 1. 物理锚点设置
mkdir -p .local/tmp
export OMNIS_VBUS_PATH="$PWD/.local/tmp/vbus0.sock"

stop_all() {
    pkill -9 -f "$BIN_DIR/$PROD" > /dev/null 2>&1
    pkill -9 -f "tools/matrix/server.py" > /dev/null 2>&1
    pkill -9 -f "modbus.py" > /dev/null 2>&1
    rm -f "$OMNIS_VBUS_PATH"
}

run_omnis() {
    # 自检：确保编译器已经产出了“数字孪生”清单
    if [ ! -f "$GEN_DIR/omnis_manifest.json" ]; then
        echo "❌ FATAL: Manifest not found! Please run ./build.sh first."
        exit 1
    fi

    echo "🚀 Launching Omnis Full-Chain Simulation..."
    echo "📍 Physical Anchor: $OMNIS_VBUS_PATH"
    
    stop_all
    
    # 清理旧日志，确保当前看到的是最新运行结果
    rm -f build/matrix_server.log build/iems_engine.log

    # 2. 启动 Matrix Server (数据中继器 + 仿真 Actor)
    export PYTHONPATH=$PWD
    .venv/bin/python3 tools/matrix/server.py --product $PROD --manifest "$GEN_DIR/omnis_manifest.json" > build/matrix_server.log 2>&1 &
    
    # 3. 启动 IEMS 内核 (带元数据自加载能力)
    export LD_LIBRARY_PATH=$PWD/.local/linux-x86_64/lib:$LD_LIBRARY_PATH
    sleep 2 # 等待 Matrix 创建物理 Socket
    
    echo "📦 Starting Engine..."
    $BIN_DIR/$PROD > build/iems_engine.log 2>&1 &
    
    # 4. 启动 UI (TUI 监控面板)
    sleep 1 # 等待 Engine 创建发现 Socket
    echo "🖥️  Launching UI Dashboard..."
    $BIN_DIR/$PROD-cli
}

if [ "$1" == "--omnis" ]; then
    run_omnis
    stop_all
else
    # 原有单元测试逻辑
    cd build && ctest --output-on-failure
fi