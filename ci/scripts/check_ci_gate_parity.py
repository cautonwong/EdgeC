#!/usr/bin/env python3
"""Keep the local architecture-gate runner in sync with the CI step.

The x86_64 CI job runs one ordered list of gate commands. A local runner that
silently drops one of them verifies less than CI while looking green. The same
comparison covers the `formal` job: dropping it (or its `--strict`) makes local
and CI formal evidence diverge. Comparing only script paths misses a dropped
argument (`--build`) or a dropped environment assignment (`STRICT_CORE_GATE=1`),
so each gate line is compared as (environment assignments, argv) after shell
quoting is resolved.

Arguments that contain a shell expansion ($...) become wildcards, which is what
lets `build` match `"$BUILD_DIR/sbom"` and the GitHub base-SHA expression match
`"$BASE_REF"`.

Known limit: a wildcard hides which variable feeds an argument, so a local
runner that passes the wrong variable in the same position with the same
argument count still passes. Resolving the variables would duplicate shell
semantics here; the literal arguments, their positions and the count are the
conservative check.
"""

import argparse
import os
import re
import shlex
import sys

STEP_NAME = "Run Architecture & Anti-Entropy Gates (x86_64 only)"
SCRIPT_RE = re.compile(r"^(?:\./)?(?:ci/)?scripts/[A-Za-z0-9_./-]+\.(?:py|sh)$")
ENV_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*=")
EXPANSION_RE = re.compile(r"\$")


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


FORMAL_JOB = "formal"
FORMAL_SCRIPTS = (
    "ci/scripts/check_formal.sh",
    "ci/scripts/check_formal_negative.sh",
    "ci/scripts/check_formal_sync.py",
    "ci/scripts/check_formal_isolation.py",
)


def job_run_lines(text, job):
    """Command lines of the first `run: |` block inside a top-level job."""
    lines = text.splitlines()
    start = None
    for i, line in enumerate(lines):
        if re.match(rf"^  {re.escape(job)}:\s*$", line):
            start = i
            break
    if start is None:
        return None
    block = []
    for line in lines[start + 1:]:
        if line.strip() and len(line) - len(line.lstrip()) <= 2:
            break
        block.append(line)
    body = []
    i = 0
    while i < len(block):
        line = block[i]
        if re.match(r"\s*run:\s*\|\s*$", line):
            indent = len(line) - len(line.lstrip())
            i += 1
            while i < len(block):
                entry = block[i]
                if entry.strip() and len(entry) - len(entry.lstrip()) <= indent:
                    break
                if entry.strip():
                    body.append(entry.strip())
                i += 1
        else:
            i += 1
    return body if body else None


def formal_entries(entries):
    return [
        entry for entry in entries
        if any(token in FORMAL_SCRIPTS for token in entry[1])
    ]


def gate_entries(lines):
    entries = []
    for line in lines:
        if line.startswith("#"):
            continue
        try:
            tokens = shlex.split(line)
        except ValueError:
            tokens = line.split()
        env = []
        while tokens and ENV_RE.match(tokens[0]):
            env.append(tokens.pop(0))
        env.sort()
        argv = []
        gate_script = False
        for token in tokens:
            if token.startswith("./"):
                token = token[2:]
            if SCRIPT_RE.match(token):
                gate_script = True
            argv.append("*" if EXPANSION_RE.search(token) else token)
        if not gate_script:
            continue
        entries.append((tuple(env), tuple(argv)))
    return entries


def describe(entry):
    env, argv = entry
    return " ".join([*env, *argv])


def same_gate(ci_entry, local_entry):
    ci_env, ci_argv = ci_entry
    local_env, local_argv = local_entry
    if ci_env != local_env or len(ci_argv) != len(local_argv):
        return False
    return all(
        ci == local or ci == "*" or local == "*"
        for ci, local in zip(ci_argv, local_argv)
    )


def compare(ci_entries, local_entries, label, problems):
    if len(ci_entries) == len(local_entries) and all(
        same_gate(entry, local) for entry, local in zip(ci_entries, local_entries)
    ):
        return True
    problems.append(f"  {label}:")
    for index, (entry, local) in enumerate(zip(ci_entries, local_entries)):
        if not same_gate(entry, local):
            problems.append(f"    #{index + 1}: CI=[{describe(entry)}] local=[{describe(local)}]")
    for entry in ci_entries[len(local_entries):]:
        problems.append(f"    missing locally: [{describe(entry)}]")
    for entry in local_entries[len(ci_entries):]:
        problems.append(f"    local-only gate: [{describe(entry)}]")
    return False


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

    with open(ci_path, encoding="utf-8") as handle:
        ci_text = handle.read()
    ci_body = ci_step_body(ci_text)
    if ci_body is None:
        print("❌ [CI_GATE_PARITY] cannot find the architecture gate step in ci.yml")
        return 1

    ci_entries = gate_entries(ci_body)
    with open(local_path, encoding="utf-8") as handle:
        local_entries = gate_entries(handle.read().splitlines())

    formal_body = job_run_lines(ci_text, FORMAL_JOB)
    formal_ci = gate_entries(formal_body or [])
    formal_local = formal_entries(local_entries)

    problems = []
    arch_ok = compare(ci_entries, local_entries, "architecture gate step", problems)
    if formal_body is None:
        problems.append("  formal job: missing or has no `run: |` step")
        formal_ok = False
    else:
        formal_ok = compare(formal_ci, formal_local, "formal job", problems)
    if arch_ok and formal_ok:
        print(
            f"✅ [CI_GATE_PARITY] local runner matches CI "
            f"({len(ci_entries)} arch gates, {len(formal_ci)} formal gates)"
        )
        return 0

    print("❌ [CI_GATE_PARITY] local runner drifted from CI:")
    print("\n".join(problems))
    return 1


if __name__ == "__main__":
    sys.exit(main())
