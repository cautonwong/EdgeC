#!/usr/bin/env bash
# ==============================================================================
# EdgeOS Anti-Entropy Gate: Check No Old src/ Tree References
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "🔍 [ARCH-GATE] Scanning codebase for illegal references to legacy 'src/' tree..."

# 1. 检查根目录是否存在遗留 src 目录
if [ -d "${ROOT_DIR}/src" ]; then
    echo "❌ [ARCH-GATE FAIL] Legacy directory '${ROOT_DIR}/src' still exists!"
    exit 1
fi

# 2. 检查所有 CMakeLists.txt 是否包含 ${CMAKE_SOURCE_DIR}/src
ILLEGAL_CMAKE_REFS=$(grep -rnE '\$\{CMAKE_SOURCE_DIR\}/src/' "${ROOT_DIR}/CMakeLists.txt" "${ROOT_DIR}/modules" "${ROOT_DIR}/cmd" "${ROOT_DIR}/tests" 2>/dev/null || true)
if [ -n "$ILLEGAL_CMAKE_REFS" ]; then
    echo "❌ [ARCH-GATE FAIL] Found illegal '\${CMAKE_SOURCE_DIR}/src/' references:"
    echo "$ILLEGAL_CMAKE_REFS"
    exit 1
fi

# 3. 检查所有 C/H 源码是否跨模块引用旧平台/主循环代码
ILLEGAL_SRC_REFS=$(grep -rnE '(src/main_loop|src/platform)' "${ROOT_DIR}/modules" "${ROOT_DIR}/include" "${ROOT_DIR}/cmd" 2>/dev/null || true)
if [ -n "$ILLEGAL_SRC_REFS" ]; then
    echo "❌ [ARCH-GATE FAIL] Found illegal cross-tree references:"
    echo "$ILLEGAL_SRC_REFS"
    exit 1
fi

echo "✅ [ARCH-GATE PASS] Zero legacy 'src/' references detected across the entire codebase."
