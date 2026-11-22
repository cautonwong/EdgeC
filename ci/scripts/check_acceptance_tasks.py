#!/usr/bin/env python3
"""Fail the acceptance wrapper when an expected task did not run or did not pass.

The platform gate only counts tasks that produced a summary.json. A task file
that fails to load produces none, so the gate can print
"全部合格:1 个验收任务" and exit 0. This check compares the declared task set
(`<root>/<product>/tasks/*.yaml`) with the per-task exit codes the wrapper wrote
to a status file (`<product>\\t<task>\\t<code>`).
"""

import argparse
import glob
import os
import sys

GUARD_NAME = "ACCEPTANCE_TASKS"


def _expected(root, products):
    tasks = []
    wanted = set(products) if products else None
    for product in sorted(os.listdir(root)):
        if wanted is not None and product not in wanted:
            continue
        tasks_dir = os.path.join(root, product, "tasks")
        if not os.path.isdir(tasks_dir):
            continue
        for path in sorted(glob.glob(os.path.join(tasks_dir, "*.yaml"))):
            tasks.append((product, os.path.splitext(os.path.basename(path))[0]))
    return tasks


def _ran(status_path):
    ran = {}
    if not os.path.exists(status_path):
        return ran
    with open(status_path, encoding="utf-8", errors="ignore") as handle:
        for line in handle:
            parts = line.rstrip("\n").split("\t")
            if len(parts) != 3:
                continue
            try:
                ran[(parts[0], parts[1])] = int(parts[2])
            except ValueError:
                ran[(parts[0], parts[1])] = -1
    return ran


def scan(root, status_path, products=None):
    expected = _expected(root, products)
    if not expected:
        return [f"no tasks/*.yaml under {root}"], 0
    ran = _ran(status_path)
    violations = []
    for product, task in expected:
        code = ran.get((product, task))
        if code is None:
            violations.append(f"{product}/{task}: did not run")
        elif code != 0:
            violations.append(f"{product}/{task}: exit {code}")
    return violations, len(expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".", help="acceptance root (contains <product>/tasks)")
    parser.add_argument("--status", default="", help="task status file (default <root>/status.txt)")
    parser.add_argument("--products", default="", help="comma-separated product dirs to require")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    status = args.status or os.path.join(root, "status.txt")
    products = [item for item in args.products.split(",") if item]
    violations, expected = scan(root, status, products)
    if violations:
        print(f"❌ [{GUARD_NAME}] expected task set is not satisfied:")
        for item in violations:
            print(f"  - {item}")
        return 1
    print(f"✅ [{GUARD_NAME}] {expected}/{expected} expected tasks exited 0.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
