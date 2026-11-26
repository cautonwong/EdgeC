#!/usr/bin/env python3
"""Reject net_buf_alloc() calls that pass anything but a timeout.

The second argument is a timeout in milliseconds. Passing a byte count there
turns a failed allocation into an event-loop stall proportional to the packet
size. Only 0 (non-blocking) and timeout_ms (forwarded by net_buf_clone) are
accepted.
"""

import argparse
import os
import re
import sys

from c_source_scan import SHARED_PRUNE_DIRS, rel_scan_parts

SKIP_DIRS = {".git", "build", "build-asan", "third_party", "vendor", "node_modules"} | SHARED_PRUNE_DIRS
ALLOWED = {"0", "timeout_ms"}
DEFINITION_PREFIX = re.compile(r"net_buf_t\s*\*+\s*$")


def calls(text):
    for match in re.finditer(r"\bnet_buf_alloc\s*\(", text):
        prefix = text[max(0, match.start() - 40):match.start()]
        if DEFINITION_PREFIX.search(prefix):
            continue
        depth = 0
        end = None
        for i in range(match.end() - 1, len(text)):
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    end = i
                    break
        if end is None:
            continue
        yield match.start(), text[match.end():end]


def split_args(raw):
    args = []
    depth = 0
    start = 0
    for i, ch in enumerate(raw):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "," and depth == 0:
            args.append(raw[start:i])
            start = i + 1
    args.append(raw[start:])
    return args


def scan(root):
    violations = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        if "fixtures" in rel_scan_parts(dirpath, root):
            continue
        for name in filenames:
            if not name.endswith((".c", ".h")):
                continue
            path = os.path.join(dirpath, name)
            try:
                with open(path, "r", encoding="utf-8", errors="replace") as handle:
                    text = handle.read()
            except OSError:
                continue
            for offset, raw in calls(text):
                args = split_args(raw)
                line = text.count("\n", 0, offset) + 1
                if len(args) != 2:
                    violations.append(f"{path}:{line}: expected 2 arguments")
                    continue
                pool = args[0].strip()
                if pool in ("NULL", "0"):
                    violations.append(f"{path}:{line}: pool is {pool}, allocation always fails")
                second = args[1].strip()
                if second not in ALLOWED:
                    violations.append(
                        f"{path}:{line}: timeout argument is {second!r}, expected 0 or timeout_ms"
                    )
    return violations


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    violations = scan(os.path.abspath(args.root))
    if violations:
        print("[NET_BUF_TIMEOUT] invalid net_buf_alloc arguments:")
        for item in violations:
            print("  " + item)
        return 1
    print("[NET_BUF_TIMEOUT] clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
