#!/usr/bin/env python3
"""
check_stack_usage.py - Static Stack Depth & Frame Limit Guard (AEP-792)

Parses GCC -fstack-usage (.su files) or scans stack buffer declarations,
enforcing maximum stack frame size (<= 512 bytes) on real-time task and ISR paths.
"""

import sys
import os
import re
import argparse

DEFAULT_MAX_STACK_BYTES = 512

def parse_su_file(su_path, max_bytes):
    violations = []
    with open(su_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 3:
                loc = parts[0]
                func_name = parts[1]
                size_str = parts[2]
                try:
                    size = int(size_str)
                    if size > max_bytes:
                        violations.append({
                            "location": loc,
                            "function": func_name,
                            "size_bytes": size,
                            "max_bytes": max_bytes
                        })
                except ValueError:
                    continue
    return violations

def scan_large_stack_buffers_in_source(file_path, repo_root, max_bytes):
    violations = []
    rel_path = os.path.relpath(file_path, repo_root).replace('\\', '/')
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line_idx, line in enumerate(f, 1):
            # Detect local stack array declarations > max_bytes (e.g. char buf[1024]; uint8_t raw[2048];)
            m = re.search(r'\b(?:uint8_t|int8_t|char|uint16_t|int16_t|uint32_t|int32_t|float|double)\s+\w+\[(\d+)\]\s*;', line)
            if m:
                count = int(m.group(1))
                type_size = 8 if ('double' in line) else (4 if ('32' in line or 'float' in line) else 1)
                total_bytes = count * type_size
                if total_bytes > max_bytes:
                    violations.append({
                        "location": f"{rel_path}:{line_idx}",
                        "function": "local_scope",
                        "size_bytes": total_bytes,
                        "max_bytes": max_bytes,
                        "line": line.strip()
                    })
    return violations

def check_stack_usage(target_dir, max_bytes=DEFAULT_MAX_STACK_BYTES):
    violations = []
    
    # 1. Check generated .su files if build dir exists
    build_dir = os.path.join(target_dir, "build")
    if os.path.isdir(build_dir):
        for root, dirs, files in os.walk(build_dir):
            dirs[:] = [d for d in dirs if d not in [".git", "third-party", "subprojects", "CMakeFiles"]]
            for f in files:
                if f.endswith('.su'):
                    violations.extend(parse_su_file(os.path.join(root, f), max_bytes))

    # 2. Source stack buffer declarations scan
    scan_dir = os.path.join(target_dir, "modules", "edge_algo")
    if not os.path.exists(scan_dir) or "guards" in target_dir:
        scan_dir = target_dir

    for root, dirs, files in os.walk(scan_dir):
        dirs[:] = [d for d in dirs if d not in [".git", "build", "third-party"]]
        if "tests" in root and "guards" not in root:
            continue
        for f in files:
            if f.endswith(('.c', '.cpp')):
                violations.extend(scan_large_stack_buffers_in_source(os.path.join(root, f), target_dir, max_bytes))

    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Static Stack Depth & Frame Limit Guard")
    parser.add_argument("--root", default=".", help="Root directory or build dir to inspect")
    parser.add_argument("--max-bytes", type=int, default=DEFAULT_MAX_STACK_BYTES, help="Maximum allowed frame size in bytes")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_stack_usage(root_dir, args.max_bytes)

    if violations:
        print(f"❌ [STACK_USAGE_GUARD] Found {len(violations)} stack frame limit violation(s) (> {args.max_bytes} bytes):")
        for v in violations:
            print(f"  - {v['location']} ({v['function']}): {v['size_bytes']} bytes > limit {v['max_bytes']} bytes")
        return 1
    else:
        print(f"✅ [STACK_USAGE_GUARD] All stack frames satisfy the static limit (<= {args.max_bytes} bytes).")
        return 0

if __name__ == "__main__":
    sys.exit(main())
