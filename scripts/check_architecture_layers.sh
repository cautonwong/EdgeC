#!/usr/bin/env bash
# ==============================================================================
# EdgeOS / EdgeC Architecture Layer Boundary & Clean Architecture Audit Gate
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "🛡️  [ARCH-GATE] Running Clean Architecture & Dependency Inversion Layer Gate..."

VIOLATIONS=0

# 1. 算法层头文件不得直接包含具体的 datahub.h 或 <uv.h>
echo "  [1/4] Checking modules/edge_algo public headers..."
for f in "${REPO_ROOT}"/modules/edge_algo/include/*.h; do
    if grep -E '^[[:space:]]*#[[:space:]]*include[[:space:]]+["<]datahub\.h[">]' "$f" >/dev/null 2>&1; then
        echo "    ❌ VIOLATION: Algorithm header $(basename "$f") includes concrete datahub.h"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
    if grep -E '^[[:space:]]*#[[:space:]]*include[[:space:]]+<uv\.h>' "$f" >/dev/null 2>&1; then
        echo "    ❌ VIOLATION: Algorithm header $(basename "$f") includes transitive <uv.h>"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

# 2. 外部公开端口 edge_port.h 不得直接包含 datahub.h
echo "  [2/4] Checking modules/edge_core/include/edge_port.h..."
if grep -E '^[[:space:]]*#[[:space:]]*include[[:space:]]+["<]datahub\.h[">]' "${REPO_ROOT}/modules/edge_core/include/edge_port.h" >/dev/null 2>&1; then
    echo "    ❌ VIOLATION: edge_port.h includes datahub.h directly"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

# 3. 核心领域与调度公开头文件不得包含 <uv.h>
echo "  [3/4] Checking core public contracts for transitive <uv.h> contamination..."
PROTECTED_HEADERS=(
    "modules/edge_core/include/arbiter.h"
    "modules/edge_core/include/edge_service.h"
    "modules/edge_core/include/edge_app_mgr.h"
    "modules/edge_io/include/driver_manager.h"
    "modules/edge_io/include/edge_agent.h"
    "modules/edge_infra/include/sqlite_service.h"
    "modules/edge_infra/include/edge_telemetry_store.h"
)
for h in "${PROTECTED_HEADERS[@]}"; do
    if [ -f "${REPO_ROOT}/${h}" ]; then
        if grep -E '^[[:space:]]*#[[:space:]]*include[[:space:]]+<uv\.h>' "${REPO_ROOT}/${h}" >/dev/null 2>&1; then
            echo "    ❌ VIOLATION: Core public header ${h} transitively includes <uv.h>"
            VIOLATIONS=$((VIOLATIONS + 1))
        fi
    fi
done

# 4. 契约代码零漂移检查
echo "  [4/4] Verifying contract drift gate..."
if ! "${SCRIPT_DIR}/check_generated_drift.sh" >/dev/null 2>&1; then
    echo "    ❌ VIOLATION: Generated contracts drift check failed"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

if [ "$VIOLATIONS" -gt 0 ]; then
    echo "❌ [ARCH-GATE FAIL] Total violations found: ${VIOLATIONS}"
    exit 1
fi

echo "✅ [ARCH-GATE PASS] All Clean Architecture and Consumer-Defined Interface boundaries verified successfully."
exit 0
