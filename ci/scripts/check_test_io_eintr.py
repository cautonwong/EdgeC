#!/usr/bin/env python3
"""Reject blocking recv loops that treat EINTR as a fatal answer.

A recv on a socket with SO_RCVTIMEO is interruptible. The CI failure from
60507e7b proved that treating only EAGAIN as retry makes a test flaky under
signals. Non-blocking recv (MSG_DONTWAIT) is out of scope.
"""

import argparse
import glob
import os
import re
import sys

CALL_RE = re.compile(r"\brecv\s*\(")
ERRNO_RE = re.compile(r"errno\s*(?:!=|==)\s*EAGAIN")


def scan(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    if "SO_RCVTIMEO" not in text:
        return []
    lines = text.splitlines()
    blocking = [line for line in lines if CALL_RE.search(line) and "MSG_DONTWAIT" not in line]
    if not blocking:
        return []
    problems = []
    for number, line in enumerate(lines, 1):
        stripped = line.lstrip()
        if stripped.startswith(("/*", "*", "//")):
            continue
        if ERRNO_RE.search(line) and "EINTR" not in line:
            problems.append((number, line.strip()))
    return problems


def main():
    parser = argparse.ArgumentParser(description="Check blocking recv EINTR handling in tests.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    tests_dir = os.path.join(root, "tests")

    offenders = []
    for path in sorted(glob.glob(os.path.join(tests_dir, "*.c"))):
        problems = scan(path)
        if problems:
            offenders.append((path, problems))

    if not offenders:
        print("✅ [TEST_IO_EINTR] blocking recv loops retry on EINTR")
        return 0

    print("❌ [TEST_IO_EINTR] blocking recv treats EINTR as a terminal error:")
    for path, problems in offenders:
        rel = os.path.relpath(path, root)
        for number, line in problems:
            print(f"   {rel}:{number}: {line}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
