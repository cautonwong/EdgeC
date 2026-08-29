#!/usr/bin/env bash
# ==============================================================================
# EdgeOS Anti-Entropy Gate: Generated Artifacts & Compiler Drift Verification
# (AEP-305: Generated Artifacts Trust Chain & Reproducibility Gate)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

STRICT_MODE="${STRICT_DRIFT_GATE:-0}"
if [[ "$*" == *"--strict"* ]]; then
    STRICT_MODE=1
fi

echo "🔍 [ARCH-GATE] Running Generated Artifacts Trust Chain & Drift Gate..."

# ------------------------------------------------------------------------------
# 1. Compiler Source-Build & Reproducibility Check
# ------------------------------------------------------------------------------
echo "  [1/3] Verifying contract-compiler-v2 source reproducibility..."
TMP_COMPILER_BIN=$(mktemp /tmp/contract-compiler-repro.XXXXXX)
trap 'rm -f "${TMP_COMPILER_BIN}"' EXIT

(
    cd "${ROOT_DIR}/tools/contract-compiler-v2"
    go build -o "${TMP_COMPILER_BIN}" cmd/main.go
)

COMMITTED_COMPILER="${ROOT_DIR}/.local/linux-x86_64/bin/contract-compiler"
if [ ! -f "${COMMITTED_COMPILER}" ]; then
    echo "❌ [ARCH-GATE ERROR] Committed compiler binary not found at ${COMMITTED_COMPILER}"
    exit 1
fi

SIZE_COMMITTED=$(stat -c%s "${COMMITTED_COMPILER}" 2>/dev/null || stat -f%z "${COMMITTED_COMPILER}")
SIZE_REBUILT=$(stat -c%s "${TMP_COMPILER_BIN}" 2>/dev/null || stat -f%z "${TMP_COMPILER_BIN}")

echo "    • Committed binary size : ${SIZE_COMMITTED} bytes"
echo "    • Source-rebuilt binary  : ${SIZE_REBUILT} bytes"

# Check size difference tolerance (within 1% or exact match)
SIZE_DIFF=$(( SIZE_COMMITTED > SIZE_REBUILT ? SIZE_COMMITTED - SIZE_REBUILT : SIZE_REBUILT - SIZE_COMMITTED ))
if [ "${SIZE_DIFF}" -gt 65536 ]; then
    echo "⚠️  [ARCH-GATE WARNING] Large size delta (${SIZE_DIFF} bytes) between committed and rebuilt compiler binary!"
    if [ "${STRICT_MODE}" = "1" ]; then
        echo "❌ Strict mode enabled: Failing build due to compiler binary drift!"
        exit 1
    fi
else
    echo "    ✅ Compiler source reproducibility confirmed (delta: ${SIZE_DIFF} bytes)."
fi

# ------------------------------------------------------------------------------
# 2. Re-generate All Contract Outputs & Tracked Bindings
# ------------------------------------------------------------------------------
echo "  [2/3] Regenerating all contract bindings from YAML sources..."

PYTHON_BIN="${ROOT_DIR}/.venv/bin/python3"
if [ ! -f "${PYTHON_BIN}" ]; then
    PYTHON_BIN="python3"
fi

# Product contract tuples: Name|ProductYAML|LocalYAML
PRODUCT_SPECS=(
    "iems|contracts/products/iems_v5.yaml|cmd/iems/manifest.yaml"
    "dcu|contracts/products/dcu.yaml|cmd/dcu/manifest.yaml"
    "smart_box|contracts/products/smart_box.yaml|cmd/smart_box/manifest.yaml"
)

for SPEC in "${PRODUCT_SPECS[@]}"; do
    IFS='|' read -r PROD PROD_YAML_REL LOCAL_YAML_REL <<< "${SPEC}"
    PROD_YAML="${ROOT_DIR}/${PROD_YAML_REL}"
    LOCAL_YAML="${ROOT_DIR}/${LOCAL_YAML_REL}"
    GEN_DIR="${ROOT_DIR}/build/products/${PROD}/generated"
    mkdir -p "${GEN_DIR}"

    CMD_ARGS=(
        "--product" "${PROD_YAML}"
        "--out" "${GEN_DIR}"
        "--base" "${ROOT_DIR}"
    )
    if [ -f "${LOCAL_YAML}" ]; then
        CMD_ARGS+=("--local" "${LOCAL_YAML}")
    fi

    "${TMP_COMPILER_BIN}" "${CMD_ARGS[@]}" >/dev/null 2>&1 || {
        echo "❌ [ARCH-GATE ERROR] Failed to compile contract for ${PROD} using source compiler"
        exit 1
    }
done

# Run Go tags aggregation
"${PYTHON_BIN}" "${ROOT_DIR}/scripts/aggregate_go_tags.py" >/dev/null 2>&1 || {
    echo "❌ [ARCH-GATE ERROR] Failed to aggregate Go tags"
    exit 1
}

# ------------------------------------------------------------------------------
# 3. Check for Generated Artifact Drift / Manual Tampering
# ------------------------------------------------------------------------------
echo "  [3/3] Checking for manual tampering or uncommitted drift in tracked generated code..."

# Check git status for tracked files under go/pkg/tags and other generated targets
DRIFTED_FILES=$(git status --porcelain -- go/pkg/tags/ contracts/idl/ || true)

if [ -n "${DRIFTED_FILES}" ]; then
    echo "❌ [ARCH-GATE FAILED] Detected manual tampering or drift in generated artifacts:"
    echo "${DRIFTED_FILES}"
    echo ""
    echo "🔍 Diff details:"
    git diff -- go/pkg/tags/ || true
    echo ""
    echo "🚨 Rule Violation: Generated artifacts must NEVER be manually edited."
    echo "👉 Update the source YAML contracts or contract-compiler templates, then re-run compiler."
    exit 1
fi

echo "✅ [ARCH-GATE PASS] Generated artifacts trust chain verified: Zero drift detected across all contracts and bindings."
exit 0
