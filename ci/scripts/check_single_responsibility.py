#!/usr/bin/env python3
"""
check_single_responsibility.py - Single Responsibility Principle (SRP) & Code Bloat Guard (AEP-796)

Enforces:
1. File Level SRP: Non-generated domain and algorithm source files (.c) must not exceed 1500 physical lines.
2. Public Contract SRP: Clean interface headers (.h) must not exceed 600 physical lines.
3. Function Level SRP: Individual C functions must not exceed 200 physical lines (prevents God-functions).
"""

import sys
import os
import re
import argparse

PRUNE_DIRS = {".git", ".local", ".venv", ".vscode", "build", "third-party", "node_modules", "docs", "web", "algo"}
EXCLUDED_PATTERNS = [
    re.compile(r'open62541\.[ch]'),
    re.compile(r'quickjs\.[ch]'),
    re.compile(r'sqlite3\.[ch]'),
    re.compile(r'.*generated/'),
    re.compile(r'.*_codec\.[ch]'),
    re.compile(r'contracts/'),
    re.compile(r'tests/')
]

MAX_C_FILE_LINES = 1500
MAX_H_FILE_LINES = 600
MAX_FUNCTION_LINES = 200

FUNC_DEF_PAT = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_* \t]+\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^;]*\)\s*\{')

def check_file(file_path, repo_root):
    rel_path = os.path.relpath(file_path, repo_root).replace('\\', '/')
    for pat in EXCLUDED_PATTERNS:
        if pat.search(rel_path):
            return []

    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return []

    violations = []
    line_count = len(lines)

    # 1. File line limit check
    if file_path.endswith('.c') and line_count > MAX_C_FILE_LINES:
        violations.append({
            "file": rel_path,
            "line": 1,
            "content": f"Source file has {line_count} lines (exceeds limit of {MAX_C_FILE_LINES})",
            "rule": "SRP: Module has too many responsibilities; refactor into focused sub-modules."
        })
    elif file_path.endswith('.h') and line_count > MAX_H_FILE_LINES:
        violations.append({
            "file": rel_path,
            "line": 1,
            "content": f"Header file has {line_count} lines (exceeds limit of {MAX_H_FILE_LINES})",
            "rule": "SRP: Public contract is oversized; segregate into client-specific interfaces."
        })

    # 2. Function line limit check
    if file_path.endswith('.c'):
        in_func = False
        func_name = ""
        func_start_line = 0
        brace_depth = 0

        for idx, line in enumerate(lines, 1):
            clean = re.sub(r'//.*', '', line)
            clean = re.sub(r'/\*.*?\*/', '', clean)

            if not in_func:
                m = FUNC_DEF_PAT.search(line)
                if m and not line.strip().startswith(('if', 'for', 'while', 'switch', 'static_assert', '_Static_assert')):
                    in_func = True
                    func_name = m.group(1)
                    func_start_line = idx
                    brace_depth = clean.count('{') - clean.count('}')
            else:
                brace_depth += clean.count('{') - clean.count('}')
                if brace_depth <= 0:
                    func_len = idx - func_start_line + 1
                    if func_len > MAX_FUNCTION_LINES:
                        violations.append({
                            "file": rel_path,
                            "line": func_start_line,
                            "content": f"Function `{func_name}` is {func_len} lines long (limit {MAX_FUNCTION_LINES})",
                            "rule": "SRP: God-function detected; decompose into single-responsibility static helpers."
                        })
                    in_func = False
                    func_name = ""
                    brace_depth = 0

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
            if f.endswith(('.c', '.h')):
                full_path = os.path.join(root, f)
                violations.extend(check_file(full_path, target_dir))
    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Single Responsibility Principle (SRP) Guard")
    parser.add_argument("--root", default=".", help="Root directory to check")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_directory(root_dir)

    if violations:
        print(f"❌ [SRP_BLOAT_GUARD] Found {len(violations)} Single Responsibility violation(s):")
        for v in violations:
            print(f"  - {v['file']}:{v['line']} => {v['content']}\n    Rule: {v['rule']}")
        return 1
    else:
        print("✅ [SRP_BLOAT_GUARD] All files and functions respect Single Responsibility bounds.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
