#!/usr/bin/env bash
# ==============================================================================
# EdgeOS Anti-Entropy Gate: Check Read-Only Core Violations
# ==============================================================================
set -euo pipefail

BASE_REF="${1:-origin/main}"
if ! git rev-parse --verify "$BASE_REF" >/dev/null 2>&1; then
    BASE_REF="HEAD~1"
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "🔍 Checking PR diff against ${BASE_REF} for protected core modifications..."

CHANGED_CORE_FILES=$(git -C "${ROOT_DIR}" diff --name-only "${BASE_REF}"...HEAD -- modules/edge_core/ include/contract_data_types.h || true)

if [ -n "$CHANGED_CORE_FILES" ]; then
    echo "⚠️  [ARCH-GATE WARNING] The following protected core files were modified:"
    echo "$CHANGED_CORE_FILES"
    echo ""
    echo "🚨 Rule R-08 Violation: 'modules/edge_core' and 'include/contract_data_types.h' are read-only assets."
    echo "👉 Modifications require explicit Architecture Committee double sign-off."
    if [ "${STRICT_CORE_GATE:-0}" = "1" ]; then
        echo "❌ Strict mode enabled: Failing build!"
        exit 1
    fi
else
    echo "✅ [ARCH-GATE PASS] No protected core files were modified."
fi
