#!/bin/bash
# IEMS 三合一 Demo 启动脚本 (Sprint 6)
# 峰谷套利 + 过压保护 + 谐波分析
set -e

G='\033[32m'; B='\033[34m'; Y='\033[33m'; R='\033[31m'; NC='\033[0m'
cd "$(dirname "$0")/.."

echo -e "${B}============================================${NC}"
echo -e "${B}  IEMS Demo Suite — All Scenarios${NC}"
echo -e "${B}============================================${NC}"
echo ""

# 1. 峰谷套利
echo -e "${Y}[Demo 1/3] 峰谷套利${NC}"
echo -e "${Y}------------------------------------------------${NC}"
bash demo/peak_shaving.sh | grep -E "时间|电价|MPC|电池|:00|→|每日|每月|每年|对比"
echo ""

# 2. 过压保护
echo -e "${Y}[Demo 2/3] 过压保护 (Matrix)${NC}"
echo -e "${Y}------------------------------------------------${NC}"
echo "  依赖: Matrix 仿真环境 + IEMS 二进制"
echo "  命令: .venv/bin/python3 -m pytest cmd/iems/matrix/test_high_freq.py -v -k test_adc_to_protection"
echo "  预期: RMS 从 220V→278V → Reflex 保护触发 → SOLAR_RELAY 断开"
echo ""

# 3. 谐波分析
echo -e "${Y}[Demo 3/3] 谐波分析 (Matrix)${NC}"
echo -e "${Y}------------------------------------------------${NC}"
echo "  依赖: Matrix 仿真环境 + IEMS 二进制"
echo "  命令: .venv/bin/python3 -m pytest cmd/iems/matrix/test_high_freq.py -v -k test_harmonic"
echo "  预期: 10% 3次谐波注入 → RMS 从 220V→221.1V 准确计算"
echo ""

# 4. 快速验证
echo -e "${Y}[Verify] 系统检查${NC}"
echo -e "${Y}------------------------------------------------${NC}"
echo -n "  C 测试: "
ctest --test-dir build --output-on-failure -R "test_version|test_shmring|test_datahub" 2>&1 | tail -1 | grep -q "passed" && echo -e "${G}PASS${NC}" || echo -e "${R}FAIL${NC}"
echo -n "  Go 编译: "
cd go && go build ./cmd/... 2>&1 && echo -e "${G}PASS${NC}" || echo -e "${R}FAIL${NC}"
cd ..
echo -n "  Tag 校验: "
bash scripts/check_tag_consistency.sh 2>&1 | tail -1 | grep -q "PASSED" && echo -e "${G}PASS${NC}" || echo -e "${R}FAIL${NC}"
echo -n "  契约 Blob: "
ls -lh build/products/iems/generated/IEMS_Controller_V5.edgec 2>/dev/null && echo -e "${G}OK${NC}" || echo -e "${R}MISSING${NC}"
echo ""

echo -e "${G}============================================${NC}"
echo -e "${G}  IEMS Demo Suite 准备就绪${NC}"
echo -e "${G}============================================${NC}"
