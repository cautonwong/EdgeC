#!/bin/bash
set -e

# ==============================================================================
# EdgeOS 集成测试与全链路演习指挥中心 (Omnis Edition)
# ==============================================================================

PROD="iems"
PROJECT_ROOT=$(pwd)
export PYTHONPATH=$PROJECT_ROOT
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PROJECT_ROOT/.local/linux-x86_64/lib
ENV_PYTHON=".venv/bin/python3"

# 颜色定义
G='\033[32m' # Green
B='\033[34m' # Blue
P='\033[35m' # Purple
Y='\033[33m' # Yellow
R='\033[31m' # Red
NC='\033[0m' # No Color

show_help() {
    echo -e "${B}Usage:${NC} $0 [option]"
    echo ""
    echo -e "${B}Options:${NC}"
    echo "  (none)          运行标准 C 语言单元测试 (cmocka)"
    echo "  --omnis         运行全链路自动化集成测试 (Matrix Plane)"
    echo "  --help          显示此帮助信息"
    echo ""
    echo -e "${B}Interactive Debugging Tools (演习与调试工具):${NC}"
    echo -e "  ${G}1. Matrix Server:${NC}  仿真基座，启动虚拟总线和 Actor"
    echo "     命令: $ENV_PYTHON tools/matrix/server.py --product $PROD"
    echo ""
    echo -e "  ${G}2. Matrix CLI:${NC}     上帝视角控制台，支持 Tab 补全和契约透视"
    echo "     命令: $ENV_PYTHON tools/matrix/matrix_cli.py --product $PROD"
    echo "     用法: (omnis) set METER_MAIN VOLTAGE_PHASE_A 230.5"
    echo "           (omnis) inspect METER_MAIN"
    echo ""
    echo -e "  ${G}3. IEMS CLI:${NC}       边缘侧 ncurses 监控面板，查看实时 DataHub 状态"
    echo "     命令: ./build/products/$PROD/bin/iems-cli"
    echo ""
    echo -e "${B}Advanced Integration Scenarios (场景驱动测试):${NC}"
    echo -e "  运行特定 YAML 场景: "
    echo "     $ENV_PYTHON tools/matrix/matrix_test.py --product $PROD --scenario cmd/iems/scenarios/fault_test.yaml"
}

stop_all() {
    echo -e "${Y}  🧹 Nuclear Cleanup...${NC}"
    pkill -9 -f "$PROD" || true
    pkill -9 -f "tools/matrix" || true
    pkill -9 -f "actors/" || true
    rm -rf /tmp/vbus*.sock /tmp/v_adc*.sock /tmp/iems*.sock
    rm -rf /dev/shm/matrix_shm_* || true
    rm -f matrix_server.log iems_run.log
}

run_unit_tests() {
    echo -e "${B}🔍 Step [1/1]: Running C Unit Tests...${NC}"
    cd build && ctest --output-on-failure
}

run_omnis() {
    echo -e "${P}🚀 Step [1/3]: Launching Matrix Plane...${NC}"
    stop_all
    
    # 1. 启动仿真基座
    $ENV_PYTHON -u tools/matrix/server.py --product $PROD > matrix_server.log 2>&1 &
    
    echo -e "${Y}  ⏳ Waiting for Backplane...${NC}"
    MAX_RETRY=10
    while [ $MAX_RETRY -gt 0 ]; do
        if grep -q "Matrix is UP" matrix_server.log; then break; fi
        sleep 1
        MAX_RETRY=$((MAX_RETRY-1))
    done

    # 2. 注入环境变量并启动 IEMS
    eval $(grep "export OMNIS_BUS_" matrix_server.log | sed 's/  //g')
    export EDGE_DB_PATH="$PROJECT_ROOT/iems.db"
    
    echo -e "${P}🚀 Step [2/3]: Launching EdgeC Core ($PROD)...${NC}"
    ./build/products/$PROD/bin/$PROD > iems_run.log 2>&1 &
    IEMS_PID=$!
    sleep 3

    # 3. 执行自动化场景验收
    echo -e "${P}🧪 Step [3/3]: Running Automated Scenarios...${NC}"
    $ENV_PYTHON tools/matrix/matrix_test.py --product $PROD
    
    TEST_RES=$?
    if [ $TEST_RES -eq 0 ]; then
        echo -e "${G}✨ OMNIS INTEGRATION TEST PASSED!${NC}"
    else
        echo -e "${R}❌ OMNIS INTEGRATION TEST FAILED!${NC}"
        echo -e "${Y}--- [Tail of iems_run.log] ---${NC}"
        tail -n 20 iems_run.log
    fi
    
    stop_all
    exit $TEST_RES
}

# 命令分发
case "$1" in
    --omnis)
        run_omnis
        ;;
    --help)
        show_help
        ;;
    "")
        run_unit_tests
        ;;
    *)
        echo -e "${R}Unknown option: $1${NC}"
        show_help
        exit 1
        ;;
esac
