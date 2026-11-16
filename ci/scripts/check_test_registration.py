#!/usr/bin/env python3
"""Test registration guard: every tests/test_*.c must be built or explicitly exempt."""

import argparse
import glob
import os
import re
import sys

EXEMPT = {
    "test_clock_backward.c": "stale: uses removed calendar_task_t/calendar_schedule API; does not compile",
    "test_wasm_signature.c": "standalone CLI tool taking wasm and PEM paths, not a cmocka test",
}


def scan(root):
    tests_dir = os.path.join(root, "tests")
    if not os.path.isdir(tests_dir):
        return [f"tests: directory not found under {root}"]

    cmake_path = os.path.join(tests_dir, "CMakeLists.txt")
    if os.path.exists(cmake_path):
        with open(cmake_path, encoding="utf-8", errors="ignore") as handle:
            text = handle.read()
    else:
        text = ""
    # A commented-out add_executable does not build the test.
    text = re.sub(r"#.*", "", text)

    violations = []
    for path in sorted(glob.glob(os.path.join(tests_dir, "test_*.c"))):
        name = os.path.basename(path)
        if name in EXEMPT or name in text:
            continue
        violations.append(f"tests/{name}: not referenced by tests/CMakeLists.txt")
    return violations


def main():
    parser = argparse.ArgumentParser(
        description="Check that every tests/test_*.c is registered or exempt."
    )
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()

    violations = scan(os.path.abspath(args.root))
    if violations:
        print("❌ [TEST_REGISTRATION_GUARD] unregistered test files:")
        for item in violations:
            print(f"  - {item}")
        return 1

    print("✅ [TEST_REGISTRATION_GUARD] every test file is registered or exempt.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
