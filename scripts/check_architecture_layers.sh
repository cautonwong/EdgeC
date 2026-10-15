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
echo "  [1/7] Checking modules/edge_algo public headers..."
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
echo "  [2/7] Checking modules/edge_core/include/edge_port.h..."
if grep -E '^[[:space:]]*#[[:space:]]*include[[:space:]]+["<]datahub\.h[">]' "${REPO_ROOT}/modules/edge_core/include/edge_port.h" >/dev/null 2>&1; then
    echo "    ❌ VIOLATION: edge_port.h includes datahub.h directly"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

# 3. 核心领域与调度公开头文件不得包含 <uv.h>
echo "  [3/7] Checking core public contracts for transitive <uv.h> contamination..."
PROTECTED_HEADERS=(
    "modules/edge_core/include/arbiter.h"
    "modules/edge_core/include/edge_service.h"
    "modules/edge_mgmt/include/edgeos/edge_app_mgr.h"
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

# 4. 基础设施与流引擎/Wasm公开头文件不得包含 concrete datahub.h (应使用前向声明或端口抽象)
echo "  [4/7] Checking infra and stream public headers for concrete datahub.h..."
DECOUPLED_PUBLIC_HEADERS=(
    "modules/edge_infra/include/npu_enclave.h"
    "modules/edge_infra/include/npu_pmu.h"
    "modules/edge_infra/include/npu_power_manager.h"
    "modules/edge_infra/include/dds_topology_monitor.h"
    "modules/edge_stream/include/stream_engine.h"
    "modules/edge_stream/include/wasm_app_scheduler.h"
    "modules/edge_stream/include/wasm_mgr.h"
)
for h in "${DECOUPLED_PUBLIC_HEADERS[@]}"; do
    if [ -f "${REPO_ROOT}/${h}" ]; then
        if grep -E '^[[:space:]]*#[[:space:]]*include[[:space:]]+["<]datahub\.h[">]' "${REPO_ROOT}/${h}" >/dev/null 2>&1; then
            echo "    ❌ VIOLATION: Public header ${h} directly includes concrete datahub.h"
            VIOLATIONS=$((VIOLATIONS + 1))
        fi
    fi
done

# 5. 算法实现源文件不得直接包含具体的 datahub.h (防腐适配器 edge_algo_datahub_adapter.c 除外)
echo "  [5/7] Checking modules/edge_algo implementation source files..."
for f in "${REPO_ROOT}"/modules/edge_algo/src/*.c; do
    base=$(basename "$f")
    if [ "$base" = "edge_algo_datahub_adapter.c" ]; then
        continue
    fi
    if grep -E '^[[:space:]]*#[[:space:]]*include[[:space:]]+["<]datahub\.h[">]' "$f" >/dev/null 2>&1; then
        echo "    ❌ VIOLATION: Algorithm source file ${base} includes concrete datahub.h directly (must use consumer ports)"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

# 6. 协议驱动源文件不得直接包含具体的 datahub.h (应由 driver_manager 依赖注入)
echo "  [6/7] Checking modules/edge_io protocol driver source files..."
PROTECTED_DRIVERS=(
    "driver_ccsds.c"
    "driver_cia402.c"
    "driver_dlms.c"
    "driver_dlt645.c"
    "driver_dlt698.c"
    "driver_hl7_fhir.c"
    "driver_iec101.c"
    "driver_iec104.c"
    "driver_sparkplug_b.c"
    "modbus_rtu_driver.c"
    "modbus_tcp_driver.c"
)
for d in "${PROTECTED_DRIVERS[@]}"; do
    df="${REPO_ROOT}/modules/edge_io/src/${d}"
    if [ -f "$df" ]; then
        if grep -E '^[[:space:]]*#[[:space:]]*include[[:space:]]+["<]datahub\.h[">]' "$df" >/dev/null 2>&1; then
            echo "    ❌ VIOLATION: Protocol driver ${d} directly includes datahub.h (must use edge_driver consumer ports)"
            VIOLATIONS=$((VIOLATIONS + 1))
        fi
    fi
done

# 7. 契约代码零漂移检查
echo "  [7/7] Verifying contract drift gate..."
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
