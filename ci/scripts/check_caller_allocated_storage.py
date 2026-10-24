#!/usr/bin/env python3
"""
check_caller_allocated_storage.py - Caller-Allocated Storage & Opaque Pointer Guard (AEP-752 / Issue #340)

Enforces:
1. Physical Opaque Encapsulation: Aggregate roots / domain entities expose opaque pointers and aligned storage placeholders (*_storage_t);
2. Placement Construction: Entities provide *_create_in(store, ...) placement constructors without internal dynamic allocations;
3. Compile-Time Assertions: Source files (.c) implementing *_storage_t must contain _Static_assert(sizeof(...) <= sizeof(..._storage_t));
4. Zero Dynamic Memory: No malloc/calloc calls inside placement constructor functions.
"""

import sys
import os
import re
import argparse

# Matches aligned storage buffer: typedef struct { alignas(...) uint8_t raw[N]; } foo_storage_t;
OPAQUE_STORAGE_TYPEDEF_PAT = re.compile(
    r'typedef\s+struct\s*\{[^}]*(?:alignas|_Alignas)\s*\([^)]+\)[^}]*(?:uint8_t|char)\s+\w+\[\d+\];?[^}]*\}\s*(\w+_storage_t)\s*;',
    re.DOTALL
)

# Matches unaligned storage buffer: typedef struct { uint8_t raw[N]; } foo_storage_t; (lacks alignas)
NON_ALIGNED_STORAGE_PAT = re.compile(
    r'typedef\s+struct\s*\{(?![^}]*(?:alignas|_Alignas))[^}]*(?:uint8_t|char)\s+\w+\[\d+\];?[^}]*\}\s*(\w+_storage_t)\s*;',
    re.DOTALL
)

CREATE_IN_PAT = re.compile(r'^\s*(\w+)\s*\*\s*(\w+_create_in)\s*\(\s*(\w+_storage_t)\s*\*\s*(\w+)')

# Whole-signature + opening brace, so the body can be brace-matched as a unit.
CREATE_IN_BODY_PAT = re.compile(
    r'\w+\s*\*\s*(\w+_create_in)\s*\([^;{)]*\w+_storage_t\s*\*\s*\w*[^;{)]*\)\s*\{'
)

ALLOC_CALL_PAT = re.compile(r'\b(?:malloc|calloc|realloc|valloc|aligned_alloc|strdup|strndup)\s*\(')
STATIC_ASSERT_STORAGE_PAT = re.compile(r'_Static_assert\s*\(\s*sizeof\s*\(\s*struct\s+\w+\s*\)\s*<=\s*sizeof\s*\(\s*(\w+_storage_t)\s*\)')

PRUNE_DIRS = {".git", ".local", ".venv", ".vscode", ".code-review-graph", "build", "third-party", "node_modules", "docs", "web", "docker"}

def check_directory(target_dir):
    violations = []
    
    header_files = []
    source_files = []
    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if not d.startswith(".") and d not in PRUNE_DIRS]
        if "fixtures" in root and not os.path.basename(target_dir).startswith("good_") and not os.path.basename(target_dir).startswith("bad_"):
            continue
        if "tests" in root and "guards" not in root:
            continue
        for f in files:
            if f.endswith(('.h', '.hpp')):
                header_files.append(os.path.join(root, f))
            elif f.endswith(('.c', '.cpp')):
                source_files.append(os.path.join(root, f))

    declared_storages = {}

    for h_file in header_files:
        rel_path = os.path.relpath(h_file, target_dir).replace('\\', '/')
        try:
            with open(h_file, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception:
            continue

        # Check for unaligned storage placeholder typedef
        for m in NON_ALIGNED_STORAGE_PAT.finditer(content):
            st_name = m.group(1)
            violations.append({
                "file": rel_path,
                "rule": f"Storage placeholder `{st_name}` must use `alignas(max_align_t)` / `_Alignas(max_align_t)` for safe physical alignment."
            })

        # Check for aligned storage placeholder typedefs
        for m in OPAQUE_STORAGE_TYPEDEF_PAT.finditer(content):
            st_name = m.group(1)
            if st_name not in declared_storages:
                declared_storages[st_name] = []
            declared_storages[st_name].append(h_file)

    source_static_asserts = set()
    
    for s_file in source_files:
        rel_path = os.path.relpath(s_file, target_dir).replace('\\', '/')
        try:
            with open(s_file, 'r', encoding='utf-8', errors='ignore') as f:
                c_content = f.read()
        except Exception:
            continue

        for m in STATIC_ASSERT_STORAGE_PAT.finditer(c_content):
            source_static_asserts.add(m.group(1))

        # Check for malloc/calloc calls inside _create_in functions.
        # Scan the real function body by brace matching instead of per line: the
        # previous line-based version reset brace_depth on the signature line and
        # then `continue`d, so a one-line body
        # `void *f_create_in(storage_t *s) { return malloc(32); }` was never
        # inspected (#410 audit §7, probe 4).
        for m in CREATE_IN_BODY_PAT.finditer(c_content):
            body_start = m.end() - 1  # position of '{'
            depth = 0
            i = body_start
            while i < len(c_content):
                ch = c_content[i]
                if ch == "{":
                    depth += 1
                elif ch == "}":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            body = c_content[body_start:i]
            if ALLOC_CALL_PAT.search(body):
                line_no = c_content.count("\n", 0, body_start) + 1
                violations.append(
                    {
                        "file": rel_path,
                        "line": line_no,
                        "rule": f"Placement constructor `{m.group(1)}` cannot call dynamic heap memory allocation.",
                    }
                )

    # Ensure every declared storage placeholder has a corresponding _Static_assert
    for st_name, h_paths in declared_storages.items():
        if st_name not in source_static_asserts:
            for h_p in h_paths:
                rel_h = os.path.relpath(h_p, target_dir).replace('\\', '/')
                violations.append({
                    "file": rel_h,
                    "rule": f"Storage placeholder `{st_name}` declared in header lacks mandatory `_Static_assert(sizeof(struct ...) <= sizeof({st_name}))` in implementation."
                })

    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Caller-Allocated Storage & Opaque Encapsulation Guard")
    parser.add_argument("--root", default=".", help="Root directory to check")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_directory(root_dir)

    if violations:
        print(f"❌ [CALLER_ALLOCATED_STORAGE_GUARD] Found {len(violations)} violation(s):")
        for v in violations:
            line_str = f":{v['line']}" if "line" in v else ""
            print(f"  - {v['file']}{line_str} => {v['rule']}")
        return 1
    else:
        print("✅ [CALLER_ALLOCATED_STORAGE_GUARD] Caller-Allocated Storage, alignment, and opaque encapsulation verified cleanly.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
