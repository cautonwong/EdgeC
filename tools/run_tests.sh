#!/bin/bash
set -e

# 获取项目根目录绝对路径 (假设脚本位于 tools/ 目录下)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "========================================"
echo ">> Running Contract Compiler Tests (Go)"
echo "========================================"
cd "$PROJECT_ROOT/tools/contract-compiler"
go test -v ./...

echo ""
echo "========================================"
echo ">> Running Matrix Simulator Tests (Python)"
echo "========================================"

# 设置 PYTHONPATH 以支持 tools.matrix... 导入
export PYTHONPATH="$PROJECT_ROOT"

# 尝试激活虚拟环境 (如果存在)
if [ -f "$PROJECT_ROOT/.venv/bin/activate" ]; then
    source "$PROJECT_ROOT/.venv/bin/activate"
elif [ -f "$PROJECT_ROOT/venv/bin/activate" ]; then
    source "$PROJECT_ROOT/venv/bin/activate"
fi

cd "$PROJECT_ROOT/tools/matrix"
# 运行 pytest
# -v: 详细输出
# --tb=short: 失败时回溯信息精简
pytest -v --tb=short

echo ""
echo "========================================"
echo "✅ All Tools Verified Successfully!"
echo "========================================"
