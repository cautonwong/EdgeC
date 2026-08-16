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

WHITELIST_FILE="${ROOT_DIR}/contracts/schema/core_readonly_whitelist.txt"

CHANGED_CORE_FILES=$(git -C "${ROOT_DIR}" diff --name-only "${BASE_REF}"...HEAD -- modules/edge_core/ include/contract_data_types.h || true)

UNAUTHORIZED_CHANGES=()
if [ -n "$CHANGED_CORE_FILES" ]; then
    while IFS= read -r file; do
        [ -z "$file" ] && continue
        if [ -f "$WHITELIST_FILE" ] && grep -v '^[[:space:]]*#' "$WHITELIST_FILE" | grep -v '^[[:space:]]*$' | grep -Fxq "$file"; then
            echo "ℹ️  [ARCH-GATE INFO] Approved whitelist exemption for: $file"
        else
            UNAUTHORIZED_CHANGES+=("$file")
        fi
    done <<< "$CHANGED_CORE_FILES"
fi

if [ ${#UNAUTHORIZED_CHANGES[@]} -gt 0 ]; then
    echo "⚠️  [ARCH-GATE WARNING] The following unauthorized protected core files were modified:"
    for f in "${UNAUTHORIZED_CHANGES[@]}"; do
        echo "   ❌ $f"
    done
    echo ""
    echo "🚨 Rule R-08 Violation: 'modules/edge_core' and 'include/contract_data_types.h' are read-only assets."
    echo "👉 Modifications require explicit Architecture Committee double sign-off in contracts/schema/core_readonly_whitelist.txt."
    if [ "${STRICT_CORE_GATE:-0}" = "1" ]; then
        echo "❌ Strict mode enabled: Failing build!"
        exit 1
    fi
else
    echo "✅ [ARCH-GATE PASS] No unauthorized protected core files were modified (all diffs match approved whitelist)."
fi
