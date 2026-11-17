#!/usr/bin/env python3
"""Every WIT world export must be dispatched by the C SDK.

`point_subscribe` was visible to the surface guard because it is a host
function. `app_on_event` is an export, and `check_app_abi_surface.py` only
parses `interface host`, so an export can be declared and never dispatched.
"""

import argparse
import os
import re
import sys

WORLD_RE = re.compile(r"^\s*world\s+[a-z0-9-]+\s*\{(.*?)^\s*\}", re.MULTILINE | re.DOTALL)
EXPORT_RE = re.compile(r"^\s*export\s+([a-z0-9-]+)\s*:", re.MULTILINE)
DISPATCH_RE = re.compile(r'strcmp\s*\(\s*fn\s*,\s*"([a-z0-9_]+)"\s*\)')


def scan(root):
    wit_path = os.path.join(root, "sdk", "wit", "app.v1.wit")
    sdk_path = os.path.join(root, "sdk", "c", "edge_app_sdk.c")
    for path in (wit_path, sdk_path):
        if not os.path.exists(path):
            return [f"{os.path.relpath(path, root)} not found"]
    with open(wit_path, encoding="utf-8") as handle:
        wit_text = handle.read()
    match = WORLD_RE.search(wit_text)
    body = match.group(1) if match else wit_text
    with open(sdk_path, encoding="utf-8") as handle:
        handled = set(DISPATCH_RE.findall(handle.read()))
    violations = []
    for name in EXPORT_RE.findall(body):
        if name.replace("-", "_") not in handled:
            violations.append(
                f"sdk/wit/app.v1.wit: export {name} is not dispatched by "
                "sdk/c/edge_app_sdk.c (edge_app_sdk_dispatch)"
            )
    return violations


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    violations = scan(os.path.abspath(args.root))
    for violation in violations:
        print(violation)
    if violations:
        print(f"❌ [APP_EXPORT_DISPATCH] {len(violations)} violation(s)")
        return 1
    print("✅ [APP_EXPORT_DISPATCH] every WIT world export is dispatched by the C SDK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
