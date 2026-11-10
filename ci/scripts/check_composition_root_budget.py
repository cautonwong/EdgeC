#!/usr/bin/env python3
"""Keep composition roots from growing. Existing roots are a shrink-only baseline."""

import argparse
import os
import sys

DEFAULT_BUDGET = 500
LISTING = os.path.join("ci", "composition_roots.txt")
BASELINE = os.path.join("ci", "baseline", "composition_root_budget.txt")


def roots(root):
    path = os.path.join(root, LISTING)
    if not os.path.exists(path):
        return []
    result = []
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if line and not line.startswith("#"):
                result.append(line)
    return result


def line_count(root, rel):
    path = os.path.join(root, rel)
    if not os.path.exists(path):
        return 0
    with open(path, encoding="utf-8", errors="ignore") as handle:
        return len(handle.read().splitlines())


def baseline(root):
    path = os.path.join(root, BASELINE)
    if not os.path.exists(path):
        return {}
    result = {}
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if line and not line.startswith("#"):
                rel, count = line.split("|", 1)
                result[rel] = int(count)
    return result


def main():
    parser = argparse.ArgumentParser(description="Check composition root line budgets.")
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--update-baseline", action="store_true")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    current = {rel: line_count(root, rel) for rel in roots(root)}
    saved = baseline(root)

    if args.update_baseline:
        path = os.path.join(root, BASELINE)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write("# Shrink-only composition root line ledger.\n")
            handle.write("# Format: <path>|<lines>\n")
            for rel in sorted(current):
                handle.write(f"{rel}|{current[rel]}\n")
        print(f"📝 [COMPOSITION_ROOT_BUDGET] ledger written: {path} ({len(current)} roots)")
        return 0

    violations = []
    for rel, count in sorted(current.items()):
        if rel in saved:
            if count > saved[rel]:
                violations.append(f"{rel}|grew {saved[rel]} -> {count}")
        elif count > DEFAULT_BUDGET:
            violations.append(f"{rel}|{count} > {DEFAULT_BUDGET} and has no baseline")

    if violations:
        print("❌ [COMPOSITION_ROOT_BUDGET] violations:")
        for item in violations:
            print(f"   - {item}")
        return 1

    shrunk = [f"{rel} {saved[rel]} -> {value}" for rel, value in current.items()
              if rel in saved and value < saved[rel]]
    if shrunk:
        print("⚠️  [COMPOSITION_ROOT_BUDGET] roots shrank; run --update-baseline: "
              + "; ".join(shrunk))
    print(f"✅ [COMPOSITION_ROOT_BUDGET] clean. {len(current)} root(s).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
