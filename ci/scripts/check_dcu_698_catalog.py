#!/usr/bin/env python3
"""`docs/design/DCU_698_OI_CATALOG.{md,json}` is generated from `docs/698_parsed.md`
by `scripts/extract_dcu_698_catalog.py`. Editing either side alone leaves the
repository with a catalog that no longer describes the standard, and the DCU
object model reads from it.

The generator always comes from this repository. `--root` only selects the
documents to compare, so a temporary copy needs to hold docs/ and nothing else.
"""

import argparse
import os
import subprocess
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
GENERATOR = os.path.join(REPO_ROOT, "scripts", "extract_dcu_698_catalog.py")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isfile(GENERATOR):
        print(f"[DCU_698_CATALOG] missing generator: {GENERATOR}")
        return 1

    proc = subprocess.run(
        [sys.executable, "-I", GENERATOR, "--check", "--root", root],
        capture_output=True,
        text=True,
    )
    if proc.returncode == 0:
        print("[DCU_698_CATALOG] catalog matches docs/698_parsed.md")
        return 0

    print("[DCU_698_CATALOG] catalog is stale; regenerate with "
          "python3 scripts/extract_dcu_698_catalog.py")
    output = (proc.stdout + proc.stderr).strip()
    if output:
        print(output)
    return 1


if __name__ == "__main__":
    sys.exit(main())
