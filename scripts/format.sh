#!/bin/bash
# 一键格式化所有源文件和头文件
find src include tests cmd -name "*.[ch]" | xargs clang-format -i
echo "✅ Codebase formatted successfully."
