#!/usr/bin/env python3
"""Reject strncpy() calls that cannot NUL-terminate the destination.

The compiler only proves this for literal sources and inlined callers. Real
code kept 17 magic-bound calls (`strncpy(dst, src, 31)` into `char dst[32]`)
that the analyzer missed: when the source fills the bound, no terminator is
written and later `%s` / `strcmp` reads past the field.

This guard flags, in repository sources:
  * a numeric bound with a non-literal source (or a literal that fills it), and
  * a bound of exactly `sizeof(dst)` (no room for the terminator),
unless an explicit `= '\\0'` assignment follows within four lines.

`sizeof(dst) - 1` stays allowed: the tree has many such calls and the warning
gate already covers the ones GCC can prove. This guard closes the magic-number
and full-capacity shapes that the compiler cannot see.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

GUARD_NAME = "STRNCPY_BOUNDS_GUARD"
SKIP_DIRS = {".git", "build", "third-party", "node_modules", ".local", "web", ".venv"}
CALL_RE = re.compile(r"\bstrncpy\s*\(")
LITERAL_RE = re.compile(r'^\s*"((?:[^"\\]|\\.)*)"\s*$')
NUMERIC_RE = re.compile(r"^\s*(0x[0-9a-fA-F]+|[0-9]+)\s*$")
SIZEOF_RE = re.compile(r"^\s*sizeof\s*\(.+\)\s*$")
TERMINATOR_RE = re.compile(r"=\s*'\\0'\s*;")


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


def calls(text):
    for match in CALL_RE.finditer(text):
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


def scan(root):
    violations = []
    inside_fixtures = os.path.join("tests", "guards", "fixtures") in root
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS and not d.startswith("build")]
        for name in filenames:
            if not name.endswith((".c", ".h")):
                continue
            path = os.path.join(dirpath, name)
            # Fixtures intentionally contain the violating shapes.
            if not inside_fixtures and os.path.join("tests", "guards", "fixtures") in path:
                continue
            try:
                with open(path, "r", encoding="utf-8", errors="replace") as handle:
                    text = handle.read()
            except OSError:
                continue
            lines = text.splitlines()
            for offset, raw in calls(text):
                args = split_args(raw)
                if len(args) != 3:
                    continue
                dst, src, bound = (a.strip() for a in args)
                numeric = NUMERIC_RE.match(bound)
                full_size = SIZEOF_RE.match(bound)
                if not numeric and not full_size:
                    continue
                if numeric:
                    value = int(numeric.group(1), 0)
                    literal = LITERAL_RE.match(src)
                    if literal and len(literal.group(1)) < value:
                        continue
                line_no = text.count("\n", 0, offset) + 1
                tail = "\n".join(lines[line_no - 1:line_no + 3])
                if TERMINATOR_RE.search(tail):
                    continue
                rel = os.path.relpath(path, root).replace(os.sep, "/")
                violations.append(
                    {
                        "fingerprint": guard_baseline.fingerprint(
                            rel, "STRNCPY_NO_TERMINATOR", dst
                        ),
                        "location": f"{rel}:{line_no}",
                        "detail": f"strncpy({dst}, {src[:40]}, {bound}) has no following = '\\0'",
                        "rule_id": "STRNCPY_NO_TERMINATOR",
                    }
                )
    return violations


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    violations = scan(root)
    baseline_path = guard_baseline.baseline_path_for(__file__)
    return guard_baseline.apply(violations, baseline_path, args.update_baseline, GUARD_NAME)


if __name__ == "__main__":
    sys.exit(main())
