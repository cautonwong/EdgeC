#!/usr/bin/env bash
# ==============================================================================
# EdgeOS Anti-Entropy Gate: Check Exported Symbols Baseline
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

check_archive_symbols() {
    local archive="$1"
    local label="$2"
    shift 2

    if [ ! -f "${archive}" ]; then
        echo "❌ [ARCH-GATE FAIL] ${archive} not found! Please run build first."
        return 1
    fi

    local all_symbols
    all_symbols=$(nm -g "${archive}")
    local missing=()
    local sym

    for sym in "$@"; do
        if ! echo "${all_symbols}" | grep -q "[TDRB] ${sym}$"; then
            missing+=("${sym}")
        fi
    done

    if [ ${#missing[@]} -gt 0 ]; then
        echo "❌ [ARCH-GATE FAIL] Missing essential global symbols in ${label}:"
        for sym in "${missing[@]}"; do
            echo "   - ${sym}"
        done
        return 1
    fi

    echo "✅ [ARCH-GATE PASS] All $# essential symbols verified in ${label}."
}

echo "🔍 [ARCH-GATE] Checking edge_io and edge_north exported symbols..."

check_archive_symbols "${BUILD_DIR}/modules/edge_io/libedge_io.a" "libedge_io.a" \
    "bus_orchestrator_create"
check_archive_symbols "${BUILD_DIR}/modules/edge_api/libedge_api.a" "libedge_api.a" \
    "edge_api_init"

check_archive_symbols "${BUILD_DIR}/adapters/linux_platform/libedge_linux_platform.a" \
    "libedge_linux_platform.a" \
    "io_executor_create" \
    "io_executor_cleanup" \
    "io_executor_http_request"

check_archive_symbols "${BUILD_DIR}/modules/edge_north/libedge_north.a" "libedge_north.a" \
    "northbound_mgr_init_ex" \
    "northbound_mgr_start" \
    "northbound_mgr_stop" \
    "northbound_mgr_destroy"
