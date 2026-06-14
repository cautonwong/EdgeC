#!/bin/bash
set -e

# ==============================================================================
# EdgeOS 集成测试与全链路演习指挥中心 (Omnis Edition)
# ==============================================================================

# Load shared environment
source "$(dirname "$0")/scripts/setup_env.sh"

PROD="iems"
# ENV_PYTHON is already set by setup_env.sh

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
    echo "  --only <test>   只运行特定的测试文件或用例 (例如: cmd/iems/matrix/test_slave_flow.py)"
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
    # 仅杀掉 iems 二进制程序，不杀掉包含 iems 字符串的脚本路径
    pkill -9 "^iems$" || true
    pkill -9 -f "tools/matrix/server.py" || true
    pkill -9 -f "actors/" || true
    rm -rf /tmp/vbus*.sock /tmp/v_adc*.sock /tmp/iems*.sock
    rm -rf /dev/shm/matrix_shm_* || true
    rm -f matrix_server.log iems_run.log
}

run_unit_tests() {
    echo -e "${B}🔍 Step [1/1]: Running C Unit Tests...${NC}"
    cd build && ctest --output-on-failure
}

run_coverage() {
    echo -e "${B}🔍 Step [1/2]: Building and Running with Coverage...${NC}"
    cd build
    cmake -DCOVERAGE=ON ..
    ninja
    ctest --output-on-failure
    
    echo -e "${P}📊 Step [2/2]: Generating Coverage Report...${NC}"
    mkdir -p coverage
    if command -v gcovr >/dev/null 2>&1; then
        gcovr -r ../ --html --html-details -o coverage/index.html -e '../tests/.*' -e '../third-party/.*' --gcov-ignore-parse-errors=negative_hits.warn
        echo -e "${G}✨ Coverage report generated at build/coverage/index.html${NC}"
    else
        echo -e "${Y}⚠️ gcovr not installed. Trying to install via pip...${NC}"
        pip install gcovr
        gcovr -r ../ --html --html-details -o coverage/index.html -e '../tests/.*' -e '../third-party/.*' --gcov-ignore-parse-errors=negative_hits.warn
        echo -e "${G}✨ Coverage report generated at build/coverage/index.html${NC}"
    fi
    cd ..
}

run_asan() {
    echo -e "${B}🔍 Step [1/1]: Building and Running with AddressSanitizer (ASAN)...${NC}"
    cd build
    # Force clean CMake cache to avoid CMake Re-running loop
    rm -f CMakeCache.txt
    cmake -DENABLE_ASAN=ON -DCOVERAGE=OFF ..
    ninja
    
    echo -e "${P}🧪 Running tests with ASAN...${NC}"
    ASAN_OPTIONS=detect_leaks=1 ctest --output-on-failure
    
    echo -e "${G}✨ ASAN memory sweep completed cleanly!${NC}"
    cd ..
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

    # 2. 注入环境变量 (用于 VBus 路径透传)
    eval $(grep "export OMNIS_BUS_" matrix_server.log | sed 's/  //g')
    export OMNIS_PRODUCT=$PROD
    
    # 3. 执行自动化场景验收 (由 pytest 内部拉起隔离的 EdgeC)
    echo -e "${P}🧪 Step [2/2]: Running Automated Scenarios...${NC}"
    $ENV_PYTHON tools/matrix/matrix_test.py --product $PROD $PYTEST_ARGS
    
    TEST_RES=$?
    if [ $TEST_RES -eq 0 ]; then
        echo -e "${G}✨ OMNIS INTEGRATION TEST PASSED!${NC}"
    else
        echo -e "${R}❌ OMNIS INTEGRATION TEST FAILED!${NC}"
        echo -e "${Y}--- [Tail of matrix_edgec.log] ---${NC}"
        tail -n 20 build/matrix_edgec.log
    fi
    
    stop_all
    exit $TEST_RES
}

# 命令分发
case "$1" in
    --omnis)
        run_omnis
        ;;
    --only)
        if [ -z "$2" ]; then
            echo -e "${R}Error: --only requires a test file or pattern.${NC}"
            exit 1
        fi
        # 暂时借用 run_omnis 的逻辑，但通过环境变量传递 pytest 过滤
        export PYTEST_ARGS="$2"
        run_omnis
        ;;
    --help)
        show_help
        ;;
    --coverage)
        run_coverage
        ;;
    --asan)
        run_asan
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
