#!/usr/bin/env python3
"""Component scorecard: rerunnable counts for the refactor's ten axes.

The value of this script is the baseline. Run it before a change and after,
and compare the JSON. It reports; it does not fail the build yet.
"""

import argparse
import json
import re
import sys
from pathlib import Path

SKIP_DIRS = {".git", "build", "third-party", "node_modules", "docs", "web", "go", "acceptance"}

HEAP_RE = re.compile(r"\b(malloc|calloc|realloc|strdup)\s*\(")
THREAD_RE = re.compile(r"\b(uv_thread_create|pthread_create)\s*\(")
GETENV_RE = re.compile(r"\bgetenv\s*\(")
STRUCT_RE = re.compile(r"^[ \t]*struct\s+[A-Za-z_][A-Za-z0-9_]*\s*\{", re.MULTILINE)
APP_LIB_RE = re.compile(r"^[ \t]*target_link_libraries\s*\([^)]*_app_lib", re.MULTILINE)

PRODUCT_CMAKE = [
    Path("cmd/dcu/CMakeLists.txt"),
    Path("cmd/iems/CMakeLists.txt"),
    Path("cmd/smart_box/CMakeLists.txt"),
]
MODULE_NAMES = [
    "edge_bootstrap", "edge_io", "edge_core", "edge_protocol",
    "edge_stream", "edge_infra", "edge_algo", "edge_security",
]


def files(root, pattern, under):
    base = root / under
    if not base.exists():
        return []
    return [
        p for p in base.rglob(pattern)
        if not any(part in SKIP_DIRS for part in p.relative_to(root).parts)
    ]


def read(p):
    try:
        return p.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return ""


def count_matches(paths, regex):
    return sum(len(regex.findall(read(p))) for p in paths)


def count_files_containing(paths, needle):
    return sum(1 for p in paths if needle in read(p))


def include_headers(root):
    module_headers = [
        p for p in files(root, "*.h", "modules")
        if "include" in p.relative_to(root).parts
    ]
    return (module_headers + files(root, "*.h", "include")
            + files(root, "*.h", "foundation/include"))


def lines_in(p):
    if not p.exists():
        return 0
    return len(p.read_text(encoding="utf-8", errors="ignore").splitlines())


def pinned_debt(root):
    total = 0
    for p in sorted((root / "ci/baseline").glob("check_*.txt")):
        for line in read(p).splitlines():
            line = line.strip()
            if line and not line.startswith("#"):
                total += 1
    return total


def composition_root_lines(root):
    listing = root / "ci/composition_roots.txt"
    if not listing.exists():
        return 0
    total = 0
    for line in read(listing).splitlines():
        line = line.strip()
        if line and not line.startswith("#") and line.startswith("cmd/"):
            total += lines_in(root / line)
    return total


def modules_linked_by_all_products(root):
    shared = []
    for module in MODULE_NAMES:
        hits = sum(1 for p in PRODUCT_CMAKE if module in read(root / p))
        if hits == 3:
            shared.append(module)
    return shared


def collect(root):
    c_files = files(root, "*.c", "modules")
    h_files = files(root, "*.h", "modules")
    public_headers = include_headers(root)

    metrics = {
        "datahub_files": {
            "value": count_files_containing(c_files + h_files, "datahub"),
            "unit": "files",
            "evidence": "modules/**/*.{c,h}",
        },
        "public_structs": {
            "value": count_matches(public_headers, STRUCT_RE),
            "unit": "structs",
            "evidence": "modules/*/include/**/*.h, include/**/*.h",
        },
        "heap_calls": {
            "value": count_matches(c_files, HEAP_RE),
            "unit": "calls",
            "evidence": "modules/**/*.c",
        },
        "thread_creations": {
            "value": count_matches(c_files, THREAD_RE),
            "unit": "calls",
            "evidence": "modules/**/*.c",
        },
        "getenv_calls": {
            "value": count_matches(c_files + files(root, "*.c", "cmd"), GETENV_RE),
            "unit": "calls",
            "evidence": "modules/**/*.c, cmd/**/*.c",
        },
        "test_app_lib_links": {
            "value": len(APP_LIB_RE.findall(read(root / "tests/CMakeLists.txt"))),
            "unit": "targets",
            "evidence": "tests/CMakeLists.txt",
        },
        "shared_module_links": {
            "value": len(modules_linked_by_all_products(root)),
            "unit": "modules",
            "evidence": ", ".join(modules_linked_by_all_products(root)),
        },
        "composition_root_lines": {
            "value": composition_root_lines(root),
            "unit": "lines",
            "evidence": "ci/composition_roots.txt",
        },
        "pinned_debt_items": {
            "value": pinned_debt(root),
            "unit": "items",
            "evidence": "ci/baseline/check_*.txt",
        },
    }
    return metrics


def main():
    parser = argparse.ArgumentParser(description="Print the component refactor scorecard.")
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--json", action="store_true", help="emit JSON")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    metrics = collect(root)

    if args.json:
        json.dump(metrics, sys.stdout, indent=2, sort_keys=True)
        sys.stdout.write("\n")
        return 0

    width = max(len(name) for name in metrics)
    print(f"{'axis'.ljust(width)}  {'value':>7}  unit")
    for name in sorted(metrics):
        item = metrics[name]
        print(f"{name.ljust(width)}  {item['value']:>7}  {item['unit']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
