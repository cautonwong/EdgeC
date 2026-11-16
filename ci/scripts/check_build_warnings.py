#!/usr/bin/env python3
"""Fail a build that emits compiler warnings from repository sources.

CI builds with `-Wall -Wextra -Wformat=2 -Wconversion` but never turned a warning
into a failure, so the tree accumulated dead code, narrowing conversions and
unterminated strncpy() calls silently. This guard parses the build log and fails
on any warning whose source file belongs to this repository.

Warnings GCC attributes to a system header (the fortified string family) are
mapped back through the nearest preceding `inlined from '<fn>' at <file>:<line>`
note, so they are charged to the caller that actually chose the bad arguments.

Fingerprint format (line-shift tolerant, guard_baseline.py):
    <rel_path>|W:<flag>|<function>:<normalized message>
Digits are normalized out of the message. Unrelated edits do not churn the
ledger; a changed warning or a moved call into another function does.

Run it against a clean build log (`--clean-first`). An incremental log omits
unchanged translation units and makes their pinned ledger entries look stale.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

GUARD_NAME = "BUILD_WARNING_GUARD"
WARNING_RE = re.compile(
    r"^(?P<file>.+?):(?P<line>\d+):(?:(?P<col>\d+):)? warning: (?P<msg>.*?)"
    r"(?: \[(?P<flag>-W[^\]]+)\])?$"
)
INLINED_RE = re.compile(r"inlined from '(?P<fn>[^']+)' at (?P<file>[^:]+):(?P<line>\d+)")
FUNCTION_RE = re.compile(r"In function '(?P<fn>[^']+)'")
IGNORED_DIRS = ("third-party/", "build", ".local/", ".git/")
IGNORED_PREFIXES = ("/usr/", "/opt/", "/lib/", "/lib64/", "/proc/", "/sys/")


def normalize(message):
    return re.sub(r"\s+", " ", re.sub(r"\d+", "#", message)).strip()


def relative_repo_path(path, root):
    if path.startswith("/"):
        if any(path.startswith(prefix) for prefix in IGNORED_PREFIXES):
            return None
        try:
            rel = os.path.relpath(path, root)
        except ValueError:
            return None
        if rel.startswith(".."):
            return None
        path = rel
    path = path.replace(os.sep, "/")
    if any(path.startswith(ignored) for ignored in IGNORED_DIRS):
        return None
    return path


def scan(log_path, root):
    violations = []
    last_inlined = None
    last_function = None
    with open(log_path, "r", encoding="utf-8", errors="replace") as handle:
        for raw in handle:
            # GCC 11 quotes names with ‘ ’ when the locale is UTF-8; fold them so
            # one ledger works for both spellings.
            line = raw.rstrip("\n").replace("‘", "'").replace("’", "'")
            inlined = INLINED_RE.search(line)
            if inlined:
                rel = relative_repo_path(inlined.group("file"), root)
                last_inlined = (inlined.group("fn"), rel, int(inlined.group("line")))
            function = FUNCTION_RE.search(line)
            if function:
                last_function = function.group("fn")
            match = WARNING_RE.match(line)
            if not match:
                continue
            rel = relative_repo_path(match.group("file"), root)
            line_number = int(match.group("line"))
            function_name = last_function or ""
            if rel is None:
                # System header: charge the warning to the inlined caller.
                if not last_inlined or not last_inlined[1]:
                    continue
                function_name, rel, line_number = last_inlined
            message = match.group("msg").strip()
            flag = match.group("flag") or "unknown"
            scope = f"{function_name}:{normalize(message)}" if function_name else normalize(message)
            violations.append(
                {
                    "fingerprint": guard_baseline.fingerprint(rel, f"W:{flag}", scope),
                    "location": f"{rel}:{line_number}",
                    "detail": message,
                    "rule_id": flag,
                }
            )
    return violations


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", required=True, help="captured compiler build log")
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    log_path = os.path.abspath(args.log)
    if not os.path.exists(log_path):
        print(f"[{GUARD_NAME}] build log not found: {log_path}")
        print("Capture the build with `cmake --build ... 2>&1 | tee build.log`.")
        return 2
    violations = scan(log_path, root)
    baseline_path = guard_baseline.baseline_path_for(__file__)
    return guard_baseline.apply(violations, baseline_path, args.update_baseline, GUARD_NAME)


if __name__ == "__main__":
    sys.exit(main())
