#!/usr/bin/env bash
# ==============================================================================
# EdgeOS DoD Test: Verify Generated Artifact Tampering Detection (AEP-305)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

echo "🧪 [TEST] Starting AEP-305 Generated Drift & Anti-Tamper Verification..."

TARGET_FILE="${ROOT_DIR}/go/pkg/tags/tags.go"
if [ ! -f "${TARGET_FILE}" ]; then
    echo "❌ Target generated file ${TARGET_FILE} not found!"
    exit 1
fi

# Step 1: Verify clean gate passes
echo "  [Step 1] Verifying clean state passes drift gate..."
if ! STRICT_DRIFT_GATE=1 "${ROOT_DIR}/scripts/check_generated_drift.sh"; then
    echo "❌ Expected clean drift gate to pass, but it failed!"
    exit 1
fi
echo "  ✅ Clean state gate passed."

# Step 2: Inject unauthorized domain contract drift
echo "  [Step 2] Injecting contract drift into contracts/schema/domains/power.yaml..."
TARGET_YAML="${ROOT_DIR}/contracts/schema/domains/power.yaml"
BACKUP_YAML="/tmp/edgeos_power_test_backup.yaml"
cp "${TARGET_YAML}" "${BACKUP_YAML}"
trap 'if [ -f "${BACKUP_YAML}" ]; then cp "${BACKUP_YAML}" "${TARGET_YAML}"; rm -f "${BACKUP_YAML}"; "${ROOT_DIR}/scripts/check_generated_drift.sh" >/dev/null 2>&1 || true; fi' EXIT

# Append a drift tag to domain YAML
cat << 'EOF' >> "${TARGET_YAML}"
  - id: 1999
    name: "TAMPER_DRIFT_UNCOMMITTED_TAG"
    type: "float"
    unit: "V"
EOF

# Step 3: Verify drift gate detects uncommitted drift and fails
echo "  [Step 3] Verifying drift gate traps uncommitted drift and fails with non-zero code..."
if STRICT_DRIFT_GATE=1 "${ROOT_DIR}/scripts/check_generated_drift.sh" >/tmp/tamper_output.log 2>&1; then
    echo "❌ [DoD FAILED] check_generated_drift.sh succeeded when it should have failed on drifted contract!"
    exit 1
else
    echo "  ✅ Contract drift successfully trapped by drift gate!"
    cat /tmp/tamper_output.log
fi

# Step 4: Revert YAML and verify gate passes again
echo "  [Step 4] Reverting YAML and re-verifying gate..."
cp "${BACKUP_YAML}" "${TARGET_YAML}"
rm -f "${BACKUP_YAML}"
if ! STRICT_DRIFT_GATE=1 "${ROOT_DIR}/scripts/check_generated_drift.sh" >/dev/null 2>&1; then
    echo "❌ Reverted gate failed!"
    exit 1
fi

echo "🎉 [DoD SUCCESS] AEP-305 Generated Drift & Anti-Tamper Verification PASSED!"
exit 0
