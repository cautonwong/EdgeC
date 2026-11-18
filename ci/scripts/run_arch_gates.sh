#!/usr/bin/env bash
# Mirror the x86_64 architecture gate step from .github/workflows/ci.yml.
# Usage: ci/scripts/run_arch_gates.sh [build-dir] [base-ref]
set -euo pipefail

BUILD_DIR="${1:-build}"
BASE_REF="${2:-origin/main}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "build directory is not configured: $BUILD_DIR" >&2
    exit 2
fi

# check_binary_sizes.sh hardcodes <root>/build, so point it at the chosen build.
CREATED_LINK=0
if [ ! -e build ]; then
    ln -s "$BUILD_DIR" build
    CREATED_LINK=1
fi
cleanup() {
    if [ "$CREATED_LINK" -eq 1 ]; then rm -f build; fi
}
trap cleanup EXIT

python3 ci/scripts/run_guard_selftest.py
python3 ci/scripts/check_sbom.py --out "$BUILD_DIR/sbom"
python3 ci/scripts/check_component_budgets.py --root . --build "$BUILD_DIR"
python3 ci/scripts/check_resource_discipline.py --root .
./ci/scripts/check_component_targets.sh "$BUILD_DIR"
./scripts/check_no_old_src.sh
./scripts/check_symbols_baseline.sh
STRICT_CORE_GATE=1 ./scripts/check_core_symbols.sh
STRICT_CORE_GATE=1 ./scripts/check_core_readonly.sh "$BASE_REF"
./scripts/check_binary_sizes.sh | tee "$BUILD_DIR/size_report.txt"
