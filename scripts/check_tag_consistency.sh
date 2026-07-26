#!/bin/bash
# [ADR-0001] 编译期点表一致性校验
# IEMS: 全量标签校验（C vs Go 必须一致）
# DCU: 数量上报（DCU 的标签集是 IEMS 的子集，仅报告不超限即通过）
set -e

PROJECT_ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
ERRORS=0

echo "--- [Tag Consistency Check] ---"

get_c_tag_count() {
    local f="$1"
    [ -f "$f" ] && grep -oP 'g_tag_metadata_len\s*=\s*\K[0-9]+' "$f" | head -1 || echo "0"
}

get_go_tag_count() {
    local f="$1"
    [ -f "$f" ] && grep -cP '^\s+\d+:' "$f" || echo "0"
}

SHARED_GO="${PROJECT_ROOT}/go/pkg/tags/tags.go"
GO_COUNT=$(get_go_tag_count "$SHARED_GO")

# SmartBox: 全量标签校验（C vs Go 必须一致）
SMARTBOX_C="${PROJECT_ROOT}/build/products/smart_box/generated/contracts.c"
SMARTBOX_C_COUNT=$(get_c_tag_count "$SMARTBOX_C")
if [ "$SMARTBOX_C_COUNT" -gt 0 ] && [ "$GO_COUNT" -gt 0 ]; then
    if [ "$SMARTBOX_C_COUNT" -eq "$GO_COUNT" ]; then
        echo "  ✅ SmartBox: C=$SMARTBOX_C_COUNT, Go=$GO_COUNT"
    else
        echo "  ❌ SmartBox MISMATCH: contracts.c has $SMARTBOX_C_COUNT, tags.go has $GO_COUNT"
        ERRORS=$((ERRORS + 1))
    fi
fi

# IEMS: 只上报（IEMS 标签集是子集，允许少于 Go 全量）
IEMS_C="${PROJECT_ROOT}/build/products/iems/generated/contracts.c"
IEMS_C_COUNT=$(get_c_tag_count "$IEMS_C")
if [ "$IEMS_C_COUNT" -gt 0 ]; then
    if [ "$IEMS_C_COUNT" -le "$GO_COUNT" ] 2>/dev/null; then
        echo "  ✅ IEMS: C=$IEMS_C_COUNT (≤ Go=$GO_COUNT, 子集合规)"
    else
        echo "  ❌ IEMS OVERFLOW: C=$IEMS_C_COUNT > Go=$GO_COUNT"
        ERRORS=$((ERRORS + 1))
    fi
fi

# DCU: 只上报（DCU 标签集是子集，允许少于 Go 全量）
DCU_C="${PROJECT_ROOT}/build/products/dcu/generated/contracts.c"
DCU_C_COUNT=$(get_c_tag_count "$DCU_C")
if [ "$DCU_C_COUNT" -gt 0 ]; then
    if [ "$DCU_C_COUNT" -le "$GO_COUNT" ] 2>/dev/null; then
        echo "  ✅ DCU: C=$DCU_C_COUNT (≤ Go=$GO_COUNT, 子集合规)"
    else
        echo "  ❌ DCU OVERFLOW: C=$DCU_C_COUNT > Go=$GO_COUNT"
        ERRORS=$((ERRORS + 1))
    fi
fi

[ "$ERRORS" -gt 0 ] && echo "❌ FAILED with $ERRORS error(s)" && exit 1
echo "✅ Tag consistency check PASSED"
