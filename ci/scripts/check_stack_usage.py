#!/usr/bin/env python3
"""
check_stack_usage.py - Static Stack Frame Limit Guard (AEP-792, #411)

Flags large *automatic* buffers, i.e. arrays that consume stack in a function body.

Fixed in #411: the previous version matched any array declaration with a builtin
element type, so it also reported

  * `static` arrays (file scope / static storage - not stack at all), and
  * struct fields such as `uint8_t prealloc_buffer[4096];` inside a struct body.

Three of the eight violations it reported on the live tree were these false
positives (`g_last_raw_rx[1024]`, `g_acc_history[100]`, and a struct field), which
is why the limit looked unachievable. Block kind is now tracked so only arrays
declared directly in a function body or at file scope count, and file-scope arrays
are reported only when they are not `static`.

A declaration may be opted out in place with a `// stack-ok: <reason>` comment on
the same line. The reason is mandatory - a bare marker does not silence the guard.
"""

import sys
import os
import re
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import guard_baseline

DEFAULT_MAX_STACK_BYTES = 512

ARRAY_DECL = re.compile(
    r"\b(?:uint8_t|int8_t|char|uint16_t|int16_t|uint32_t|int32_t|uint64_t|int64_t|"
    r"float|double|size_t)\s+\**(\w+)\s*\[(\d+)\]\s*(?:\[[^\]]*\])?\s*;"
)

BLOCK_KINDS = ("struct", "union", "enum", "func", "other")


def parse_su_file(su_path, max_bytes):
    violations = []
    with open(su_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 3:
                try:
                    size = int(parts[2])
                except ValueError:
                    continue
                if size > max_bytes:
                    violations.append(
                        {
                            "location": parts[0],
                            "function": parts[1],
                            "size_bytes": size,
                            "rule_id": "SU_FRAME",
                        }
                    )
    return violations


def _element_size(line):
    if "double" in line or "uint64_t" in line or "int64_t" in line or "size_t" in line or "float" not in line and "64" in line:
        return 8
    if "float" in line or "32" in line:
        return 4
    return 1


def _block_kind(prefix):
    tail = prefix.rstrip()
    if re.search(r"\b(struct|union|enum)\b[\w\s]*$", tail):
        return "struct"
    if tail.endswith(")"):
        return "func"
    return "other"


def scan_large_stack_buffers_in_source(file_path, repo_root, max_bytes):
    violations = []
    rel_path = os.path.relpath(file_path, repo_root).replace("\\", "/")

    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    stack = []  # enclosing block kinds
    for line_idx, line in enumerate(lines, 1):
        if line.lstrip().startswith(("//", "*", "/*")):
            continue

        in_storage_class = bool(re.search(r"\b(?:static|extern|register)\b", line))
        match = ARRAY_DECL.search(line)
        enclosing = stack[-1] if stack else "other"

        if match:
            # Struct/union/enum bodies are layout, not stack. `static`/`extern` arrays
            # live in .bss/.data. A file-scope non-static array is still real storage,
            # so it is reported.
            is_layout = enclosing in ("struct", "union", "enum")
            if not is_layout and not in_storage_class:
                symbol = match.group(1)
                count = int(match.group(2))
                total = count * _element_size(line)
                if total > max_bytes:
                    opted_out = re.search(r"//\s*stack-ok:\s*\S", line)
                    if not opted_out:
                        violations.append(
                            {
                                "location": f"{rel_path}:{line_idx}",
                                "function": enclosing,
                                "size_bytes": total,
                                "rule_id": "LARGE_BUFFER",
                                "detail": line.strip()[:70],
                                "symbol": symbol,
                            }
                        )

        # Update block stack after evaluating this line, so a declaration on the same
        # line as `{` is attributed to the enclosing block.
        for pos, ch in enumerate(line):
            if ch == "{":
                stack.append(_block_kind(line[:pos]))
            elif ch == "}" and stack:
                stack.pop()

    return violations


def check_stack_usage(target_dir, max_bytes=DEFAULT_MAX_STACK_BYTES):
    violations = []
    su_found = False
    for root, _, files in os.walk(target_dir):
        for f in files:
            if f.endswith(".su"):
                su_found = True
                violations.extend(parse_su_file(os.path.join(root, f), max_bytes))

    is_fixture_root = os.path.basename(target_dir).startswith(("good_", "bad_"))

    if not su_found or "guards" in target_dir:
        for root, _, files in os.walk(target_dir):
            if "third-party" in root:
                continue
            # Guard fixtures are validated by the refutation suite, never as repo code.
            if "guards" + os.sep + "fixtures" in root and not is_fixture_root:
                continue
            if "tests" in root and "guards" not in root:
                continue
            for f in files:
                if f.endswith((".c", ".cpp")):
                    violations.extend(
                        scan_large_stack_buffers_in_source(
                            os.path.join(root, f), target_dir, max_bytes
                        )
                    )
    return violations


def main():
    parser = argparse.ArgumentParser(description="EdgeOS Static Stack Frame Limit Guard")
    parser.add_argument("--root", default=".", help="Root directory or build dir to inspect")
    parser.add_argument("--max-bytes", type=int, default=DEFAULT_MAX_STACK_BYTES)
    parser.add_argument("--baseline", default=None)
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()

    root_dir = os.path.abspath(args.root)
    violations = check_stack_usage(root_dir, args.max_bytes)

    for v in violations:
        v["fingerprint"] = guard_baseline.fingerprint(
            v["location"].rsplit(":", 1)[0], v["rule_id"], v.get("symbol", "")
        )
        v["issue"] = "#412"
        v["detail"] = f"{v['size_bytes']} bytes  {v.get('detail', '')}"

    baseline_path = args.baseline or guard_baseline.baseline_path_for(__file__)
    is_fixture = os.path.basename(root_dir).startswith(("good_", "bad_"))
    return guard_baseline.apply(
        violations,
        baseline_path,
        args.update_baseline,
        "STACK_USAGE_GUARD",
        allow_missing_baseline=is_fixture,
    )


if __name__ == "__main__":
    sys.exit(main())
