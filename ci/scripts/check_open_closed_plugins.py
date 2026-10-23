#!/usr/bin/env python3
"""
check_open_closed_plugins.py - Open-Closed Principle (OCP) & Table-Driven Dispatch Guard (AEP-795)

Enforces:
1. Open for Extension, Closed for Modification: Core message routing, driver dispatch,
   and protocol handling must use pluggable registration tables (LUT / Ops Tables) rather than
   monolithic, hardcoded switch-case dispatch blocks.
2. Anti-God-Switch: Restricts monolithic switch statements (> 12 cases) in domain routing logic.
"""

import sys
import os
import re
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

PRUNE_DIRS = {".git", ".local", ".venv", ".vscode", "build", "third-party", "node_modules", "docs", "web", "algo"}
EXCLUDED_FILES = {"quickjs.c", "open62541.c", "sqlite3.c", "simd_cdd_codec.c", "modbus_codec.c", "dlt698_codec.c"}

def analyze_switch_cases(lines):
    violations = []
    in_switch = False
    switch_start_line = 0
    case_count = 0
    brace_depth = 0
    switch_brace_depth = 0

    for idx, line in enumerate(lines, 1):
        clean = re.sub(r'//.*', '', line)
        clean = re.sub(r'/\*.*?\*/', '', clean)

        # Count braces
        brace_depth += clean.count('{') - clean.count('}')

        m_switch = re.search(r'\bswitch\s*\(', clean)
        if m_switch and not in_switch:
            in_switch = True
            switch_start_line = idx
            case_count = 0
            switch_brace_depth = brace_depth

        if in_switch:
            if re.search(r'\bcase\b.*?:', clean):
                case_count += 1
            if brace_depth < switch_brace_depth:
                # Exited switch
                if case_count > 12:
                    violations.append({
                        "line": switch_start_line,
                        "case_count": case_count
                    })
                in_switch = False
                case_count = 0

    return violations

# A CLI with a dozen subcommands is a command table, not a design smell.
STRCMP_DISPATCH_LIMIT = 12


def analyze_strcmp_dispatch(lines):
    """Detect a hardcoded `strcmp(expr, "...")` dispatch chain.

    A chain of string comparisons is the same closed-for-modification smell as a
    monolithic switch, but the switch-only rule could not see it
    (#410 audit §6.3). `pipeline_loader.c` carried 15 such branches while a
    table-driven registry already existed next to it.
    """
    violations = []
    in_func = False
    brace_depth = 0
    func_start = 0
    branches = 0
    func_name = ""

    for idx, line in enumerate(lines, 1):
        clean = re.sub(r"//.*", "", line)
        clean = re.sub(r"/\*.*?\*/", "", clean)

        if not in_func:
            m = re.search(r"^[A-Za-z_][\w \t\*]*?\b(\w+)\s*\([^;]*\)\s*\{", clean)
            if m and not clean.strip().startswith(("if", "for", "while", "switch")):
                in_func = True
                func_name = m.group(1)
                func_start = idx
                branches = 0
                brace_depth = clean.count("{") - clean.count("}")
            continue

        brace_depth += clean.count("{") - clean.count("}")
        if re.search(r'\bstrcmp\s*\(\s*\w+\s*,\s*"', clean):
            branches += 1

        if brace_depth <= 0:
            if branches > STRCMP_DISPATCH_LIMIT:
                violations.append(
                    {
                        "line": func_start,
                        "branch_count": branches,
                        "function": func_name,
                    }
                )
            in_func = False
    return violations


def check_file(file_path, repo_root):
    rel_path = os.path.relpath(file_path, repo_root).replace('\\', '/')
    filename = os.path.basename(file_path)
    if filename in EXCLUDED_FILES or "generated" in rel_path or "test" in rel_path:
        return []

    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return []

    violations = []
    rule_issue = "#415"
    for v in analyze_switch_cases(lines):
        violations.append(
            {
                "file": rel_path,
                "line": v["line"],
                "rule_id": "GOD_SWITCH",
                "content": f"Monolithic switch block with {v['case_count']} cases detected",
                "rule": "OCP: Replace hardcoded monolithic switch with table-driven lookup (LUT) or driver ops registration.",
                "fingerprint": guard_baseline.fingerprint(rel_path, "GOD_SWITCH", ""),
                "issue": rule_issue,
                "location": f"{rel_path}:{v['line']}",
            }
        )
    for v in analyze_strcmp_dispatch(lines):
        violations.append(
            {
                "file": rel_path,
                "line": v["line"],
                "rule_id": "STRCMP_DISPATCH",
                "content": (
                    f"`{v['function']}` dispatches on {v['branch_count']} strcmp() branches"
                ),
                "rule": "OCP: Replace the strcmp chain with a registry/LUT so new cases do not edit this function.",
                "fingerprint": guard_baseline.fingerprint(rel_path, "STRCMP_DISPATCH", v["function"]),
                "issue": rule_issue,
                "location": f"{rel_path}:{v['line']}",
            }
        )
    return violations

def check_directory(target_dir):
    violations = []
    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if not d.startswith(".") and d not in PRUNE_DIRS]
        if "fixtures" in root and not os.path.basename(target_dir).startswith("good_") and not os.path.basename(target_dir).startswith("bad_"):
            continue
        if "tests" in root and "fixtures" not in root and "guards" not in root:
            continue
        for f in files:
            if f.endswith(('.c', '.cpp')):
                full_path = os.path.join(root, f)
                violations.extend(check_file(full_path, target_dir))
    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Open-Closed Principle (OCP) Guard")
    parser.add_argument("--root", default=".", help="Root directory to check")
    parser.add_argument("--baseline", default=None)
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_directory(root_dir)
    baseline_path = args.baseline or guard_baseline.baseline_path_for(__file__)
    is_fixture = os.path.basename(root_dir).startswith(("good_", "bad_"))
    for v in violations:
        v["detail"] = f"{v['content']}  ({v['rule']})"
    return guard_baseline.apply(
        violations,
        baseline_path,
        args.update_baseline,
        "OCP_PLUGIN_GUARD",
        allow_missing_baseline=is_fixture,
    )

if __name__ == "__main__":
    sys.exit(main())
