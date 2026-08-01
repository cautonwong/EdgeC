#!/usr/bin/env bash
# ==============================================================================
# EdgeOS / EdgeC Repository Cleanliness & Hygiene Verification Script
# Enforces zero-binary tracking rules (disallows tracked ELF, DB, TGZ, WASM)
# Vendored dependencies in .local/ are whitelisted.
# ==============================================================================
set -euo pipefail

echo "🔍 Checking Git tracked files for prohibited binaries and build artifacts..."

# Prohibited file extensions/patterns (excluding .local/)
PROHIBITED_PATTERNS='(\.db|\.db-shm|\.db-wal|\.sqlite|\.tgz|\.tar\.gz|rehab_companion\.wasm)$'

VIOLATIONS=$(git ls-files | grep -v '^\.local/' | grep -E "${PROHIBITED_PATTERNS}" || true)

# Also check for tracked ELF executables outside .local/
ELF_VIOLATIONS=""
for f in $(git ls-files | grep -v '^\.local/'); do
    if [ -f "$f" ]; then
        if file "$f" 2>/dev/null | grep -q "ELF"; then
            # Ignore static library archives .a
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
