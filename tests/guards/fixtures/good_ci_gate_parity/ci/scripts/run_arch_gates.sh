#!/usr/bin/env bash
set -euo pipefail
python3 ci/scripts/run_guard_selftest.py
python3 ci/scripts/check_component_budgets.py --root . --build "${1:-build}"
STRICT_CORE_GATE=1 ./scripts/check_core_symbols.sh
./scripts/check_binary_sizes.sh | tee "${1:-build}/size_report.txt"
./ci/scripts/check_formal.sh ${FORMAL_STRICT:+--strict}
./ci/scripts/check_formal_negative.sh ${FORMAL_STRICT:+--strict}
python3 ci/scripts/check_formal_sync.py --root .
python3 ci/scripts/check_formal_isolation.py --root .
