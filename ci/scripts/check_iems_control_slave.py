#!/usr/bin/env python3
"""Reject a hardcoded slave id in IEMS control writes.

A control write must carry the slave address resolved from poll_tasks/inventory.
`edge_api_write_tag(bus_id, proto, 1, ...)` sends every command to device 1 no
matter which tag was addressed, which can actuate the wrong meter on a shared
RS485 bus.
"""

import argparse
import glob
import os
import re
import sys

CALL_RE = re.compile(r"edge_api_write_tag\s*\(\s*([^,]+),\s*([^,]+),\s*([^,]+)\s*,")
COMMENT_RE = re.compile(r"//[^\n]*|/\*.*?\*/", re.DOTALL)


def strip_comments(text):
    return COMMENT_RE.sub("", text)


def scan(root):
    problems = []
    for path in sorted(glob.glob(os.path.join(root, "cmd", "iems", "*.c"))):
        with open(path, encoding="utf-8", errors="replace") as handle:
            text = strip_comments(handle.read())
        for match in CALL_RE.finditer(text):
            third = match.group(3).strip()
            if re.fullmatch(r"(?:0[xX][0-9a-fA-F]+|\d+)[uUlL]*", third):
                line = text.count("\n", 0, match.start()) + 1
                problems.append((os.path.relpath(path, root), line, third))
    return problems


def main():
    parser = argparse.ArgumentParser(description="Check IEMS control write slave ids.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    problems = scan(root)
    if not problems:
        print("✅ [IEMS_CONTROL_SLAVE] control writes carry a resolved slave id")
        return 0

    print("❌ [IEMS_CONTROL_SLAVE] hardcoded slave id in IEMS control write:")
    for rel, line, value in problems:
        print(f"   {rel}:{line}: edge_api_write_tag(...) slave id literal {value}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
