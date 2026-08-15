#!/usr/bin/env bash
# ==============================================================================
# EdgeOS Anti-Entropy Gate: Check Exported Symbols Baseline
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "🔍 [ARCH-GATE] Checking libedge_io.a exported symbols..."

if [ ! -f "${BUILD_DIR}/modules/edge_io/libedge_io.a" ]; then
    echo "❌ [ARCH-GATE FAIL] ${BUILD_DIR}/modules/edge_io/libedge_io.a not found! Please run build first."
    exit 1
fi

ALL_SYMBOLS=$(nm -g "${BUILD_DIR}/modules/edge_io/libedge_io.a")

# 核心符号完整性断言
REQUIRED_IO_SYMBOLS=(
    "northbound_mgr_init_ex"
    "northbound_mgr_start"
    "northbound_mgr_stop"
    "northbound_mgr_destroy"
    "io_executor_create"
    "io_executor_cleanup"
    "io_executor_http_request"
)

MISSING_SYMBOLS=()
for sym in "${REQUIRED_IO_SYMBOLS[@]}"; do
    if ! echo "$ALL_SYMBOLS" | grep -q "[TDRB] ${sym}$"; then
        MISSING_SYMBOLS+=("$sym")
    fi
done

if [ ${#MISSING_SYMBOLS[@]} -gt 0 ]; then
    echo "❌ [ARCH-GATE FAIL] Missing essential global symbols in libedge_io.a:"
    for sym in "${MISSING_SYMBOLS[@]}"; do
        echo "   - ${sym}"
    done
    exit 1
fi

echo "✅ [ARCH-GATE PASS] All ${#REQUIRED_IO_SYMBOLS[@]} essential symbols verified in libedge_io.a."
