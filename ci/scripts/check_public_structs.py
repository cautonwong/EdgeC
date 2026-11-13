#!/usr/bin/env python3
"""Check public structs against the audited baseline."""

import argparse
import glob
import os
import re
import sys

STRUCT = re.compile(r"^[ \t]*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{", re.MULTILINE)
BASELINE = os.path.join("ci", "baseline", "public_structs.txt")


def public_headers(root):
    for pattern in ("modules/*/include/**/*.h", "include/**/*.h"):
        for path in glob.glob(os.path.join(root, pattern), recursive=True):
            yield os.path.relpath(path, root).replace(os.sep, "/"), path


def load_baseline(root):
    entries = set()
    path = os.path.join(root, BASELINE)
    if not os.path.exists(path):
        return entries
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split("|")
            if len(parts) >= 2:
                entries.add((parts[0], parts[1]))
    return entries


def main():
    parser = argparse.ArgumentParser(description="Check public struct audit baseline.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    known = load_baseline(root)
    found = []
    for rel, path in public_headers(root):
        with open(path, encoding="utf-8", errors="ignore") as handle:
            text = handle.read()
        for match in STRUCT.finditer(text):
            found.append((rel, match.group(1)))
    problems = []
    for rel, name in found:
        if (rel, name) not in known:
            problems.append(f"{rel}: {name} is not in the public struct audit baseline")
    if problems:
        print("❌ [PUBLIC_STRUCTS] violations:")
        for problem in problems:
            print(f"   - {problem}")
        print("   Update docs/design/PUBLIC_STRUCT_AUDIT.md and ci/baseline/public_structs.txt.")
        return 1
    print(f"✅ [PUBLIC_STRUCTS] clean: {len(found)} public structs audited.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
