#!/usr/bin/env python3
"""Coverage floor gate.

Reads an lcov .info file and fails when a scope's line coverage drops below
the floor recorded in ci/baseline/coverage_floor.txt. Floors are measured from
the CI-equivalent local run plus a margin; raising a floor is progress, lowering
one is an architecture decision that must be justified in the commit.
"""

import argparse
import os
import re
import sys

BASELINE = os.path.join("ci", "baseline", "coverage_floor.txt")


def load_floors(path, root):
    floors = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            scope, value = line.split("|", 1)
            floors[scope.strip()] = float(value.strip())
    return floors


def scope_of(rel_path, scopes):
    best = "ALL"
    best_len = -1
    for scope in scopes:
        if scope == "ALL":
            continue
        prefix = scope.rstrip("/") + "/"
        if rel_path.startswith(prefix) and len(prefix) > best_len:
            best = scope
            best_len = len(prefix)
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--info", required=True)
    ap.add_argument("--root", default=".")
    ap.add_argument("--baseline", default=BASELINE)
    args = ap.parse_args()

    root = os.path.abspath(args.root)
    floors = load_floors(os.path.join(root, args.baseline), root)

    hits = {scope: [0, 0] for scope in floors if scope != "ALL"}
    overall = [0, 0]
    current = None
    with open(args.info, encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith("SF:"):
                path = line[3:].strip()
                current = os.path.abspath(path)
            elif line.startswith("DA:") and current:
                rel = os.path.relpath(current, root)
                if rel.startswith("..") or "/tests/" in rel or "/third-party/" in rel:
                    continue
                _, count = line[3:].strip().split(",", 1)
                scope = scope_of(rel, floors)
                overall[1] += 1
                if int(count) > 0:
                    overall[0] += 1
                if scope != "ALL":
                    hits.setdefault(scope, [0, 0])
                    hits[scope][1] += 1
                    if int(count) > 0:
                        hits[scope][0] += 1

    failures = []
    print(f"{'scope':<24} {'lines':>10} {'floor':>7} {'result':>7}")
    for scope in sorted(floors):
        if scope == "ALL":
            hit, all_lines = overall
        else:
            hit, all_lines = hits.get(scope, [0, 0])
        if all_lines == 0:
            failures.append(f"{scope}: no instrumented lines found")
            print(f"{scope:<24} {'0':>10} {floors[scope]:>6.1f}% {'MISSING':>7}")
            continue
        rate = 100.0 * hit / all_lines
        ok = rate + 1e-9 >= floors[scope]
        print(f"{scope:<24} {rate:>9.1f}% {floors[scope]:>6.1f}% {'PASS' if ok else 'FAIL':>7}")
        if not ok:
            failures.append(f"{scope}: {rate:.1f}% < floor {floors[scope]:.1f}%")

    if failures:
        for failure in failures:
            print(f"❌ [COVERAGE_GATE] {failure}", file=sys.stderr)
        return 1
    print("✅ [COVERAGE_GATE] all scopes meet their floors")
    return 0


if __name__ == "__main__":
    sys.exit(main())
