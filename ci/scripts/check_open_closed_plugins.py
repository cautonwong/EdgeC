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
    for v in analyze_switch_cases(lines):
        violations.append({
            "file": rel_path,
            "line": v["line"],
            "content": f"Monolithic switch block with {v['case_count']} cases detected",
            "rule": "OCP: Replace hardcoded monolithic switch with table-driven lookup (LUT) or driver ops registration."
        })
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
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_directory(root_dir)

    if violations:
        print(f"❌ [OCP_PLUGIN_GUARD] Found {len(violations)} Open-Closed Principle violation(s):")
        for v in violations:
            print(f"  - {v['file']}:{v['line']} => {v['content']}\n    Rule: {v['rule']}")
        return 1
    else:
        print("✅ [OCP_PLUGIN_GUARD] All core dispatchers satisfy Open-Closed Principle.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
