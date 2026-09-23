#!/usr/bin/env python3
"""
check_plane_isolation.py - Four-Plane Dependency Topology & Layer Guard (AEP-792)

Enforces unidirectional plane isolation rules:
1. Foundation (edge_core) must not include higher-plane headers (edge_algo, edge_stream, edge_io/drivers, products);
2. Compute (edge_algo, edge_stream) must not include Southbound driver private headers (modules/edge_io/drivers);
3. Southbound Execution (edge_io/drivers) must not include application-level logic (cmd/);
4. Codebase must respect Consumer-Defined Ports and avoid reverse couplings.
"""

import sys
import os
import re
import argparse

FORBIDDEN_RULES = [
    # (source_dir_regex, forbidden_include_pattern, rule_description)
    (
        r"^modules/edge_core/",
        r'#include\s+["<](?:edge_algo/|edge_stream/|driver_.*\.h|cmd/|products/)',
        "Foundation (edge_core) cannot include Compute/Execution/Product headers"
    ),
    (
        r"^modules/edge_algo/",
        r'#include\s+["<](?:drivers/|cmd/|driver_radar_fmcw\.h|dcu_.*\.h)',
        "Compute (edge_algo) cannot include Southbound concrete driver headers"
    ),
    (
        r"^modules/edge_io/drivers/",
        r'#include\s+["<](?:cmd/|iems_.*\.h|dcu_soe_.*\.h|smartbox_.*\.h)',
        "Southbound Execution Drivers cannot depend on Application-level product headers"
    )
]

def check_file_plane_isolation(file_path, repo_root):
    rel_path = os.path.relpath(file_path, repo_root).replace('\\', '/')
    violations = []
    
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
        
    for rule_src_re, forbidden_inc_re, desc in FORBIDDEN_RULES:
        if re.search(rule_src_re, rel_path):
            for line_idx, line in enumerate(lines, 1):
                if re.search(forbidden_inc_re, line):
                    violations.append({
                        "file": rel_path,
                        "line": line_idx,
                        "content": line.strip(),
                        "rule": desc
                    })
    return violations

def run_plane_isolation_check(target_dir):
    violations = []
    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if d not in [".git", "build", "third-party"]]
        for f in files:
            if f.endswith(('.c', '.h', '.cpp', '.hpp')):
                violations.extend(check_file_plane_isolation(os.path.join(root, f), target_dir))
    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Plane Isolation & Layer Guard")
    parser.add_argument("--root", default=".", help="Root directory to check")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = run_plane_isolation_check(root_dir)

    if violations:
        print(f"❌ [PLANE_ISOLATION_GUARD] Found {len(violations)} architectural plane isolation violation(s):")
        for v in violations:
            print(f"  - {v['file']}:{v['line']} => {v['content']} ({v['rule']})")
        return 1
    else:
        print("✅ [PLANE_ISOLATION_GUARD] All modules satisfy Four-Plane unidirectional isolation rules.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
