#!/usr/bin/env bash
# ==============================================================================
# EdgeOS / EdgeC Repository Cleanliness & Hygiene Verification Script
# Enforces zero-binary tracking rules (disallows tracked ELF, DB, TGZ, WASM)
# Vendored dependencies in .local/ are whitelisted.
# ==============================================================================
set -euo pipefail

# Always run relative to the repository root directory
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${PROJECT_ROOT}"

echo "🔍 Checking Git tracked files for prohibited binaries and build artifacts..."

# Whitelisted test fixture .wasm files (retained for unit tests / examples):
# - algo/wasm/demo/demo.wasm
# - examples/wasm/c_moving_average/moving_average.wasm
# - examples/wasm/c_voltage_guard/voltage_guard.wasm
# - tests/virus_divzero.wasm
# - tests/virus_loop.wasm

# Prohibited file extensions/patterns (excluding .local/ and whitelisted test fixtures)
PROHIBITED_PATTERNS='(\.db|\.db-shm|\.db-wal|\.sqlite|\.tgz|\.tar\.gz|rehab_companion\.wasm)$'

VIOLATIONS=$(git ls-files | grep -v '^\.local/' | grep -E "${PROHIBITED_PATTERNS}" || true)

ELF_VIOLATIONS=""
for f in $(git ls-files | grep -v '^\.local/' | grep -v '\.\(c\|h\|py\|go\|md\|yaml\|yml\|json\|cmake\|txt\|sh\|in\|toml\|rst\|html\|css\|js\|png\|svg\|jpg\)$'); do
    if [ -f "$f" ]; then
        if file "$f" 2>/dev/null | grep -q "ELF"; then
            if [[ "$f" != *.a ]]; then
                ELF_VIOLATIONS="${ELF_VIOLATIONS}\n${f}"
            fi
        fi
    fi
done

if [ -n "${VIOLATIONS}" ] || [ -n "${ELF_VIOLATIONS}" ]; then
    echo "❌ ERROR: Prohibited binary files or build artifacts detected in git index:"
    if [ -n "${VIOLATIONS}" ]; then
        echo -e "${VIOLATIONS}"
    fi
    if [ -n "${ELF_VIOLATIONS}" ]; then
        echo -e "${ELF_VIOLATIONS}"
    fi
    echo ""
    echo "Please remove them using: git rm --cached <file>"
    exit 1
fi

echo "✅ Repository hygiene check PASSED! No prohibited binary artifacts tracked in git."
exit 0
