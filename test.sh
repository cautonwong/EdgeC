#!/bin/bash
set -e

# ==============================================================================
# EdgeOS 测试入口
#   默认:  C 单元测试(ctest)
#   --omnis: 产品验收,由 test-platform 跑和判(acceptance/run.sh)
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
    echo "  --omnis [产品..] 运行产品验收 (test-platform),缺省跑 acceptance/ 下所有产品"
    echo "  --asan          使用 AddressSanitizer 构建并运行测试"
    echo "  --ubsan         使用 UndefinedBehaviorSanitizer 构建并运行测试"
    echo "  --coverage      生成代码覆盖率报告 (gcovr)"
    echo "  --only <产品>   同 --omnis <产品>"
    echo "  --help          显示此帮助信息"
    echo ""
    echo -e "${B}产品验收 (test-platform):${NC}"
    echo -e "  ${G}入口:${NC} ./acceptance/run.sh [产品]"
    echo -e "  ${G}内容:${NC} 清单/任务/用例在 acceptance/<产品>/,判据/证据/归因/门禁全在平台里"
    echo -e "  ${G}锁版本:${NC} acceptance/testplatform.sha —— 平台是独立仓库,升级 = 改这一行的一次 PR"
    echo -e "  ${G}退出码:${NC} 0 合格 / 1 不合格 / 2 测试无效 / 3 未完成 / 4 平台错误 / 5 没跑过 / 6 没有验收任务"
}

stop_all() {
    echo -e "${Y}  🧹 Nuclear Cleanup...${NC}"
    # 仅杀掉 iems 二进制程序，不杀掉包含 iems 字符串的脚本路径
    pkill -9 "^iems$" || true
    pkill -9 -f "tools/matrix/server.py" || true
    pkill -9 -f "actors/" || true
    pkill -9 -f "adc_simulator" || true
    rm -rf /tmp/vbus*.sock /tmp/v_adc*.sock /tmp/iems*.sock /tmp/matrix_adc_ctrl.sock
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
    rm -rf CMakeCache.txt CMakeFiles
    cmake -GNinja -DCOVERAGE=ON -DENABLE_ASAN=OFF -DENABLE_UBSAN=OFF ..
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
    # Force clean CMake cache and files to avoid generator mismatch or CMake Re-running loop
    rm -rf CMakeCache.txt CMakeFiles
    cmake -GNinja -DENABLE_ASAN=ON -DCOVERAGE=OFF -DENABLE_UBSAN=OFF ..
    ninja
    
    echo -e "${P}🧪 Running tests with ASAN...${NC}"
    ASAN_OPTIONS=detect_leaks=1 ctest --output-on-failure
    
    echo -e "${G}✨ ASAN memory sweep completed cleanly!${NC}"
    cd ..
}

run_ubsan() {
    echo -e "${B}🔍 Step [1/1]: Building and Running with UndefinedBehaviorSanitizer (UBSAN)...${NC}"
    cd build
    rm -rf CMakeCache.txt CMakeFiles
    cmake -GNinja -DENABLE_UBSAN=ON -DCOVERAGE=OFF -DENABLE_ASAN=OFF ..
    ninja
    
    echo -e "${P}🧪 Running tests with UBSAN...${NC}"
    UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --output-on-failure
    
    echo -e "${G}✨ UBSAN undefined behavior sweep completed cleanly!${NC}"
    cd ..
}


run_acceptance() {
    echo -e "${P}🚀 产品验收 (test-platform)${NC}"
    stop_all

    # 台架、清单、用例都在 acceptance/<产品>/,结论由平台给。这里只转退出码。
    local code=0
    ./acceptance/run.sh "$@" || code=$?

    stop_all
    if [ $code -eq 0 ]; then
        echo -e "${G}✨ 验收通过${NC}"
    else
        echo -e "${R}❌ 验收未通过(门禁退出码 $code)${NC}"
    fi
    exit $code
}

# 命令分发
case "$1" in
    --omnis)
        shift
        run_acceptance "$@"
        ;;
    --only)
        if [ -z "${2:-}" ]; then
            echo -e "${R}Error: --only requires a product name (e.g. dcu)${NC}"
            exit 1
        fi
        run_acceptance "$2"
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
    --ubsan)
        run_ubsan
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
