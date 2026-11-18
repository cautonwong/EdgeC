#!/usr/bin/env python3
"""Reject a hardcoded protocol in the IEMS task loader.

iEMS keeps the protocol per inventory device. A literal here silently sends
every task to the same driver and hides a bad inventory row, so the loader must
read the SQL protocol column (argv[3]).
"""

import argparse
import os
import re
import sys

STRING_RE = re.compile(r'"([^"\\]*(?:\\.[^"\\]*)*)"')
PROTOCOL_RE = re.compile(
    r'^"?(?:MODBUS|MODBUSRTU|MODBUSTCP|IEC104|CAN|DLT645(?:-1997|-2007)?)"?$',
    re.IGNORECASE,
)
ARGV3_RE = re.compile(r"\bargv\s*\[\s*3\s*\]")


def scan(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        lines = handle.read().splitlines()
    literals = []
    has_argv3 = False
    for number, line in enumerate(lines, 1):
        for match in STRING_RE.finditer(line):
            if PROTOCOL_RE.match(match.group(0)):
                literals.append((number, match.group(0)))
        if ARGV3_RE.search(line):
            has_argv3 = True
    return literals, has_argv3


def main():
    parser = argparse.ArgumentParser(description="Check IEMS task protocol selection.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    rel = os.path.join("cmd", "iems", "iems_task_loader.c")
    path = os.path.join(root, rel)

    if not os.path.isfile(path):
        print(f"❌ [PROTOCOL_SELECTION] {rel}:1: file not found")
        return 1

    literals, has_argv3 = scan(path)
    problems = []
    for number, literal in literals:
        problems.append(
            f"   {rel}:{number}: hardcoded protocol {literal}; "
            "protocol must come from the SQL column argv[3]"
        )
    if not has_argv3:
        problems.append(f"   {rel}:1: no argv[3]; protocol must come from the SQL column")

    if not problems:
        print("✅ [PROTOCOL_SELECTION] protocol comes from inventory.protocol (argv[3])")
        return 0

    print("❌ [PROTOCOL_SELECTION] hardcoded protocol in IEMS task loader:")
    print("\n".join(problems))
    return 1


if __name__ == "__main__":
    sys.exit(main())
