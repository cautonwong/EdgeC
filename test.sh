#!/bin/bash
set -e

# ==============================================================================
# Omnis Runner: Unified Test & Simulation Environment
# ==============================================================================

# 1. 加载共享环境
source "$(dirname "$0")/scripts/setup_env.sh"

# 默认参数
MODE="test" 
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

if [ "$MODE" == "test" ]; then
    echo "=========================================================="
    echo "🚀 MODE: TEST (Automated Full Chain Verification)"
    echo "=========================================================="
    
    # 1. 运行 C 单元测试
    echo ">> [1/4] Running C Unit Tests..."
    cd "$PROJECT_ROOT/build"
    ctest --output-on-failure -j$(nproc)

    # 2. 运行 Go 编译器测试
    echo ">> [2/4] Running Go Compiler Tests..."
    cd "$PROJECT_ROOT/tools/contract-compiler"
    go test ./...

    # 3. 运行 Python 业务测试
    echo ""
    echo ">> [3/4] Running Business Integration Tests for $PRODUCT..."
    cd "$PROJECT_ROOT"
    export MATRIX_PRODUCT="$PRODUCT"
    # 核心：使用 -p no:xdist 显式禁止并行，保护物理 UDS 路径
    "$ENV_PYTHON" -m pytest -v --tb=short -p no:xdist "cmd/$PRODUCT/matrix"

    # 4. 运行 SDD 算法验证
    echo ""
    echo ">> [4/4] Running SDD Algorithm Verifier..."
    "$ENV_PYTHON" -m pytest -v --tb=short "tools/matrix/tests/test_sdd_rms.py"

    echo ""
    echo "✅ All Verification Tiers Passed!"

else
    # SERVER 模式逻辑保持不变...
    echo "=========================================================="
    echo "🌐 MODE: SERVER"
    echo "=========================================================="
    export MATRIX_PRODUCT="$PRODUCT"
    python3 -m tools.matrix.server --schema "$PROJECT_ROOT/build/products/$PRODUCT/generated/omnis_manifest.json" > "$LOG_DIR/matrix_server.log" 2>&1 &
    MATRIX_PID=$!
    sleep 2
    export EDGE_REST_PORT=9091
    export OMNIS_VBUS_PATH="/tmp/vbus0.sock"
    "$PROJECT_ROOT/build/products/$PRODUCT/bin/$PRODUCT" > "$LOG_DIR/edgec.log" 2>&1 &
    EDGEC_PID=$!
    echo ">> Launching Matrix-CLI Console..."
    python3 "$PROJECT_ROOT/tools/matrix/matrix_cli.py" 127.0.0.1 9999
    cleanup() { kill $EDGEC_PID $MATRIX_PID 2>/dev/null || true; exit 0; }
    trap cleanup SIGINT SIGTERM
    wait
fi