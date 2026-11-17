#!/usr/bin/env python3
"""Sanitizer-gated perf tests must disable wall-clock budgets for every sanitizer.

`EDGE_SANITIZER_BUILD` turns off microsecond/millisecond budgets that
instrumentation invalidates. A condition that lists ASAN and TSAN but forgets
UBSAN leaves UBSAN builds asserting budgets they cannot meet, so a loaded
machine turns the suite red for reasons unrelated to the code.
"""

import argparse
import os
import re
import sys

SANITIZERS = ("ENABLE_ASAN", "ENABLE_TSAN", "ENABLE_UBSAN")
IF_RE = re.compile(r"^\s*if\s*\((.*)\)\s*$")
ENDIF_RE = re.compile(r"^\s*endif\s*\(")


def scan(root):
    violations = []
    for rel in ("CMakeLists.txt", os.path.join("tests", "CMakeLists.txt")):
        path = os.path.join(root, rel)
        if not os.path.exists(path):
            continue
        with open(path, encoding="utf-8", errors="ignore") as handle:
            lines = handle.read().splitlines()
        conditions = []
        for number, raw in enumerate(lines, 1):
            line = re.sub(r"#.*", "", raw)
            match = IF_RE.match(line)
            if match:
                conditions.append((number, match.group(1)))
                continue
            if ENDIF_RE.match(line):
                if conditions:
                    conditions.pop()
                continue
            if "EDGE_SANITIZER_BUILD" not in line:
                continue
            for start, condition in conditions:
                if not any(name in condition for name in SANITIZERS):
                    continue
                missing = [name for name in SANITIZERS if name not in condition]
                if missing:
                    violations.append(
                        f"{rel}:{number}: EDGE_SANITIZER_BUILD is gated by a condition "
                        f"missing {', '.join(missing)} (line {start}: {condition.strip()})"
                    )
    return violations


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    violations = scan(os.path.abspath(args.root))
    for violation in violations:
        print(violation)
    if violations:
        print(f"❌ [SANITIZER_PERF_GATES] {len(violations)} violation(s)")
        return 1
    print("✅ [SANITIZER_PERF_GATES] every sanitizer-gated perf budget covers ASAN, TSAN and UBSAN")
    return 0


if __name__ == "__main__":
    sys.exit(main())
