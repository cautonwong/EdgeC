#!/usr/bin/env python3
"""Product build graph guard: no test doubles or test sources in a product."""

import argparse
import os
import re
import sys

FORBIDDEN = ("fake_ports", "tests/", "mock", "demo_stub")
BLOCK_RE = re.compile(
    r"target_(?:link_libraries|sources)\s*\((.*?)\)", re.MULTILINE | re.DOTALL
)


def scan(root):
    violations = []
    products = os.path.join(root, "cmd")
    for product in sorted(os.listdir(products)) if os.path.isdir(products) else []:
        cmake = os.path.join(products, product, "CMakeLists.txt")
        if not os.path.exists(cmake):
            continue
        with open(cmake, encoding="utf-8", errors="ignore") as handle:
            text = handle.read()
        for block in BLOCK_RE.findall(text):
            for token in FORBIDDEN:
                if token in block:
                    violations.append(f"cmd/{product}/CMakeLists.txt|{token}")
    return violations


def main():
    parser = argparse.ArgumentParser(description="Check product link graphs for test doubles.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    violations = scan(root)
    if violations:
        print("❌ [PRODUCT_BUILD_GRAPH_GUARD] violations:")
        for item in violations:
            print(f"   - {item}")
        return 1

    print("✅ [PRODUCT_BUILD_GRAPH_GUARD] clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
