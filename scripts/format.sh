#!/bin/bash
# EdgeOS 精准格式化脚本

# 1. 定义需要包含的目录
INCLUDE_DIRS=("src" "include" "cmd" "tests" "algo" "examples" "contracts")

# 2. 检查 clang-format 是否存在
if ! command -v clang-format >/dev/null 2>&1; then
    echo "[Format] clang-format not found. Skipping formatting."
    exit 0
fi

# 3. 遍历并执行格式化
for dir in "${INCLUDE_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "[Format] Processing directory: $dir..."
        find "$dir" -type f \( -name "*.c" -o -name "*.h" \) -not -path "*/generated/*" -exec clang-format -i {} +
    fi
done

echo "[Format] Done. All core sources are standardized."