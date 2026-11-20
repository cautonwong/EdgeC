#!/usr/bin/env bash
set -euo pipefail
python3 ci/scripts/run_guard_selftest.py
python3 ci/scripts/check_component_budgets.py --root .
STRICT_CORE_GATE=1 ./scripts/check_core_symbols.sh
./scripts/check_binary_sizes.sh | tee "${1:-build}/size_report.txt"
