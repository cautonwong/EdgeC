#!/bin/bash
# AEP-411: 72h / Accelerated Soak Test & Memory Convergence Runner
set -e

DURATION_SECS=${1:-10}
echo "========================================================="
echo "  🚀 Starting EdgeOS Soak Test Runner (Duration: ${DURATION_SECS}s)"
echo "========================================================="

BUILD_DIR="$(cd "$(dirname "$0")/../build" && pwd)"
SOAK_BIN="${BUILD_DIR}/tests/test_soak_memory_convergence"

if [ ! -x "${SOAK_BIN}" ]; then
    echo "❌ Soak binary not found at ${SOAK_BIN}, please compile first."
    exit 1
fi

echo "Running memory convergence test..."
${SOAK_BIN}

echo "✅ Soak test completed successfully."
