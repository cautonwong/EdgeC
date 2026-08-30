#!/usr/bin/env bash
# ==============================================================================
# EdgeOS DoD Test: Verify edge-cli Scaffolding & Contract Linter (AEP-303)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

CLI_BIN="${ROOT_DIR}/.local/linux-x86_64/bin/edge-cli"
if [ ! -x "${CLI_BIN}" ]; then
    echo "🔨 Building edge-cli binary..."
    ( cd "${ROOT_DIR}/tools/edge-cli" && go build -o "${CLI_BIN}" cmd/main.go )
fi

echo "🧪 [TEST] Starting AEP-303 edge-cli Scaffolding & Linter Verification..."

# ------------------------------------------------------------------------------
# Test 1: CLI Version & Help
# ------------------------------------------------------------------------------
echo "  [Test 1] Testing edge-cli version & help..."
"${CLI_BIN}" version | grep -q "edge-cli version 1.0.0"
"${CLI_BIN}" help | grep -q "create-product"
echo "  ✅ Version and help commands verified."

# ------------------------------------------------------------------------------
# Test 2: Contract Linter Validation
# ------------------------------------------------------------------------------
echo "  [Test 2] Testing contract linter against valid & invalid schemas..."
"${CLI_BIN}" lint-contract contracts/products/iems_v5.yaml --strict
"${CLI_BIN}" lint-contract contracts/products/dcu.yaml --strict

# Test invalid YAML detection
TMP_BAD_YAML="/tmp/bad_contract.yaml"
cat << 'EOF' > "${TMP_BAD_YAML}"
product: "bad_product"
tags:
  - id: 1001
    name: "VALID_TAG_A"
    type: "float"
  - id: 1001
    name: "CONFLICTING_TAG_ID"
    type: "illegal_type_xyz"
EOF

if "${CLI_BIN}" lint-contract "${TMP_BAD_YAML}" >/tmp/bad_lint_out.log 2>&1; then
    echo "❌ Expected linter to fail on bad contract YAML, but it succeeded!"
    rm -f "${TMP_BAD_YAML}"
    exit 1
fi
rm -f "${TMP_BAD_YAML}"
echo "  ✅ Contract linter correctly rejected duplicate TagID and illegal type."

# ------------------------------------------------------------------------------
# Test 3: Product Scaffolding & Contract Compiler Compatibility
# ------------------------------------------------------------------------------
echo "  [Test 3] Testing product scaffolding (create-product)..."
PROD_NAME="scaffold_demo"
CLEANUP_PATHS=(
    "${ROOT_DIR}/cmd/${PROD_NAME}"
    "${ROOT_DIR}/contracts/products/${PROD_NAME}.yaml"
    "${ROOT_DIR}/build/products/${PROD_NAME}"
)

# Ensure clean state before test
for p in "${CLEANUP_PATHS[@]}"; do rm -rf "$p"; done

"${CLI_BIN}" create-product "${PROD_NAME}" --base="${ROOT_DIR}"

# Verify files created
[ -f "${ROOT_DIR}/cmd/${PROD_NAME}/CMakeLists.txt" ]
[ -f "${ROOT_DIR}/cmd/${PROD_NAME}/main.c" ]
[ -f "${ROOT_DIR}/cmd/${PROD_NAME}/manifest.yaml" ]
[ -f "${ROOT_DIR}/contracts/products/${PROD_NAME}.yaml" ]

# Lint generated contract
"${CLI_BIN}" lint-contract "${ROOT_DIR}/contracts/products/${PROD_NAME}.yaml" --base="${ROOT_DIR}" --strict

# Verify contract compiler processes scaffolded product
mkdir -p "${ROOT_DIR}/build/products/${PROD_NAME}/generated"
"${ROOT_DIR}/.local/linux-x86_64/bin/contract-compiler" \
    --product "${ROOT_DIR}/contracts/products/${PROD_NAME}.yaml" \
    --local "${ROOT_DIR}/cmd/${PROD_NAME}/manifest.yaml" \
    --out "${ROOT_DIR}/build/products/${PROD_NAME}/generated" \
    --base "${ROOT_DIR}"

[ -f "${ROOT_DIR}/build/products/${PROD_NAME}/generated/contracts.c" ]
[ -f "${ROOT_DIR}/build/products/${PROD_NAME}/generated/edge_driver_registry.h" ]
[ -f "${ROOT_DIR}/build/products/${PROD_NAME}/generated/edge_driver_registry.c" ]

# Clean up product test artifacts
for p in "${CLEANUP_PATHS[@]}"; do rm -rf "$p"; done
echo "  ✅ Product scaffolding & compilation verified."

# ------------------------------------------------------------------------------
# Test 4: Driver Scaffolding
# ------------------------------------------------------------------------------
echo "  [Test 4] Testing driver scaffolding (create-driver)..."
DRV_NAME="test_meter"
DRV_HEADER="${ROOT_DIR}/modules/edge_io/include/drivers/${DRV_NAME}_driver.h"
DRV_SRC="${ROOT_DIR}/modules/edge_io/src/drivers/${DRV_NAME}_driver.c"
DRV_BINDING="${ROOT_DIR}/contracts/bindings/modbus/${DRV_NAME}.yaml"

rm -f "${DRV_HEADER}" "${DRV_SRC}" "${DRV_BINDING}"

"${CLI_BIN}" create-driver "${DRV_NAME}" --type=serial --protocol=modbus --base="${ROOT_DIR}"

[ -f "${DRV_HEADER}" ]
[ -f "${DRV_SRC}" ]
[ -f "${DRV_BINDING}" ]

grep -q "extern const edge_driver_t ${DRV_NAME}_driver;" "${DRV_HEADER}"
grep -q "const edge_driver_t ${DRV_NAME}_driver" "${DRV_SRC}"
"${CLI_BIN}" lint-contract "${DRV_BINDING}" --base="${ROOT_DIR}"

# Clean up driver test artifacts
rm -f "${DRV_HEADER}" "${DRV_SRC}" "${DRV_BINDING}"
echo "  ✅ Driver scaffolding verified."

echo "🎉 [DoD SUCCESS] AEP-303 edge-cli Scaffolding & Linter Verification PASSED!"
exit 0
