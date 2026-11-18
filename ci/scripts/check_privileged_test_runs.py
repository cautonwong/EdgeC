#!/usr/bin/env python3
"""A ctest suite that needs root must never also run without sudo.

`app_platform_e2e` starts the product through edgeos-launcher, which rejects a
non-root caller (SO_PEERCRED). CI therefore runs it in a `sudo` step. The ASAN
job ran the same `-R` directly as well, and that duplicate kept the job red
until it was removed.
"""

import argparse
import os
import re
import sys

R_ARG = re.compile(r"-R\s+(?:'([^']*)'|\"([^\"]*)\"|(\S+))")


def commands(text):
    """Split a workflow into shell commands, joining backslash continuations."""
    joined = text.replace("\\\n", " ")
    out = []
    for line in joined.splitlines():
        stripped = line.strip()
        if stripped and not stripped.startswith("#"):
            out.append(stripped)
    return out


def suite_names(command):
    if "ctest" not in command:
        return []
    names = []
    for match in R_ARG.finditer(command):
        pattern = next(group for group in match.groups() if group)
        body = pattern.strip("^$").strip("()")
        names.extend(part for part in body.split("|") if part)
    return names


def scan(root):
    workflows = os.path.join(root, ".github", "workflows")
    if not os.path.isdir(workflows):
        return []
    violations = []
    for name in sorted(os.listdir(workflows)):
        if not name.endswith((".yml", ".yaml")):
            continue
        path = os.path.join(workflows, name)
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            found = commands(handle.read())
        privileged = set()
        for command in found:
            if "sudo" in command:
                privileged.update(suite_names(command))
        for command in found:
            if "sudo" in command:
                continue
            for suite in suite_names(command):
                if suite in privileged:
                    violations.append(f"{name}: `{suite}` runs with sudo elsewhere; "
                                      f"this line runs it without: {command}")
    return violations


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    violations = scan(os.path.abspath(args.root))
    if violations:
        print("[PRIVILEGED_TEST_RUNS] a root-only suite is also run unprivileged:")
        for item in violations:
            print("  " + item)
        return 1
    print("[PRIVILEGED_TEST_RUNS] root-only suites run only under sudo.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
