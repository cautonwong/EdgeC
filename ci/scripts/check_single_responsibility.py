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

FUNC_DEF_PAT = re.compile(
    r'(?m)^[ \t]*(?:static\s+)?(?:inline\s+)?(?:const\s+)?'
    r'[A-Za-z_][A-Za-z0-9_*\s]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*'
    r'\([^;{}]*\)\s*\{'
)
CONTROL_KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "do", "else"}


def _strip_comments_and_strings(text):
    """Replace comments and string/char literals with spaces, preserving newlines."""
    out = list(text)
    i = 0
    n = len(text)
    state = "normal"
    while i < n:
        c = text[i]
        if state == "normal":
            if c == "/" and i + 1 < n and text[i + 1] == "/":
                out[i] = out[i + 1] = " "
                i += 2
                state = "line_comment"
            elif c == "/" and i + 1 < n and text[i + 1] == "*":
                out[i] = out[i + 1] = " "
                i += 2
                state = "block_comment"
            elif c == '"':
                out[i] = " "
                i += 1
                state = "string"
            elif c == "'":
                out[i] = " "
                i += 1
                state = "char"
            else:
                i += 1
        elif state == "line_comment":
            if c == "\n":
                state = "normal"
            else:
                out[i] = " "
            i += 1
        elif state == "block_comment":
            if c == "*" and i + 1 < n and text[i + 1] == "/":
                out[i] = out[i + 1] = " "
                i += 2
                state = "normal"
            else:
                if c != "\n":
                    out[i] = " "
                i += 1
        else:
            if c == "\\" and i + 1 < n:
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if (state == "string" and c == '"') or (state == "char" and c == "'"):
                out[i] = " "
                i += 1
                state = "normal"
            else:
                if c != "\n":
                    out[i] = " "
                i += 1
    return "".join(out)


def _matching_brace(text, open_idx):
    depth = 0
    for i in range(open_idx, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return None


def _check_functions(stripped):
    violations = []
    pos = 0
    while True:
        m = FUNC_DEF_PAT.search(stripped, pos)
        if not m:
            break
        name = m.group(1)
        if name in CONTROL_KEYWORDS:
            pos = m.end()
            continue
        open_idx = m.end() - 1
        close_idx = _matching_brace(stripped, open_idx)
        if close_idx is None:
            break
        start_line = stripped.count("\n", 0, m.start()) + 1
        end_line = stripped.count("\n", 0, close_idx) + 1
        func_len = end_line - start_line + 1
        if func_len > MAX_FUNCTION_LINES:
            violations.append(
                {
                    "line": start_line,
                    "content": f"Function `{name}` is {func_len} lines long (limit {MAX_FUNCTION_LINES})",
                    "rule": "SRP: God-function detected; decompose into single-responsibility static helpers.",
                }
            )
        pos = close_idx + 1
    return violations


def check_file(file_path, repo_root):
    rel_path = os.path.relpath(file_path, repo_root).replace('\\', '/')
    for pat in EXCLUDED_PATTERNS:
        if pat.search(rel_path):
            return []

    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            text = f.read()
    except Exception:
        return []

    violations = []
    line_count = len(text.splitlines())

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
        for v in _check_functions(_strip_comments_and_strings(text)):
            v["file"] = rel_path
            violations.append(v)

    return violations


def check_directory(target_dir):
    violations = []
    # Only skip the fixture tree when scanning the repository. When the target
    # *is* a fixture we must scan it: the old good_*/bad_* name check silently
    # skipped every other fixture, turning the refutation test into a no-op.
    fixture_mode = "/fixtures" in target_dir.replace("\\", "/")
    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if not d.startswith(".") and d not in PRUNE_DIRS]
        if not fixture_mode and "/fixtures/" in "/" + root.replace("\\", "/") + "/":
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
