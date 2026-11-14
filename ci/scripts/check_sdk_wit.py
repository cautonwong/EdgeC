#!/usr/bin/env python3
"""Fail when the hand-written C binding drifts from the frozen WIT contract."""

import argparse
import hashlib
import os
import re
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
WIT_PATH = os.path.join("sdk", "wit", "app.v1.wit")
HEADER_PATH = os.path.join("sdk", "c", "edge_app_sdk.h")
HASH_PATTERN = re.compile(r'#define EDGE_APP_WIT_SHA256 "([0-9a-f]{64})"')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=REPO_ROOT)
    root = os.path.abspath(parser.parse_args().root)
    wit_path = os.path.join(root, WIT_PATH)
    header_path = os.path.join(root, HEADER_PATH)
    try:
        with open(wit_path, "rb") as fp:
            digest = hashlib.sha256(fp.read()).hexdigest()
        with open(header_path, "r", encoding="utf-8") as fp:
            header = fp.read()
    except OSError as exc:
        print(f"check_sdk_wit: {exc}")
        return 1

    match = HASH_PATTERN.search(header)
    if match is None:
        print("check_sdk_wit: EDGE_APP_WIT_SHA256 missing from sdk/c/edge_app_sdk.h")
        return 1
    if match.group(1) != digest:
        print("check_sdk_wit: sdk/c/edge_app_sdk.h is stale")
        print(f"  WIT sha256:    {digest}")
        print(f"  header sha256: {match.group(1)}")
        print("  Update the header after regenerating the hand-written binding.")
        return 1
    print(f"check_sdk_wit: ok ({digest})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
