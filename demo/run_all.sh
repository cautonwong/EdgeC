#!/bin/bash
# IEMS Demo 启动脚本
# 峰谷套利 + 系统检查
# 验收入口: ./acceptance/run.sh iems (覆盖采集/启动/健壮性,判据在 acceptance/iems/configs/clauses.yaml)
# 过压保护与谐波的仿真场景(cmd/iems/matrix/*.py)目前没有可运行的入口:Matrix 基座已不在测试链路里
set -e

G='\033[32m'; B='\033[34m'; Y='\033[33m'; R='\033[31m'; NC='\033[0m'
cd "$(dirname "$0")/.."

echo -e "${B}============================================${NC}"
echo -e "${B}  IEMS Demo Suite${NC}"
echo -e "${B}============================================${NC}"
echo ""

# 1. 峰谷套利
echo -e "${Y}[Demo] 峰谷套利${NC}"
echo -e "${Y}------------------------------------------------${NC}"
bash demo/peak_shaving.sh | grep -E "时间|电价|MPC|电池|:00|→|每日|每月|每年|对比"
echo ""

# 2. 系统检查
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
echo -e "${G}  IEMS Demo Suite 就绪${NC}"
echo -e "${G}============================================${NC}"
