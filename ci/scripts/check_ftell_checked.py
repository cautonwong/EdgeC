#!/usr/bin/env python3
"""Require ftell()/ftello() results to be checked before use.

A failed ftell returns -1; casting it straight to size_t allocated `SIZE_MAX`
bytes and then `fread` wrote with that length (three call sites fixed in
2026-11-16). The compiler has no warn_unused_result here, so this guard carries
the rule: the result must be assigned and compared against 0 within three lines
before any allocation or fread.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

GUARD_NAME = "FTELL_CHECKED_GUARD"
from c_source_scan import SHARED_PRUNE_DIRS, rel_scan_parts

SKIP_DIRS = {".git", "build", "third-party", "node_modules", ".local", "web", ".venv"} | SHARED_PRUNE_DIRS
ASSIGN_RE = re.compile(r"\b(\w+)\s*=\s*ftell(?:lo)?\s*\(")
CAST_USE_RE = re.compile(r"\(\s*(?:size_t|uint32_t|int|long)\s*\)\s*ftell(?:lo)?\s*\(")


def scan(root):
    violations = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS and not d.startswith("build")]
        for name in filenames:
            if not name.endswith((".c", ".h")):
                continue
            path = os.path.join(dirpath, name)
            if "fixtures" in rel_scan_parts(dirpath, root):
                continue
            try:
                with open(path, "r", encoding="utf-8", errors="replace") as handle:
                    lines = handle.read().splitlines()
            except OSError:
                continue
            for index, line in enumerate(lines):
                if CAST_USE_RE.search(line):
                    rel = os.path.relpath(path, root).replace(os.sep, "/")
                    violations.append(
                        {
                            "fingerprint": guard_baseline.fingerprint(
                                rel, "FTELL_UNCHECKED", "cast"
                            ),
                            "location": f"{rel}:{index + 1}",
                            "detail": "ftell() cast to an integer type without a -1 check",
                            "rule_id": "FTELL_UNCHECKED",
                        }
                    )
                    continue
                match = ASSIGN_RE.search(line)
                if not match:
                    continue
                var = match.group(1)
                window = "\n".join(lines[index:index + 4])
                if re.search(
                    r"\b" + re.escape(var) + r"\b\s*(?:<|<=|>|>=|==|!=)\s*[-0-9]",
                    window,
                ):
                    continue
                rel = os.path.relpath(path, root).replace(os.sep, "/")
                violations.append(
                    {
                        "fingerprint": guard_baseline.fingerprint(
                            rel, "FTELL_UNCHECKED", var
                        ),
                        "location": f"{rel}:{index + 1}",
                        "detail": f"ftell() result '{var}' is not compared with 0/-1 within three lines",
                        "rule_id": "FTELL_UNCHECKED",
                    }
                )
    return violations


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()
    violations = scan(os.path.abspath(args.root))
    baseline_path = guard_baseline.baseline_path_for(__file__)
    return guard_baseline.apply(violations, baseline_path, args.update_baseline, GUARD_NAME)


if __name__ == "__main__":
    sys.exit(main())
