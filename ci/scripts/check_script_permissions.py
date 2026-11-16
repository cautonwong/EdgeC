#!/usr/bin/env python3
"""Fail when a workflow invokes a repo script that is not executable.

A patch or archive round-trip can drop the executable bit, and CI then dies
with "Permission denied" before any test runs. Workflows are the contract:
every path they call as ./<path> must carry the executable bit.
"""

import argparse
import os
import re
import sys

INVOCATION = re.compile(r"\./([A-Za-z0-9_./-]+\.(?:sh|py))")


def scan(root):
    workflows = os.path.join(root, ".github", "workflows")
    if not os.path.isdir(workflows):
        return []
    violations = []
    for name in sorted(os.listdir(workflows)):
        if not name.endswith((".yml", ".yaml")):
            continue
        path = os.path.join(workflows, name)
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            for line_no, line in enumerate(handle, 1):
                for match in INVOCATION.finditer(line):
                    target = os.path.join(root, match.group(1))
                    if not os.path.isfile(target):
                        violations.append(f"{path}:{line_no}: missing {match.group(1)}")
                    elif not os.access(target, os.X_OK):
                        violations.append(f"{path}:{line_no}: not executable {match.group(1)}")
    return violations


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    violations = scan(os.path.abspath(args.root))
    if violations:
        print("[SCRIPT_PERMISSIONS] workflow invokes non-executable or missing scripts:")
        for item in violations:
            print("  " + item)
        return 1
    print("[SCRIPT_PERMISSIONS] clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
