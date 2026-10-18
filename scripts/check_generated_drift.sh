#!/usr/bin/env bash
# ==============================================================================
# EdgeOS Anti-Entropy Gate: No-Contract & No-Compiler Anti-Regression Gate (ADR-0010)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

echo "🔍 [ARCH-GATE] Running No-Contract & Anti-Regression Guard (ADR-0010)..."

VIOLATIONS=0

# 1. 确保全局契约编译器与构建模块绝不回潮
echo "  [1/3] Verifying complete elimination of contract compiler..."
if [ -d "${ROOT_DIR}/tools/contract-compiler-v2" ]; then
    echo "    ❌ VIOLATION: tools/contract-compiler-v2 directory re-introduced!"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

if [ -f "${ROOT_DIR}/cmake/EdgeContract.cmake" ]; then
    echo "    ❌ VIOLATION: cmake/EdgeContract.cmake re-introduced!"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

if [ -f "${ROOT_DIR}/cmake/BuildContractCompiler.cmake" ]; then
    echo "    ❌ VIOLATION: cmake/BuildContractCompiler.cmake re-introduced!"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

# 2. 确保核心产品中零 contracts.h 污染
echo "  [2/3] Checking core products for forbidden contracts.h contamination..."
PRODUCTS=("cmd/dcu" "cmd/iems" "cmd/smart_box")
for p in "${PRODUCTS[@]}"; do
    if [ -d "${ROOT_DIR}/${p}" ]; then
        if grep -rn --include="*.c" --include="*.h" '#include.*"contracts\.h"' "${ROOT_DIR}/${p}" >/dev/null 2>&1; then
            echo "    ❌ VIOLATION: Product ${p} contains forbidden '#include \"contracts.h\"'!"
            grep -rn --include="*.c" --include="*.h" '#include.*"contracts\.h"' "${ROOT_DIR}/${p}"
            VIOLATIONS=$((VIOLATIONS + 1))
        fi
        if grep -rn "edge_add_contract" "${ROOT_DIR}/${p}/CMakeLists.txt" >/dev/null 2>&1; then
            echo "    ❌ VIOLATION: Product ${p}/CMakeLists.txt calls forbidden 'edge_add_contract'!"
            VIOLATIONS=$((VIOLATIONS + 1))
        fi
    fi
done

# 3. 确保核心模块 modules/ 零 contracts 依赖
echo "  [3/3] Verifying modules/ plane isolation from contracts..."
if grep -rn --include="*.c" --include="*.h" '#include.*"contracts\.h"' "${ROOT_DIR}/modules" >/dev/null 2>&1; then
    echo "    ❌ VIOLATION: modules/ contains forbidden '#include \"contracts.h\"'!"
    grep -rn --include="*.c" --include="*.h" '#include.*"contracts\.h"' "${ROOT_DIR}/modules"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

if [ "${VIOLATIONS}" -gt 0 ]; then
    echo "❌ [ARCH-GATE FAILED] Detected ${VIOLATIONS} contract regression violation(s)!"
    exit 1
fi

echo "✅ [ARCH-GATE PASS] No-Contract anti-regression gate verified: Zero contract compiler & zero contracts.h contamination."
exit 0
