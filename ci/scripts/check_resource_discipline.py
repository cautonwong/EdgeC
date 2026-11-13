#!/usr/bin/env python3
"""Shrink-only resource discipline counts for module code.

Modules do not parse environment variables; composition roots and edge_bootstrap
own that. Heap and thread counts are ratchets: they may fall, never rise without
a baseline entry in the same change.
"""

import argparse
import re
import sys
from pathlib import Path

SKIP = {".git", "build", "third-party", "node_modules", "docs", "web", "go", "acceptance"}
HEAP = re.compile(r"\b(malloc|calloc|realloc|strdup)\s*\(")
THREAD = re.compile(r"\b(uv_thread_create|pthread_create)\s*\(")
GETENV = re.compile(r"\bgetenv\s*\(")
BASELINE = Path("ci/baseline/resource_discipline.txt")


def module_c_files(root):
    for path in (root / "modules").rglob("*.c"):
        if not any(part in SKIP for part in path.relative_to(root).parts):
            yield path


def count(pattern, paths):
    return sum(len(pattern.findall(p.read_text(encoding="utf-8", errors="ignore"))) for p in paths)


def main():
    parser = argparse.ArgumentParser(description="Check module resource discipline counts.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = Path(args.root).resolve()
    files = list(module_c_files(root))
    env_files = [p for p in files if "edge_bootstrap" not in p.parts]
    counts = {
        "getenv_calls": count(GETENV, env_files),
        "heap_calls": count(HEAP, files),
        "thread_creations": count(THREAD, files),
    }
    baseline = {}
    path = root / BASELINE
    if path.exists():
        for line in path.read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                key, value = line.split("=", 1)
                baseline[key.strip()] = int(value)
    problems = []
    for key, value in counts.items():
        limit = baseline.get(key)
        if limit is None:
            problems.append(f"{key}: no baseline entry (current {value})")
        elif value > limit:
            problems.append(f"{key}: {value} > {limit}")
    if problems:
        print("❌ [RESOURCE_DISCIPLINE] violations:")
        for problem in problems:
            print(f"   - {problem}")
        return 1
    print("✅ [RESOURCE_DISCIPLINE] clean: "
          + ", ".join(f"{key}={value}" for key, value in counts.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
