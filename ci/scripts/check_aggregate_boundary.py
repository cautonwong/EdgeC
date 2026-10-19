#!/usr/bin/env python3
"""
check_aggregate_boundary.py - Aggregate & Aggregate Root Boundary Encapsulation Guard (AEP-793)

Enforces:
1. Aggregate Root Encapsulation: Private internal structures and fields of aggregate roots
   (e.g., datahub_t, stream_engine_t, dcu_node_t) must NOT be dereferenced outside their owning module.
2. Direct Pointer / Mutex Penetration: External code must use public atomic APIs instead of
   peeking into internal fields like hub->ring, hub->listeners, engine->stages, or node->*.
3. Single Source of Truth: Aggregate invariants are guarded exclusively by the aggregate root.
"""

import sys
import os
import re
import argparse

# Rules: (aggregate_name, forbidden_dereference_pattern, allowed_path_regex, error_msg)
AGGREGATE_RULES = [
    (
        "datahub_t",
        re.compile(r'\b(?:hub|datahub)\s*->\s*(?:ring|listeners|io_pool|lock|topic_tree)\b'),
        re.compile(r'^(?:modules/edge_core/src/datahub.*\.c|modules/edge_mcp/|tests/test_datahub.*\.c|tests/guards/fixtures/)'),
        "Direct dereference of datahub_t internal fields is forbidden outside edge_core/src/datahub. Use datahub_* API."
    ),
    (
        "stream_engine_t",
        re.compile(r'\b(?:engine|stream_engine)\s*->\s*(?:stages|pipelines|ring_buf|loop)\b'),
        re.compile(r'^(?:modules/edge_stream/src/|tests/test_stream.*\.c|tests/guards/fixtures/)'),
        "Direct dereference of stream_engine_t internal fields is forbidden outside edge_stream. Use stream_engine_* API."
    ),
    (
        "dcu_node_t",
        re.compile(r'\b(?:node|dcu_node)\s*->\s*(?:inventory|poller|router|storage|root)\b'),
        re.compile(r'^(?:cmd/dcu/dcu_node.*\.c|tests/test_dcu.*\.c|tests/guards/fixtures/)'),
        "Direct dereference of dcu_node_t internal fields is forbidden outside cmd/dcu/dcu_node.c. Use dcu_node_* API."
    ),
    (
        "iems_node_t",
        re.compile(r'\b(?:iems_node\s*->\s*(?:bms_cluster|pcs_ctrl|coordinator|fire_safety|pcs_dispatcher|comp_root|hub)|node\s*->\s*(?:bms_cluster|pcs_ctrl|coordinator|fire_safety|pcs_dispatcher))\b'),
        re.compile(r'^(?:cmd/iems/iems_node.*\.c|tests/test_iems.*\.c|tests/guards/fixtures/)'),
        "Direct dereference of iems_node_t internal fields is forbidden outside cmd/iems/iems_node.c. Use iems_node_* API."
    ),
    (
        "control_write_bypass",
        re.compile(r'\bdatahub_publish\s*\([^)]*IEMS_TAG_PCS_CONTROL'),
        re.compile(r'^(?:cmd/iems/iems_node.*\.c|tests/|tests/guards/fixtures/)'),
        "Control write bypass detected: direct datahub_publish of IEMS_TAG_PCS_CONTROL is forbidden outside aggregate root (cmd/iems/iems_node.c). Use iems_node_dispatch_power()."
    ),
    (
        "raw_southbound_write_bypass",
        re.compile(r'\bedge_api_write_tag\s*\('),
        re.compile(r'^(?:modules/|include/|tests/|tests/guards/fixtures/|cmd/iems/iems_app\.c|cmd/dcu/dcu_driver_adapter\.c|cmd/iems/iems_driver_adapter\.c)'),
        "Direct edge_api_write_tag in product business logic is forbidden outside authorized app assembler/adapter. Control writes must pass through aggregate root domain operations."
    )
]

PRUNE_DIRS = {".git", ".local", ".venv", ".vscode", "build", "third-party", "node_modules", "docs", "web"}

def check_file(file_path, repo_root):
    rel_path = os.path.relpath(file_path, repo_root).replace('\\', '/')
    violations = []
    
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return violations

    for agg_name, pattern, allowed_re, err_msg in AGGREGATE_RULES:
        if allowed_re.search(rel_path):
            continue
        for line_idx, line in enumerate(lines, 1):
            clean_line = re.sub(r'//.*', '', line)
            clean_line = re.sub(r'/\*.*?\*/', '', clean_line)
            if pattern.search(clean_line):
                violations.append({
                    "file": rel_path,
                    "line": line_idx,
                    "content": line.strip(),
                    "rule": f"[{agg_name}] {err_msg}"
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
    parser = argparse.ArgumentParser(description="EdgeOS Aggregate Root Boundary Encapsulation Guard")
    parser.add_argument("--root", default=".", help="Root directory to check")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_directory(root_dir)

    if violations:
        print(f"❌ [AGGREGATE_BOUNDARY_GUARD] Found {len(violations)} aggregate encapsulation violation(s):")
        for v in violations:
            print(f"  - {v['file']}:{v['line']} => {v['content']}\n    Rule: {v['rule']}")
        return 1
    else:
        print("✅ [AGGREGATE_BOUNDARY_GUARD] All aggregate root boundaries are strictly encapsulated.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
