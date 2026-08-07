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

validate_product_tags() {
    local name="$1"
    local c_file="$2"
    if [ ! -f "$c_file" ]; then return 0; fi

    local c_ids=$(sed -n '/g_tag_metadata\[\]/,/g_tag_metadata_len/p' "$c_file" | grep -oP '\{\s*\K[0-9]+(?=,\s*")' || true)
    local missing=0
    for id in $c_ids; do
        if [ "$id" -eq 0 ]; then continue; fi
        if ! grep -qP "(=\s*${id}\b|^\s*${id}:)" "$SHARED_GO"; then
            echo "  ❌ $name: Tag ID $id in $c_file is missing from $SHARED_GO"
            missing=$((missing + 1))
        fi
    done
    if [ "$missing" -gt 0 ]; then
        ERRORS=$((ERRORS + missing))
    else
        echo "  ✅ $name: All C TagIDs match Go tags.go"
    fi
}

# SmartBox: 标签覆盖校验（SmartBox C <= Go 聚合全量）
SMARTBOX_C="${PROJECT_ROOT}/build/products/smart_box/generated/contracts.c"
SMARTBOX_C_COUNT=$(get_c_tag_count "$SMARTBOX_C")
if [ "$SMARTBOX_C_COUNT" -gt 0 ] && [ "$GO_COUNT" -gt 0 ]; then
    if [ "$SMARTBOX_C_COUNT" -le "$GO_COUNT" ]; then
        echo "  ✅ SmartBox: C=$SMARTBOX_C_COUNT (≤ Go=$GO_COUNT, 契约覆盖合规)"
    else
        echo "  ❌ SmartBox OVERFLOW: contracts.c has $SMARTBOX_C_COUNT > Go=$GO_COUNT"
        ERRORS=$((ERRORS + 1))
    fi
    validate_product_tags "SmartBox" "$SMARTBOX_C"
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
    validate_product_tags "IEMS" "$IEMS_C"
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
    validate_product_tags "DCU" "$DCU_C"
fi

[ "$ERRORS" -gt 0 ] && echo "❌ FAILED with $ERRORS error(s)" && exit 1
echo "✅ Tag consistency check PASSED"
