#!/usr/bin/env python3
"""
check_no_dynamic_memory.py - Zero Dynamic Memory in Real-time Critical Loops (AEP-792)

Verifies that algorithm step/update functions and real-time critical sections
do not use raw heap allocation (malloc/calloc/realloc/free) in control execution paths.
"""

import sys
import os
import re
import argparse

CRITICAL_FUNCTION_PATTERNS = [
    r'(\w+_(?:step|update|process|eval|execute|isr|callback)\s*\([^)]*\)\s*\{)',
]

DYNAMIC_ALLOC_CALLS = [
    r'\b(?:malloc|calloc|realloc|free|valloc|aligned_alloc)\s*\(',
]

def check_file_dynamic_memory(file_path, repo_root):
    rel_path = os.path.relpath(file_path, repo_root).replace('\\', '/')
    violations = []
    
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Look for functions named *_step / *_update / *_process / *_eval
    lines = content.split('\n')
    in_critical_func = False
    brace_depth = 0
    cur_func_name = ""

    for line_idx, line in enumerate(lines, 1):
        clean_line = re.sub(r'//.*', '', line) # remove line comments
        
        # Check start of critical function
        for pat in CRITICAL_FUNCTION_PATTERNS:
            match = re.search(pat, clean_line)
            if match and not in_critical_func:
                in_critical_func = True
                brace_depth = 0
                cur_func_name = match.group(1).split('(')[0].strip()

        if in_critical_func:
            brace_depth += clean_line.count('{') - clean_line.count('}')
            
            # Check for forbidden dynamic heap calls inside critical function
            for alloc_pat in DYNAMIC_ALLOC_CALLS:
                if re.search(alloc_pat, clean_line):
                    violations.append({
                        "file": rel_path,
                        "line": line_idx,
                        "function": cur_func_name,
                        "content": line.strip(),
                        "rule": "Real-time critical function cannot call dynamic heap memory allocation"
                    })

            if brace_depth <= 0:
                in_critical_func = False

    return violations

def check_dynamic_memory(target_dir):
    violations = []
    scan_dir = os.path.join(target_dir, "modules", "edge_algo")
    if not os.path.exists(scan_dir):
        scan_dir = target_dir

    for root, dirs, files in os.walk(scan_dir):
        dirs[:] = [d for d in dirs if d not in [".git", "build", "third-party"]]
        if "tests" in root and "guards" not in root:
            continue
        for f in files:
            if f.endswith(('.c', '.cpp')):
                violations.extend(check_file_dynamic_memory(os.path.join(root, f), target_dir))
    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Zero Dynamic Memory Guard")
    parser.add_argument("--root", default=".", help="Root directory to check")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_dynamic_memory(root_dir)

    if violations:
        print(f"❌ [ZERO_DYNAMIC_MEMORY_GUARD] Found {len(violations)} dynamic memory violation(s) in critical paths:")
        for v in violations:
            print(f"  - {v['file']}:{v['line']} in `{v['function']}` => {v['content']}")
        return 1
    else:
        print("✅ [ZERO_DYNAMIC_MEMORY_GUARD] Zero dynamic heap memory allocation verified in real-time critical paths.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
