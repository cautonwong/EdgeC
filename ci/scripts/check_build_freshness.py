#!/usr/bin/env python3
"""Reject a build directory whose artifacts are stale for the current source tree."""

import argparse
import os
import subprocess
import sys

TIMEOUT_SECONDS = 60
PENDING_PREVIEW = 10


def fail(detail):
    print(
        f"❌ [BUILD_FRESHNESS] build artifacts are stale for the current tree: {detail}",
        file=sys.stderr,
    )
    return 1


def cache_values(path):
    values = {}
    with open(path, encoding="utf-8", errors="replace") as handle:
        for line in handle:
            key, sep, value = line.partition("=")
            if sep:
                values[key.strip()] = value.strip()
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".", help="source tree the build must belong to")
    parser.add_argument("--build", required=True, help="configured CMake build directory")
    args = parser.parse_args()

    root = os.path.realpath(args.root)
    cache_path = os.path.join(args.build, "CMakeCache.txt")
    if not os.path.isfile(cache_path):
        return fail(f"{args.build} has no CMakeCache.txt")

    cache = cache_values(cache_path)
    home = cache.get("CMAKE_HOME_DIRECTORY:INTERNAL")
    if home is None:
        return fail(f"{cache_path} has no CMAKE_HOME_DIRECTORY")
    if os.path.realpath(home) != root:
        return fail(f"configured for {home}, current root is {root}")

    generator = cache.get("CMAKE_GENERATOR:INTERNAL")
    if generator != "Ninja":
        return fail(f"generator is {generator!r}, this gate needs Ninja")

    try:
        proc = subprocess.run(
            ["ninja", "-n", "-C", args.build],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=TIMEOUT_SECONDS,
        )
    except FileNotFoundError:
        return fail("ninja is not installed")
    except subprocess.TimeoutExpired:
        return fail(f"ninja -n did not finish within {TIMEOUT_SECONDS}s")

    output = proc.stdout
    if proc.returncode != 0:
        return fail(f"ninja -n failed with rc {proc.returncode}:\n{output.strip()}")
    if "no work to do" not in output:
        pending = [line for line in output.splitlines() if line.strip()]
        preview = "\n".join(pending[:PENDING_PREVIEW]) or "(ninja printed nothing)"
        return fail(f"{args.build} has pending work:\n{preview}")

    print(f"✅ [BUILD_FRESHNESS] {args.build} is current for {root}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
