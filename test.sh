#!/bin/bash
set -e

# ==============================================================================
# Omnis Runner: Unified Test & Simulation Environment
# ==============================================================================

# 1. Load shared environment
source "$(dirname "$0")/scripts/setup_env.sh"

# 默认参数
MODE="test" # 默认为 test 模式
PRODUCT="iems"
LOG_DIR="$PROJECT_ROOT/build/testing_logs"

usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --test      Run automated batch tests (CI mode)"
    echo "  --server    Launch persistent simulation environment (Dev mode)"
    echo "  --product   Target product (default: iems)"
    exit 1
}

# 解析参数
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --test) MODE="test" ;;
        --server) MODE="server" ;;
        --product) PRODUCT="$2"; shift ;;
        -h|--help) usage ;;
        *) echo "Unknown parameter: $1"; usage ;;
    esac
    shift
done

mkdir -p "$LOG_DIR"

# 检查二进制文件
BIN_PATH="$PROJECT_ROOT/build/products/$PRODUCT/bin/$PRODUCT"
if [ ! -f "$BIN_PATH" ]; then
    echo "⚠️ Warning: Binary not found at $BIN_PATH."
    echo ">> Attempting to build $PRODUCT first..."
    ./build.sh
fi

if [ "$MODE" == "test" ]; then
    echo "=========================================================="
    echo "🚀 MODE: TEST (Automated Full Chain Verification)"
    echo "=========================================================="
    
    echo ">> [1/4] Running C Unit Tests (ctest)..."
    cd "$PROJECT_ROOT/build"
    ctest --output-on-failure -j$(nproc)

    echo ""
    echo ">> [2/4] Running Go Compiler Tests..."
    cd "$PROJECT_ROOT/tools/contract-compiler"
    go test ./...

    echo ""
    echo ">> [3/4] Running Python Component Tests..."
    cd "$PROJECT_ROOT/tools/matrix"
    pytest -v --tb=short tests/test_core.py tests/test_modbus.py tests/test_cli.py

    echo ""
    echo ">> [4/4] Running SDD Algorithm Verifier..."
    export MATRIX_PRODUCT="$PRODUCT"
    if ! pytest -v --tb=short tests/test_sdd_rms.py; then
        echo "❌ SDD Test Failed. Dumping EdgeC Evidence Logs..."
        # Note: Pytest matrix fixture logs are in build/products/$PRODUCT/matrix_reports/...
        # But we also have build/matrix_edgec.log as a temp
        tail -n 100 "$PROJECT_ROOT/build/matrix_edgec.log" 2>/dev/null || true
        exit 1
    fi

    echo ""
    echo "✅ All Verification Tiers Passed Successfully!"

else
    echo "=========================================================="
    echo "🌐 MODE: SERVER (Persistent Simulation Environment)"
    echo "=========================================================="
    echo "Product: $PRODUCT"
    echo "Log Directory: $LOG_DIR"
    echo "----------------------------------------------------------"

    # 1. 启动 Matrix Server (仿真引擎)
    echo ">> Starting Matrix Simulation Server..."
    export MATRIX_PRODUCT="$PRODUCT"
    python3 -m tools.matrix.server --schema "$PROJECT_ROOT/build/products/$PRODUCT/generated/omnis_manifest.json" > "$LOG_DIR/matrix_server.log" 2>&1 &
    MATRIX_PID=$!

    # 2. 等待总线就绪
    sleep 2

    # 3. 启动 EdgeC 实例
    echo ">> Starting EdgeC Instance ($PRODUCT)..."
    export EDGE_REST_PORT=9091
    export OMNIS_VBUS_PATH="/tmp/vbus0.sock"
    "$BIN_PATH" > "$LOG_DIR/edgec.log" 2>&1 &
    EDGEC_PID=$!

    echo "----------------------------------------------------------"
    echo "📡 Simulation Active!"
    echo "🔗 REST API: http://127.0.0.1:9091"
    echo "📁 Logs: $LOG_DIR"
    echo "⌨️ Press Ctrl+C to stop all services."
    echo "----------------------------------------------------------"

    # 4. 自动拉起交互式外壳 (Matrix CLI)
    echo ">> Launching Matrix-CLI Console..."
    # Ensure it's reachable from PROJECT_ROOT
    python3 "$PROJECT_ROOT/tools/matrix/matrix_cli.py" 127.0.0.1 9999

    # 清理函数
    cleanup() {
        echo ""
        echo "🛑 Shutting down..."
        kill $EDGEC_PID 2>/dev/null || true
        kill $MATRIX_PID 2>/dev/null || true
        exit 0
    }
    trap cleanup SIGINT SIGTERM

    wait
fi
