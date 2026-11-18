#!/usr/bin/env python3
"""Keep the local architecture-gate runner in sync with the CI step.

The x86_64 CI job runs one ordered list of gate commands. A local runner that
silently drops one of them verifies less than CI while looking green.
"""

import argparse
import os
import re
import sys

STEP_NAME = "Run Architecture & Anti-Entropy Gates (x86_64 only)"
SCRIPT_RE = re.compile(r"(?<![\w/])((?:\./)?(?:ci/)?scripts/[A-Za-z0-9_./-]+\.(?:py|sh))")


def ci_step_body(text):
    lines = text.splitlines()
    start = None
    for i, line in enumerate(lines):
        if STEP_NAME in line and line.strip().startswith("- name:"):
            start = i
            break
    if start is None:
        return None
    run_line = None
    for j in range(start, min(start + 8, len(lines))):
        if re.match(r"\s*run:\s*\|", lines[j]):
            run_line = j
            break
    if run_line is None:
        return None
    indent = len(lines[run_line]) - len(lines[run_line].lstrip())
    body = []
    for line in lines[run_line + 1:]:
        if not line.strip():
            continue
        current = len(line) - len(line.lstrip())
        if current <= indent:
            break
        body.append(line.strip())
    return body


def gate_entries(lines):
    entries = []
    for line in lines:
        if line.startswith("#"):
            continue
        for match in SCRIPT_RE.finditer(line):
            path = match.group(1)
            if path.startswith("./"):
                path = path[2:]
            entries.append((path, "STRICT_CORE_GATE=1" in line))
    return entries


def main():
    parser = argparse.ArgumentParser(description="Compare CI gate step with local runner.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    ci_path = os.path.join(root, ".github", "workflows", "ci.yml")
    local_path = os.path.join(root, "ci", "scripts", "run_arch_gates.sh")
    missing = [p for p in (ci_path, local_path) if not os.path.exists(p)]
    if missing:
        print("❌ [CI_GATE_PARITY] missing file(s): " + ", ".join(missing))
        return 1

    ci_body = ci_step_body(open(ci_path, encoding="utf-8").read())
    if ci_body is None:
        print("❌ [CI_GATE_PARITY] cannot find the architecture gate step in ci.yml")
        return 1

    ci_entries = gate_entries(ci_body)
    local_entries = gate_entries(open(local_path, encoding="utf-8").read().splitlines())

    if ci_entries == local_entries:
        print(f"✅ [CI_GATE_PARITY] local runner matches CI ({len(ci_entries)} gates)")
        return 0

    problems = []
    for index, (entry, local) in enumerate(zip(ci_entries, local_entries)):
        if entry != local:
            problems.append(f"  #{index + 1}: CI={entry} local={local}")
    for entry in ci_entries[len(local_entries):]:
        problems.append(f"  missing locally: {entry}")
    for entry in local_entries[len(ci_entries):]:
        problems.append(f"  local-only gate: {entry}")
    print("❌ [CI_GATE_PARITY] local runner drifted from the CI gate step:")
    print("\n".join(problems))
    return 1


if __name__ == "__main__":
    sys.exit(main())
