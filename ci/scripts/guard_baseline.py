#!/usr/bin/env python3
"""
guard_baseline.py - Shrink-only violation ledger shared by all architecture guards (#411)

Retrofitting a guard onto an existing codebase produces a large first-pass violation
set. Deleting the guard is not an option; letting it stay silent is what caused the
audit finding in #410. The standard answer is a ratchet:

  * `ci/baseline/<guard>.txt` pins the violations that exist today.
  * A guard fails when it finds a violation that is NOT in the baseline (new debt).
  * A guard warns when a baseline entry no longer reproduces (fixed debt) so the
    ledger is pulled downwards in the same PR that removes the violation.
  * An entry may carry an owning-issue tag so debt is traceable, never anonymous.

Fingerprint format (line-shift tolerant on purpose):

    <rel_path>|<RULE_ID>|<scope>[|<issue>]

`scope` is a stable symbol: a function name, an included header, or a type name.
Line numbers are deliberately excluded - they churn on every unrelated edit and
would turn the ledger into noise.

When no baseline file exists the guard is strict: every violation fails the build.
That keeps the refutation fixtures in tests/guards honest.
"""

from __future__ import annotations

import os
import sys


def fingerprint(rel_path: str, rule_id: str, scope: str = "") -> str:
    return f"{rel_path}|{rule_id}|{scope}"


def baseline_path_for(script_file: str) -> str:
    """ci/scripts/check_x.py -> ci/baseline/check_x.txt"""
    repo_root = os.path.abspath(os.path.join(os.path.dirname(script_file), "..", ".."))
    name = os.path.splitext(os.path.basename(script_file))[0]
    return os.path.join(repo_root, "ci", "baseline", name + ".txt")


def load_baseline(path: str) -> dict[str, str]:
    """Return {fingerprint: issue_tag}. Missing file -> empty ledger."""
    entries: dict[str, str] = {}
    if not os.path.exists(path):
        return entries
    with open(path, "r", encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split("|")
            if len(parts) < 3:
                continue
            key = "|".join(parts[:3])
            entries[key] = parts[3] if len(parts) > 3 else ""
    return entries


def load_limits(script_file: str) -> dict[str, int]:
    """{guard_name: max_entries} from ci/baseline/limits.txt.

    Hand-maintained on purpose: raising a cap is an architecture decision that has to
    show up in review. Without it `--update-baseline` could silently grow the debt it
    is supposed to cap.
    """
    repo_root = os.path.abspath(os.path.join(os.path.dirname(script_file), "..", ".."))
    path = os.path.join(repo_root, "ci", "baseline", "limits.txt")
    limits: dict[str, int] = {}
    if not os.path.exists(path):
        return limits
    with open(path, "r", encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            try:
                limits[key.strip()] = int(value.strip())
            except ValueError:
                continue
    return limits


def _cap_for(baseline_path: str, guard_name: str):
    """Look the guard's ceiling up via its sibling check_x.py script."""
    script = os.path.join(
        os.path.dirname(baseline_path),
        "..",
        "scripts",
        os.path.basename(baseline_path)[: -len(".txt")] + ".py",
    )
    return load_limits(os.path.abspath(script)).get(guard_name)


def write_baseline(path: str, violations: list[dict]) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    seen: dict[str, str] = {}
    for v in violations:
        seen.setdefault(v["fingerprint"], v.get("issue", ""))
    lines = [
        "# Shrink-only violation ledger (#411). Regenerate with --update-baseline.",
        "# Format: <path>|<RULE_ID>|<scope>[|<owning-issue>]",
        "# This file may only shrink. Removing an entry is done by fixing the code.",
    ]
    for key in sorted(seen):
        tag = seen[key]
        lines.append(f"{key}|{tag}" if tag else key + "|")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")


def apply(
    violations: list[dict],
    baseline_path: str,
    update: bool,
    guard_name: str,
    allow_missing_baseline: bool = False,
) -> int:
    """Compare violations against the ledger and return a process exit code."""
    cap = _cap_for(baseline_path, guard_name)

    if update:
        unique = _dedup(violations)
        if cap is not None and len(unique) > cap:
            print(
                f"❌ [{guard_name}] refusing to write {len(unique)} entries: the ledger cap is {cap} "
                "(ci/baseline/limits.txt). Fix the code, or raise the cap deliberately in review."
            )
            return 1
        write_baseline(baseline_path, violations)
        print(f"📝 [{guard_name}] ledger written: {baseline_path} ({len(unique)} entries)")
        return 0

    ledger = load_baseline(baseline_path)

    if not ledger and not os.path.exists(baseline_path) and not allow_missing_baseline:
        # Strict mode: no ledger means every violation is new debt.
        if violations:
            _print_violations(guard_name, violations, "no ledger - strict mode")
            return 1
        print(f"✅ [{guard_name}] No violations.")
        return 0

    new = [v for v in violations if v["fingerprint"] not in ledger]
    stale = [k for k in ledger if k not in {v["fingerprint"] for v in violations}]

    if stale:
        print(
            f"⚠️  [{guard_name}] {len(stale)} ledger entr{'y' if len(stale) == 1 else 'ies'} "
            "no longer reproduce - run with --update-baseline to shrink the ledger:"
        )
        for key in sorted(stale)[:20]:
            print(f"   - {key}")
        if len(stale) > 20:
            print(f"   ... and {len(stale) - 20} more")

    if new:
        _print_violations(guard_name, new, f"{len(new)} NEW violation(s) vs ledger")
        return 1

    if cap is not None and len(ledger) > cap:
        print(
            f"❌ [{guard_name}] ledger holds {len(ledger)} entries but the cap is {cap} "
            "(ci/baseline/limits.txt)."
        )
        return 1

    print(
        f"✅ [{guard_name}] No new violations. "
        f"{len(ledger) - len(stale)} tracked debt item(s) remain pinned."
    )
    return 0


def _dedup(violations: list[dict]) -> list[dict]:
    seen = {}
    for v in violations:
        seen.setdefault(v["fingerprint"], v)
    return list(seen.values())


def _print_violations(guard_name: str, violations: list[dict], header: str) -> None:
    unique = _dedup(violations)
    print(f"❌ [{guard_name}] {header}: {len(unique)} violation(s)")
    for v in unique[:80]:
        loc = v.get("location", v.get("file", "?"))
        detail = v.get("detail", "")
        print(f"  - {loc} => {v['rule_id']}: {detail}")
    if len(unique) > 80:
        print(f"  ... and {len(unique) - 80} more")
