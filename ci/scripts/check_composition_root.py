#!/usr/bin/env python3
"""
check_composition_root.py - Composition Root Exclusivity & Assembly Layer Guard (AEP-794)

Enforces:
1. Assembly Layer Exclusivity: Only the dedicated bootstrap layer (modules/edge_bootstrap/),
   product main entry points (cmd/*/main.c), and tests may include edge_composition_root.h
   or instantiate system-wide composition roots.
2. Anti-Pseudo-Root: Domain logic, algorithms (edge_algo), protocols (edge_protocol),
   and Southbound drivers (edge_io) must remain pure consumers and never act as composition roots.
"""

import sys
import os
import re
import argparse

ALLOWED_ROOT_PATHS = re.compile(
    r'^(?:modules/edge_bootstrap/|cmd/.*/main\.c|cmd/[^/]+/[^/]+_node\.[ch]|tests/|fixtures/)'
)

INCLUDE_COMPOSITION_ROOT_PAT = re.compile(
    r'^\s*#\s*include\s+["<].*edge_composition_root\.h[">]'
)

PRUNE_DIRS = {".git", ".local", ".venv", ".vscode", "build", "third-party", "node_modules", "docs", "web"}

def check_file(file_path, repo_root):
    rel_path = os.path.relpath(file_path, repo_root).replace('\\', '/')
    violations = []

    if ALLOWED_ROOT_PATHS.search(rel_path):
        return violations

    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return violations

    for line_idx, line in enumerate(lines, 1):
        if re.search(r'#include\s+["<](?:edgeos/)?edge_composition_root\.h[">]', line):
            violations.append({
                "file": rel_path,
                "line": line_idx,
                "content": line.strip(),
                "rule": "Only bootstrap layer and product entry points (cmd/*/main.c) can include edge_composition_root.h"
            })
        if re.search(r'\bedge_composition_root_(?:init|start|bind)\b', line):
            violations.append({
                "file": rel_path,
                "line": line_idx,
                "content": line.strip(),
                "rule": "Business/domain modules cannot act as pseudo-composition-roots"
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
            if f.endswith(('.c', '.h')):
                full_path = os.path.join(root, f)
                violations.extend(check_file(full_path, target_dir))
    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Composition Root Exclusivity Guard")
    parser.add_argument("--root", default=".", help="Root directory to check")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_directory(root_dir)

    if violations:
        print(f"❌ [COMPOSITION_ROOT_GUARD] Found {len(violations)} composition root isolation violation(s):")
        for v in violations:
            print(f"  - {v['file']}:{v['line']} => {v['content']}\n    Rule: {v['rule']}")
        return 1
    else:
        print("✅ [COMPOSITION_ROOT_GUARD] All composition roots are strictly confined to the assembly layer.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
