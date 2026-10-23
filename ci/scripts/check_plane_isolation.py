#!/usr/bin/env python3
"""
check_plane_isolation.py - Four-Plane Dependency Topology & Layer Guard (AEP-792, #411)

Enforces unidirectional plane isolation.

Two rule families run together:

1. PATH_PATTERN rules (legacy, path-only). They fire even when the included header
   does not exist in the tree, which is what keeps the refutation fixture
   `tests/guards/fixtures/bad_plane_isolation` meaningful.

2. CROSS_CONTEXT rules (owner-aware). Every header under `modules/<m>/include/**`
   is owned by module `<m>`; every header under `include/**` is shared (ROOT).
   A source file in `modules/<m>/` may only include headers it owns, headers
   owned by a module listed in MODULE_ALLOWED[<m>], or shared ROOT headers.

Rule 2 is what the previous version of this guard could not see: the codebase
includes headers by bare basename (`#include "datahub.h"`), so a regex on
`edge_stream/...` never matched anything (#410 audit §6.3).
"""

import sys
import os
import re
import glob
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

# ---------------------------------------------------------------------------
# Family 1: path-only patterns (must keep firing on non-existent headers)
# ---------------------------------------------------------------------------
PATH_RULES = [
    (
        r"^modules/edge_core/",
        r'#include\s+["<](?:edge_algo/|edge_stream/|driver_.*\.h|cmd/|products/)',
        "CORE_UPWARD",
        "Foundation (edge_core) cannot include Compute/Execution/Product headers",
    ),
    (
        r"^modules/edge_algo/",
        r'#include\s+["<](?:drivers/|cmd/|driver_radar_fmcw\.h|dcu_.*\.h)',
        "ALGO_SOUTHBOUND",
        "Compute (edge_algo) cannot include Southbound concrete driver headers",
    ),
    (
        # The drivers live in modules/edge_io/src/drivers and modules/edge_io/src/driver_*.c.
        # The previous pattern pointed at modules/edge_io/drivers, a directory that does
        # not exist, so this rule could never match (#410 audit §6.3).
        r"^modules/edge_io/(?:src/)?(?:drivers/|driver_)",
        r'#include\s+["<](?:cmd/|iems_.*\.h|dcu_soe_.*\.h|smartbox_.*\.h)',
        "DRIVER_UPWARD",
        "Southbound Execution Drivers cannot depend on Application-level product headers",
    ),
]

# ---------------------------------------------------------------------------
# Family 2: module ownership allow-list
# ---------------------------------------------------------------------------
MODULE_ALLOWED = {
    "edge_core": set(),
    "edge_algo": {"edge_core"},
    "edge_protocol": {"edge_core"},
    "edge_security": {"edge_core"},
    "edge_stream": {"edge_core", "edge_algo"},
    "edge_infra": {"edge_core"},
    "edge_mgmt": {"edge_core", "edge_infra"},
    "edge_io": {
        "edge_core",
        "edge_algo",
        "edge_protocol",
        "edge_stream",
        "edge_security",
        "edge_infra",
        "edge_mgmt",
    },
    "edge_mcp": {"edge_core", "edge_io", "edge_stream", "edge_infra"},
    "edge_bootstrap": {"edge_core", "edge_infra"},
}

PRUNE_DIRS = {
    ".git",
    ".local",
    ".venv",
    ".vscode",
    ".code-review-graph",
    "build",
    "third-party",
    "node_modules",
    "docs",
    "web",
    "docker",
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')


def _build_owner_map(repo_root):
    """basename -> set(owner_module | 'ROOT')"""
    owner = {}
    for h in glob.glob(os.path.join(repo_root, "modules", "*", "include", "**", "*.h"), recursive=True):
        rel = os.path.relpath(h, repo_root).replace("\\", "/")
        mod = rel.split("/")[1]
        owner.setdefault(os.path.basename(h), set()).add(mod)
    for h in glob.glob(os.path.join(repo_root, "include", "**", "*.h"), recursive=True):
        owner.setdefault(os.path.basename(h), set()).add("ROOT")
    return owner


def _module_of(rel_path):
    if rel_path.startswith("modules/"):
        return rel_path.split("/")[1]
    return None


def check_file(file_path, repo_root, owner_map):
    rel_path = os.path.relpath(file_path, repo_root).replace("\\", "/")
    violations = []

    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as fh:
            lines = fh.readlines()
    except OSError:
        return violations

    # Family 1
    for src_re, inc_re, rule_id, desc in PATH_RULES:
        if not re.search(src_re, rel_path):
            continue
        for idx, line in enumerate(lines, 1):
            if re.search(inc_re, line):
                violations.append(
                    {
                        "file": rel_path,
                        "location": f"{rel_path}:{idx}",
                        "rule_id": rule_id,
                        "detail": f"{line.strip()}  ({desc})",
                        "fingerprint": guard_baseline.fingerprint(rel_path, rule_id, line.strip()),
                        "issue": "#414",
                    }
                )

    # Family 2
    mod = _module_of(rel_path)
    if mod:
        # Unknown modules default to an empty allow-list: a new module must declare its
        # dependencies in MODULE_ALLOWED instead of silently escaping the guard.
        allowed = MODULE_ALLOWED.get(mod, set())
        for idx, line in enumerate(lines, 1):
            m = INCLUDE_RE.match(line)
            if not m:
                continue
            header = os.path.basename(m.group(1).replace("\\", "/"))
            owners = owner_map.get(header)
            if not owners:
                continue
            if mod in owners or "ROOT" in owners:
                continue
            if owners & allowed:
                continue
            violation = {
                "file": rel_path,
                "location": f"{rel_path}:{idx}",
                "rule_id": "CROSS_CONTEXT",
                "detail": f"{mod} -> {'/'.join(sorted(owners))} via {header}",
                "fingerprint": guard_baseline.fingerprint(rel_path, "CROSS_CONTEXT", header),
                "issue": "#414",
            }
            violations.append(violation)

    return violations


def check_directory(target_dir):
    target_dir = os.path.abspath(target_dir)
    owner_map = _build_owner_map(target_dir)
    violations = []
    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if not d.startswith(".") and d not in PRUNE_DIRS]
        if "fixtures" in root and not os.path.basename(target_dir).startswith(("good_", "bad_")):
            continue
        if "tests" in root and "guards" not in root:
            continue
        for name in files:
            if name.endswith((".c", ".h", ".cpp", ".hpp")):
                violations.extend(check_file(os.path.join(root, name), target_dir, owner_map))
    return violations


def main():
    parser = argparse.ArgumentParser(description="EdgeOS Plane Isolation & Layer Guard")
    parser.add_argument("--root", default=".", help="Root directory to check")
    parser.add_argument("--baseline", default=None, help="Ledger path (default: ci/baseline/<guard>.txt)")
    parser.add_argument("--update-baseline", action="store_true", help="Rewrite the ledger from current violations")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_directory(root_dir)
    baseline_path = args.baseline or guard_baseline.baseline_path_for(__file__)

    # Fixture directories are validated strictly, never against the repo ledger.
    is_fixture = os.path.basename(root_dir).startswith(("good_", "bad_"))
    return guard_baseline.apply(
        violations,
        baseline_path,
        args.update_baseline,
        "PLANE_ISOLATION_GUARD",
        allow_missing_baseline=is_fixture,
    )


if __name__ == "__main__":
    sys.exit(main())
