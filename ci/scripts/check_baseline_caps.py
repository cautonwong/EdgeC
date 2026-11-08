#!/usr/bin/env python3
"""check_baseline_caps.py - Keep ci/baseline/limits.txt equal to the debt it caps.

A cap is a ceiling on how many entries a guard's ledger may hold. Once the code is
fixed and the ledger shrinks, a cap left behind allows `--update-baseline` to absorb
that much new debt silently - which is exactly what the hand-maintained file exists
to prevent. This guard fails whenever a cap and its ledger disagree, in either
direction, so the two move together:

  * ledger grew past the cap   -> fix the code, or raise the cap in review
  * ledger shrank below the cap -> lower the cap, that is the progress

Keys are the guard names the check scripts pass to guard_baseline.apply().
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

# guard_name -> the ledger file it caps
LEDGERS = {
    "PLANE_ISOLATION_GUARD": "check_plane_isolation.txt",
    "AGGREGATE_BOUNDARY_GUARD": "check_aggregate_boundary.txt",
    "COMPOSITION_ROOT_GUARD": "check_composition_root.txt",
    "OCP_PLUGIN_GUARD": "check_open_closed_plugins.txt",
    "ZERO_DYNAMIC_MEMORY_GUARD": "check_no_dynamic_memory.txt",
    "STACK_USAGE_GUARD": "check_stack_usage.txt",
}


def main():
    parser = argparse.ArgumentParser(description="Check ledger caps match ledger sizes.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    limits_path = os.path.join(root, "ci", "baseline", "limits.txt")
    caps = guard_baseline.load_limits(os.path.join(root, "ci", "scripts", "check_baseline_caps.py"))
    if not caps:
        print("❌ [BASELINE_CAPS] ci/baseline/limits.txt is missing or unreadable.")
        return 1

    problems = []
    for guard_name, ledger_name in LEDGERS.items():
        if guard_name not in caps:
            problems.append(f"{guard_name} has no cap in {os.path.relpath(limits_path, root)}")
            continue
        ledger_path = os.path.join(root, "ci", "baseline", ledger_name)
        actual = len(guard_baseline.load_baseline(ledger_path))
        cap = caps[guard_name]
        if actual != cap:
            hint = (
                "ledger shrank, lower the cap" if actual < cap else "fix the code, or raise the cap in review"
            )
            problems.append(f"{guard_name}: cap {cap} but {ledger_name} holds {actual} ({hint})")

    if problems:
        print("❌ [BASELINE_CAPS] cap and ledger disagree:")
        for item in problems:
            print(f"   - {item}")
        return 1

    print(f"✅ [BASELINE_CAPS] All {len(LEDGERS)} caps match their ledgers.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
