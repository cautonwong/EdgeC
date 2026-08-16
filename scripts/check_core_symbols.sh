#!/usr/bin/env bash
# ==============================================================================
# EdgeOS Anti-Entropy Gate: Check Core Undefined Symbols (nm -u)
# Ensures modules/edge_core does not depend on forbidden 3rd-party engines (sqlite3, paho, dds)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CORE_LIB="${BUILD_DIR}/modules/edge_core/libedge_core.a"

echo "🔍 [ARCH-GATE] Checking libedge_core.a undefined symbols (nm -u)..."

if [ ! -f "${CORE_LIB}" ]; then
    echo "❌ [ARCH-GATE FAIL] ${CORE_LIB} not found! Please run build first."
    exit 1
fi

UNDEFINED_SYMBOLS=$(nm -u "${CORE_LIB}" | awk '{print $NF}' | sort -u)

# 违规外部引擎符号黑名单
FORBIDDEN_PATTERNS=(
    "^sqlite3_"
    "^MQTTAsync_"
    "^MQTTClient_"
    "^dds_"
    "^ddsc_"
    "^UA_"
)

VIOLATIONS=()
for pattern in "${FORBIDDEN_PATTERNS[@]}"; do
    MATCHES=$(echo "$UNDEFINED_SYMBOLS" | grep -E "$pattern" || true)
    if [ -n "$MATCHES" ]; then
        while IFS= read -r sym; do
            VIOLATIONS+=("$sym (matched: $pattern)")
        done <<< "$MATCHES"
    fi
done

if [ ${#VIOLATIONS[@]} -gt 0 ]; then
    echo "🚨 [ARCH-GATE FAIL] Forbidden 3rd-party engine symbols found in libedge_core.a:"
    for v in "${VIOLATIONS[@]}"; do
        echo "   ❌ $v"
    done
    echo "👉 Clean Architecture Violation: edge_core must use abstract interfaces (storage_ops_t / datahub_ops_t)."
    if [ "${STRICT_CORE_GATE:-0}" = "1" ]; then
        exit 1
    fi
else
    echo "✅ [ARCH-GATE PASS] libedge_core.a is completely free of forbidden 3rd-party engine symbols (0 sqlite3/paho/dds references)."
fi
