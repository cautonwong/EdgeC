#!/bin/bash
# EdgeOS 精准格式化脚本
# 仅针对自有代码执行 clang-format，自动跳过第三方目录

# 1. 定义需要包含的目录
INCLUDE_DIRS=("src" "include" "cmd" "tests" "algo")

# 2. 遍历并执行格式化
for dir in "${INCLUDE_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "[Format] Processing directory: $dir..."
        find "$dir" -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
    fi
done

echo "[Format] Done. All core sources are standardized."