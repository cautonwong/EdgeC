#!/usr/bin/env python3
"""
check_no_dynamic_memory.py - Zero heap allocation on real-time execution paths (AEP-792, #411)

Scope: *execution* functions - the callbacks, polling loops and per-event processors that
run on the hot path. Construction is allowed to allocate; execution is not.

Fixed in #411:

  * The function-name allow-list only matched `_step|_update|_process|_eval|_execute|_isr|
    _callback`. Real hot paths in this tree are named `_proc` (e.g.
    `_sink_proc` in standard_stages.c), so a `malloc` + `strdup` pair inside the event
    path was invisible (#410 audit §6.3).
  * Bodies are now brace-matched instead of tracked per line, so a body that opens and
    closes on one line is still scanned.

Allocations inside a *constructor* (`*_create*`, `*_init*`, `*_new*`, `*_load*`,
`*_open*`, `*_alloc*`, `*_ctor*`) are out of scope here and tracked by #412 instead.
"""

import sys
import os
import re
import glob
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

HOTPATH_NAME = re.compile(
    r"\b(\w+_(?:step|update|process|eval|execute|isr|callback|proc|run|handle|tick|dispatch|poll|recv|on_event))\s*\("
)
CONSTRUCTOR_NAME = re.compile(r"\b(\w+_(?:create|init|new|alloc|load|open|ctor)\w*)\s*\(")
ALLOC_CALL = re.compile(r"\b(?:malloc|calloc|realloc|valloc|aligned_alloc|strdup|strndup)\s*\(")

PRUNE_DIRS = {
    ".git",
    ".local",
    ".venv",
    ".vscode",
    ".code-review-graph",
    "build",
    "third-party",
    "node_modules",
    "docs",
    "web",
    "docker",
}


def _iter_function_bodies(text):
    """Yield (name, start_offset, body) for each top-level function definition."""
    for m in re.finditer(r"\b(\w+)\s*\([^;{]*\)\s*\{", text):
        name = m.group(1)
        if name in ("if", "for", "while", "switch", "sizeof", "return"):
            continue
        brace_start = m.end() - 1
        depth = 0
        i = brace_start
        while i < len(text):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        yield name, brace_start, text[brace_start : i + 1]


def check_file(file_path, repo_root):
    rel_path = os.path.relpath(file_path, repo_root).replace("\\", "/")
    violations = []
    try:
        text = open(file_path, "r", encoding="utf-8", errors="ignore").read()
    except OSError:
        return violations

    # Strip line comments so commented-out code cannot trigger the guard.
    stripped = re.sub(r"//[^\n]*", "", text)

    for name, offset, body in _iter_function_bodies(stripped):
        if CONSTRUCTOR_NAME.search(name):
            continue
        if not HOTPATH_NAME.search(name + "("):
            continue
        for alloc in ALLOC_CALL.finditer(body):
            line_no = stripped.count("\n", 0, offset + alloc.start()) + 1
            violations.append(
                {
                    "file": rel_path,
                    "location": f"{rel_path}:{line_no}",
                    "rule_id": "RT_ALLOC",
                    "detail": f"{name}() calls {alloc.group(0)}() on the execution path",
                    "fingerprint": guard_baseline.fingerprint(rel_path, "RT_ALLOC", name),
                    "issue": "#412",
                }
            )
    return violations


def check_dynamic_memory(target_dir):
    target_dir = os.path.abspath(target_dir)
    is_fixture = os.path.basename(target_dir).startswith(("good_", "bad_"))
    violations = []
    for root, dirs, files in os.walk(target_dir):
        dirs[:] = [d for d in dirs if not d.startswith(".") and d not in PRUNE_DIRS]
        if "fixtures" in root and not is_fixture:
            continue
        if "tests" in root and "guards" not in root:
            continue
        for f in files:
            if f.endswith((".c", ".cpp")):
                violations.extend(check_file(os.path.join(root, f), target_dir))
    return violations


def main():
    parser = argparse.ArgumentParser(description="EdgeOS Zero-Dynamic-Memory-on-Hot-Path Guard")
    parser.add_argument("--root", default=".")
    parser.add_argument("--baseline", default=None)
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_dynamic_memory(root_dir)
    baseline_path = args.baseline or guard_baseline.baseline_path_for(__file__)
    is_fixture = os.path.basename(root_dir).startswith(("good_", "bad_"))
    return guard_baseline.apply(
        violations,
        baseline_path,
        args.update_baseline,
        "ZERO_DYNAMIC_MEMORY_GUARD",
        allow_missing_baseline=is_fixture,
    )


if __name__ == "__main__":
    sys.exit(main())
