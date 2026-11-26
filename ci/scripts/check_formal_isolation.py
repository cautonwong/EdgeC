#!/usr/bin/env python3
"""formal/ may include production sources; the product must never reference formal/.

Only tracked files are product source. Generated build files embed the absolute
checkout path, so scanning the disk flagged any checkout whose path contained
"formal".
"""

import argparse
import os
import re
import subprocess
import sys

from c_source_scan import SHARED_PRUNE_DIRS, skip_subtree
import guard_baseline

PRUNE_DIRS = {".git", ".local", ".venv", ".vscode", "build", "third-party",
              "node_modules", "docs", "web", "docker"} | SHARED_PRUNE_DIRS
CMAKE_REF = re.compile(r"(^|[^A-Za-z0-9_])formal([^A-Za-z0-9_]|$)")
INCLUDE_REF = re.compile(r'#\s*include\s*[<"][^">]*\bformal/')


def tracked_files(target_dir):
    proc = subprocess.run(["git", "-C", target_dir, "ls-files", "-z"],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if proc.returncode != 0:
        raise RuntimeError(
            f"git ls-files failed in {target_dir}: {os.fsdecode(proc.stderr).strip()}")
    return [os.fsdecode(name) for name in proc.stdout.split(b"\0") if name]


def scan(target_dir):
    target_dir = os.path.abspath(target_dir)
    fixture_mode = guard_baseline.is_selftest_fixture(target_dir)
    violations = []
    for rel in tracked_files(target_dir):
        dirs = rel.split("/")[:-1]
        if any(d.startswith(".") or d in PRUNE_DIRS for d in dirs):
            continue
        if not fixture_mode and "fixtures" in dirs:
            continue
        path = os.path.join(target_dir, rel)
        if not os.path.isfile(path):
            continue
        name = rel.rsplit("/", 1)[-1]
        if name == "CMakeLists.txt" or name.endswith(".cmake"):
            with open(path, encoding="utf-8", errors="replace") as handle:
                text = handle.read()
            for line_no, line in enumerate(text.splitlines(), 1):
                if CMAKE_REF.search(line):
                    violations.append(f"{rel}:{line_no}: build graph references formal/")
            continue
        if not name.endswith((".c", ".h", ".cpp", ".hpp")):
            continue
        with open(path, encoding="utf-8", errors="replace") as handle:
            for line_no, line in enumerate(handle, 1):
                if INCLUDE_REF.search(line):
                    violations.append(f"{rel}:{line_no}: includes formal/")
    return violations


def main():
    parser = argparse.ArgumentParser(description="Check that the product does not use formal/.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    try:
        violations = scan(args.root)
    except RuntimeError as exc:
        print(f"❌ [FORMAL-ISOLATION] {exc}")
        return 1
    if violations:
        print("❌ [FORMAL-ISOLATION] product references formal/:")
        for violation in violations:
            print("  " + violation)
        return 1
    print("✅ [FORMAL-ISOLATION] product build graph and sources are clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
