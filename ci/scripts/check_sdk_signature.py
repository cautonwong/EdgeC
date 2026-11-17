#!/usr/bin/env python3
"""The C SDK wrappers must match the WIT host signatures.

`check_app_abi_surface.py` matches function names. This guard compares the
parameter and out-parameter types against the frozen WIT, so a wrapper that
gains, loses, or retypes an argument fails before an app compiles against it.
"""

import argparse
import os
import re
import sys

WIT = os.path.join("sdk", "wit", "app.v1.wit")
HEADER = os.path.join("sdk", "c", "edge_app_sdk.h")

HOST_FN = re.compile(r"^\s*([a-z][a-z0-9-]*):\s*func\(([^)]*)\)", re.MULTILINE)
C_FN = re.compile(r"const\s+char\s*\*\s*(edge_app_[a-z0-9_]+)\s*\(([^;]*?)\)\s*;", re.DOTALL)

# WIT type -> C type used by the SDK mapping.
WIT_TYPES = {
    "string": "const char *",
    "point-handle": "uint32_t",
    "f64": "double",
    "u32": "uint32_t",
    "log-level": "const char *",
    "sample": "const edge_app_sample_t *",
}

# Per-function out-parameter types, in declaration order after the inputs.
EXPECTED_OUT = {
    "point-resolve": ["uint32_t *", "const char **"],
    "point-read": ["edge_app_sample_t *"],
    "point-subscribe": ["uint32_t *"],
    "point-publish": [],
    "log": [],
    "now": ["uint64_t *"],
}

C_NAME = {
    "point-resolve": "edge_app_point_resolve",
    "point-read": "edge_app_point_read",
    "point-subscribe": "edge_app_point_subscribe",
    "point-publish": "edge_app_point_publish",
    "log": "edge_app_log",
    "now": "edge_app_now",
}

# Hand-written ext wrappers have no WIT signature; sdk/abi_status.json and the
# 03 ABI table record their surface instead.
EXT_ONLY = {"edge_app_setting_get", "edge_app_setting_watch"}


def normalize_type(text):
    return re.sub(r"\s+", " ", text).replace("*", " *").replace("  ", " ").strip()


def c_arg_type(arg):
    """`const char **access` -> `const char **`."""
    match = re.match(r"^(.*?)([A-Za-z_]\w*)$", arg.strip())
    return normalize_type(match.group(1) if match else arg)


def parse_wit(path):
    with open(path, encoding="utf-8") as handle:
        text = handle.read()
    start = text.find("interface host")
    if start < 0:
        return {}
    body = text[start:]
    functions = {}
    for match in HOST_FN.finditer(body):
        params = []
        for item in match.group(2).split(","):
            item = item.strip()
            if not item:
                continue
            name, _, wit_type = item.partition(":")
            params.append((name.strip(), wit_type.strip()))
        functions[match.group(1)] = params
    return functions


def parse_header(path):
    with open(path, encoding="utf-8") as handle:
        text = handle.read()
    return {
        match.group(1): [arg.strip() for arg in match.group(2).split(",")]
        for match in C_FN.finditer(text)
    }


def scan(root):
    wit_path = os.path.join(root, WIT)
    header_path = os.path.join(root, HEADER)
    for path in (wit_path, header_path):
        if not os.path.exists(path):
            return [f"{os.path.relpath(path, root)} not found"]
    functions = parse_wit(wit_path)
    wrappers = parse_header(header_path)
    violations = []

    for name, params in functions.items():
        if name not in EXPECTED_OUT:
            violations.append(f"sdk/wit/app.v1.wit: {name} has no signature contract")
            continue
        wrapper = C_NAME.get(name)
        if wrapper not in wrappers:
            violations.append(f"sdk/c/edge_app_sdk.h: missing {wrapper}")
            continue
        args = wrappers[wrapper]
        expected = ["edge_app_sdk_t *"]
        for _, wit_type in params:
            expected.append(WIT_TYPES.get(wit_type, f"<unknown {wit_type}>"))
        expected.extend(EXPECTED_OUT[name])
        if len(args) != len(expected):
            violations.append(
                f"sdk/c/edge_app_sdk.h: {wrapper} has {len(args)} args, expected {len(expected)}"
            )
            continue
        for index, (arg, want) in enumerate(zip(args, expected)):
            got = c_arg_type(arg)
            if got != normalize_type(want):
                violations.append(
                    f"sdk/c/edge_app_sdk.h: {wrapper} arg {index + 1} is '{got}', "
                    f"WIT expects '{normalize_type(want)}'"
                )

    for wrapper in wrappers:
        if wrapper.startswith("edge_app_sdk_") or wrapper in EXT_ONLY:
            continue
        name = next((wit for wit, c in C_NAME.items() if c == wrapper), None)
        if name is None:
            violations.append(f"sdk/c/edge_app_sdk.h: {wrapper} has no WIT host function")
    return violations


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    violations = scan(os.path.abspath(args.root))
    for violation in violations:
        print(violation)
    if violations:
        print(f"❌ [SDK_SIGNATURE] {len(violations)} violation(s)")
        return 1
    print("✅ [SDK_SIGNATURE] C SDK wrappers match the WIT host signatures")
    return 0


if __name__ == "__main__":
    sys.exit(main())
