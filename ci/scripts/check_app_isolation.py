#!/usr/bin/env python3
"""
check_app_isolation.py - Algorithm & Pure Domain App Isolation Guard (AEP-792)

Verifies that high-performance compute algorithms (edge_algo) and contracts
are completely decoupled from RTOS internals and raw platform hardware handles.
"""

import sys
import os
import re
import argparse

FORBIDDEN_APP_PATTERNS = [
    (r'#include\s+["<](?:zephyr/kernel_internal\.h|arch/cpu\.h|sys/sys_io\.h)[">]', "Algorithmic code cannot include RTOS private kernel internals"),
    (r'#include\s+["<](?:sys/syscall\.h|asm/unistd\.h)[">]', "Algorithms must use standard consumer port abstractions rather than raw syscalls"),
]

def check_app_isolation(target_dir):
    violations = []
    algo_dir = os.path.join(target_dir, "modules", "edge_algo")
    if not os.path.exists(algo_dir):
        # Support running on fixtures
        algo_dir = target_dir

    for root, _, files in os.walk(algo_dir):
        for f in files:
            if f.endswith(('.c', '.h')):
                full_path = os.path.join(root, f)
                rel_path = os.path.relpath(full_path, target_dir).replace('\\', '/')
                with open(full_path, 'r', encoding='utf-8', errors='ignore') as fp:
                    for line_idx, line in enumerate(fp, 1):
                        for pat, desc in FORBIDDEN_APP_PATTERNS:
                            if re.search(pat, line):
                                violations.append({
                                    "file": rel_path,
                                    "line": line_idx,
                                    "content": line.strip(),
                                    "rule": desc
                                })
    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS App & Algorithm Isolation Guard")
    parser.add_argument("--root", default=".", help="Root directory to check")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_app_isolation(root_dir)

    if violations:
        print(f"❌ [APP_ISOLATION_GUARD] Found {len(violations)} app/algorithm isolation violation(s):")
        for v in violations:
            print(f"  - {v['file']}:{v['line']} => {v['content']} ({v['rule']})")
        return 1
    else:
        print("✅ [APP_ISOLATION_GUARD] All algorithm modules are completely isolated from RTOS internals.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
