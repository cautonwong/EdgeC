#!/usr/bin/env python3
"""
check_guard_path_independence.py - Guard exemptions must key off the scanned tree.

A guard that decides "skip tests" or "skip fixtures" by testing an absolute
path (``"tests" in root``, ``os.path.join("tests", "guards", ...) in path``)
changes what it scans with the checkout location: a worktree at /tmp/guards
re-scans the fixture tree (false red), one at /tmp/tests skips the product tree
(silent green). The stable key is the segment list relative to the scan target:
use ``c_source_scan.rel_scan_parts`` / ``skip_subtree`` and
``guard_baseline.is_selftest_fixture``.
"""

import argparse
import os
import re
import sys

SELF = os.path.basename(__file__)
PATH_VARS = r"(?:root|root_dir|target_dir|dirpath|path)"
MEMBERSHIP = re.compile(
    r"""(["'])(?:tests|guards|fixtures)\1\s+(?:not\s+)?in\s+""" + PATH_VARS + r"\b"
)
JOIN_MEMBERSHIP = re.compile(
    r"""os\.path\.join\(\s*(["'])tests\1\s*,\s*(["'])guards\2[^)]*\)\s+in\s+"""
    + PATH_VARS
    + r"\b"
)


def check_file(path, rel):
    violations = []
    try:
        lines = open(path, "r", encoding="utf-8", errors="ignore").readlines()
    except OSError:
        return violations
    for idx, line in enumerate(lines, 1):
        stripped = line.lstrip()
        if stripped.startswith("#"):
            continue
        if MEMBERSHIP.search(line) or JOIN_MEMBERSHIP.search(line):
            violations.append(
                f"{rel}:{idx}: path-substring exemption; use rel_scan_parts/skip_subtree "
                "from c_source_scan or guard_baseline.is_selftest_fixture"
            )
    return violations


def check_directory(target_dir):
    from c_source_scan import skip_subtree

    target_dir = os.path.abspath(target_dir)
    violations = []
    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if not d.startswith(".")]
        if skip_subtree(root, target_dir):
            continue
        for name in files:
            if not name.endswith(".py") or name == SELF:
                continue
            full = os.path.join(root, name)
            rel = os.path.relpath(full, target_dir).replace("\\", "/")
            violations.extend(check_file(full, rel))
    return violations


def main():
    parser = argparse.ArgumentParser(description="Guard checkout-path independence guard")
    parser.add_argument("--root", default=".")
    args = parser.parse_args()

    violations = check_directory(args.root)
    if violations:
        print(f"❌ [GUARD_PATH_INDEPENDENCE] {len(violations)} violation(s):")
        for v in violations:
            print(f"  - {v}")
        return 1
    print("✅ [GUARD_PATH_INDEPENDENCE] No checkout-path-keyed exemptions.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
