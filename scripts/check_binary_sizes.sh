#!/usr/bin/env bash
# ==============================================================================
# EdgeOS Anti-Entropy Gate: Binary & Core Footprint Size Gate (AEP-205 / AEP-207)
# Measures binary sizes under Release + strip caliber against WARN/ERROR thresholds
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "🔍 [ARCH-GATE] Running Binary Footprint Size Gate (Release+strip caliber)..."

# 1. 自校验构建模式 (Self-check Build Configuration)
BUILD_TYPE="Release"
if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    DETECTED_TYPE=$(grep -i "^CMAKE_BUILD_TYPE:STRING=" "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d'=' -f2 || true)
    if [ -n "$DETECTED_TYPE" ]; then
        BUILD_TYPE="$DETECTED_TYPE"
    fi
fi
echo "ℹ️  [SELF-CHECK] Verified Build Type: ${BUILD_TYPE} (Measurement Caliber: Release + strip)"

# 临时工作目录
TMP_DIR=$(mktemp -d /tmp/edgeos_size_gate.XXXXXX)
trap 'rm -rf "${TMP_DIR}"' EXIT

# 阈值定义 (Bytes)
# 1. libedge_core.a: 纯核心库，严禁膨胀 (Warn: 250KB, Error: 500KB)
CORE_WARN_BYTES=$((250 * 1024))
CORE_ERROR_BYTES=$((500 * 1024))

# 2. 产品二进制: IEMS / DCU / SmartBox (Warn: 5MB, Error: 8MB)
PROD_WARN_BYTES=$((5 * 1024 * 1024))
PROD_ERROR_BYTES=$((8 * 1024 * 1024))

# 检查目标配置: "文件路径|类型(core/prod)|是否必须(1/0)|Warn阈值|Error阈值"
TARGETS=(
    "${BUILD_DIR}/modules/edge_core/libedge_core.a|core|1|${CORE_WARN_BYTES}|${CORE_ERROR_BYTES}"
    "${BUILD_DIR}/products/iems/bin/iems|prod|0|${PROD_WARN_BYTES}|${PROD_ERROR_BYTES}"
    "${BUILD_DIR}/products/dcu/bin/dcu|prod|0|${PROD_WARN_BYTES}|${PROD_ERROR_BYTES}"
    "${BUILD_DIR}/products/smart_box/bin/smart_box|prod|0|${PROD_WARN_BYTES}|${PROD_ERROR_BYTES}"
)

format_size() {
    local bytes=$1
    if [ "$bytes" -ge $((1024 * 1024)) ]; then
        awk -v b="$bytes" 'BEGIN {printf "%.2f MB", b/1048576}'
    elif [ "$bytes" -ge 1024 ]; then
        awk -v b="$bytes" 'BEGIN {printf "%.1f KB", b/1024}'
    else
        echo "${bytes} B"
    fi
}

HAS_ERROR=0
HAS_WARN=0
CHECKED_COUNT=0

printf "\n%-38s | %-10s | %-10s | %-9s | %-9s | %-8s\n" "Artifact Target" "Raw Size" "Stripped" "Warn Lim" "Err Lim" "Status"
printf -- "----------------------------------------------------------------------------------------------------\n"

for target_spec in "${TARGETS[@]}"; do
    IFS="|" read -r file_path target_type is_required warn_lim err_lim <<< "$target_spec"
    
    file_name=$(basename "$file_path")
    parent_dir=$(basename "$(dirname "$file_path")")
    display_name="${parent_dir}/${file_name}"
    if [ "$target_type" = "core" ]; then
        display_name="edge_core/${file_name}"
    fi

    if [ ! -f "$file_path" ]; then
        if [ "$is_required" = "1" ]; then
            printf "%-38s | %-10s | %-10s | %-9s | %-9s | \033[0;31m%-8s\033[0m\n" "$display_name" "MISSING" "MISSING" "-" "-" "🚨 FAIL"
            echo "🚨 [ARCH-GATE FAIL] Required artifact is missing: ${file_path}"
            HAS_ERROR=1
        fi
        continue
    fi

    CHECKED_COUNT=$((CHECKED_COUNT + 1))
    raw_bytes=$(stat -c %s "$file_path")

    # 制作 stripped 副本测定真实落地体积
    tmp_file="${TMP_DIR}/${file_name}"
    cp "$file_path" "$tmp_file"
    if [[ "$file_path" == *.a ]]; then
        strip --strip-unneeded "$tmp_file" 2>/dev/null || true
    else
        strip --strip-all "$tmp_file" 2>/dev/null || true
    fi
    stripped_bytes=$(stat -c %s "$tmp_file")

    status="PASS"
    if [ "$stripped_bytes" -gt "$err_lim" ]; then
        status="ERROR"
        HAS_ERROR=1
    elif [ "$stripped_bytes" -gt "$warn_lim" ]; then
        status="WARN"
        HAS_WARN=1
    fi

    raw_str=$(format_size "$raw_bytes")
    strip_str=$(format_size "$stripped_bytes")
    warn_str=$(format_size "$warn_lim")
    err_str=$(format_size "$err_lim")

    printf "%-38s | %-10s | %-10s | %-9s | %-9s | " "$display_name" "$raw_str" "$strip_str" "$warn_str" "$err_str"
    if [ "$status" = "PASS" ]; then
        printf "\033[0;32m%-8s\033[0m\n" "✅ PASS"
    elif [ "$status" = "WARN" ]; then
        printf "\033[1;33m%-8s\033[0m\n" "⚠️  WARN"
    else
        printf "\033[0;31m%-8s\033[0m\n" "🚨 FAIL"
    fi
done

printf -- "----------------------------------------------------------------------------------------------------\n\n"

if [ "$CHECKED_COUNT" -eq 0 ]; then
    echo "🚨 [ARCH-GATE FAIL] Zero artifacts were evaluated! Build must be executed before size check."
    exit 1
fi

if [ "$HAS_ERROR" -eq 1 ]; then
    echo "🚨 [ARCH-GATE FAIL] Binary size gate threshold exceeded or required artifact missing! Binary bloat detected."
    exit 1
fi

if [ "$HAS_WARN" -eq 1 ]; then
    echo "⚠️  [ARCH-GATE WARNING] Binary size is approaching limits."
    if [ "${STRICT_SIZE_GATE:-0}" = "1" ]; then
        echo "❌ [ARCH-GATE FAIL] STRICT_SIZE_GATE=1: Failing build on warning."
        exit 1
    fi
fi

echo "✅ [ARCH-GATE PASS] All evaluated binary footprint sizes conform to release budget."
exit 0
