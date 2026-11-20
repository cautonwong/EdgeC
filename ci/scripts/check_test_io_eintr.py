#!/usr/bin/env python3
"""Reject blocking recv loops that treat EINTR as a fatal answer.

A recv on a socket with SO_RCVTIMEO is interruptible. The CI failure from
60507e7b proved that treating only EAGAIN as retry makes a test flaky under
signals. Non-blocking recv (MSG_DONTWAIT) is out of scope.

EINTR may be handled on its own branch (`if (errno == EINTR) continue;`), so
the guard accepts any code-level mention of EINTR after comments are stripped
instead of requiring it on the same line as the EAGAIN check.

Known limit: the EINTR exemption is per file. A test file with several blocking
recv loops only needs one of them to handle EINTR. Upgrade to a per-function
scan when a test file grows more than one recv loop.
"""

import argparse
import glob
import os
import re
import sys

import c_source_scan

CALL_RE = re.compile(r"\brecv\s*\(")
ERRNO_RE = re.compile(r"errno\s*(?:!=|==)\s*EAGAIN")
EINTR_RE = re.compile(r"\bEINTR\b")


def scan(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = c_source_scan.strip_comments(handle.read())
    if "SO_RCVTIMEO" not in text:
        return []
    lines = text.splitlines()
    blocking = [line for line in lines if CALL_RE.search(line) and "MSG_DONTWAIT" not in line]
    if not blocking:
        return []
    if EINTR_RE.search(text):
        return []
    return [(number, line.strip()) for number, line in enumerate(lines, 1) if ERRNO_RE.search(line)]


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
