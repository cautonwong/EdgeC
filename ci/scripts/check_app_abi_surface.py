#!/usr/bin/env python3
"""Fail when the app host ABI ledger drifts from any recorded surface.

sdk/abi_status.json is the single record of every function in the
sdk/wit/app.v1.wit host interface and where it is wired: C SDK wrapper, host
dispatch status, wasm native symbol, wasm import whitelist, and conformance
vector. A `known_gap` field is a recorded, allowed divergence and prints as a
warning.
"""

import argparse
import json
import os
import re
import sys

LEDGER = "sdk/abi_status.json"
WIT = "sdk/wit/app.v1.wit"
C_SDK = "sdk/c/edge_app_sdk.h"
HOST = "modules/edge_app_host/src/app_host_server.c"
WASM_MAIN = "cmd/app-host/main.c"
WHITELIST = "foundation/src/edge_wasm_imports.c"
VECTORS = "sdk/conformance/vectors.json"

REQUIRED_FIELDS = ("c_sdk", "host", "wasm_symbol", "whitelist")
# shortcut: surfaces are matched by their current syntax. A table-driven
# dispatch or a renamed symbol array needs these patterns updated with it.
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
WIT_FN = re.compile(r"^\s*([a-z][a-z0-9-]*):\s*func\(", re.MULTILINE)
C_SDK_FN = re.compile(r"\b(edge_app_(?:point_[a-z0-9_]+|log|now))\s*\(")
HOST_FN = re.compile(r'strcmp\(fn->valuestring,\s*"([^"]+)"\)')
ENV_IMPORT = re.compile(r'\{\s*"env"\s*,\s*"([^"]+)"\s*\}')


def read(root, rel):
    with open(os.path.join(root, rel), "r", encoding="utf-8") as handle:
        return handle.read()


def strip_comments(text):
    return COMMENTS.sub(lambda match: "\n" * match.group(0).count("\n"), text)


def braced_block(text, marker):
    start = text.index("{", text.index(marker))
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:index]
    raise ValueError(f"unbalanced braces after {marker!r}")


def parse_wit(root):
    host = braced_block(read(root, WIT), "interface host")
    return {match.group(1) for match in WIT_FN.finditer(host)}


def parse_c_sdk(root):
    return {match.group(1) for match in C_SDK_FN.finditer(read(root, C_SDK))}


def parse_host(root):
    """Map dispatch name to 'implemented' or 'not_supported' per branch body."""
    lines = strip_comments(read(root, HOST)).splitlines()
    branches = {}
    index = 0
    while index < len(lines):
        names = HOST_FN.findall(lines[index])
        if not names:
            index += 1
            continue
        while index < len(lines) and ") {" not in lines[index]:
            index += 1
            if index < len(lines):
                names += HOST_FN.findall(lines[index])
        if index >= len(lines):
            raise ValueError("unterminated host dispatch condition")
        body = next((line for line in lines[index + 1:] if line.strip()), "")
        status = "not_supported" if "not_supported" in body else "implemented"
        for name in names:
            branches[name] = status
        index += 1
    return branches


def parse_wasm_symbols(root):
    symbols = braced_block(strip_comments(read(root, WASM_MAIN)), "kSymbols[]")
    return {match.group(1) for match in re.finditer(r'\{\s*"([^"]+)"\s*,', symbols)}


def parse_whitelist(root):
    return set(ENV_IMPORT.findall(read(root, WHITELIST)))


def parse_vectors(root):
    document = json.loads(read(root, VECTORS))
    names = set()

    def walk(node):
        if isinstance(node, dict):
            for key, value in node.items():
                if key == "fn" and isinstance(value, str):
                    names.add(value)
                else:
                    walk(value)
        elif isinstance(node, list):
            for item in node:
                walk(item)

    walk(document)
    return names


def check(root):
    ledger = json.loads(read(root, LEDGER))
    functions = ledger.get("functions")
    extra = ledger.get("extra_host_json", {})
    if not isinstance(functions, dict) or not isinstance(extra, dict):
        return ["sdk/abi_status.json needs `functions` and `extra_host_json` objects"], []

    wit = parse_wit(root)
    c_sdk = parse_c_sdk(root)
    host = parse_host(root)
    wasm_symbols = parse_wasm_symbols(root)
    whitelist = parse_whitelist(root)
    vector_fns = parse_vectors(root)

    errors = []
    warnings = []

    for name in sorted(wit - functions.keys()):
        errors.append(f"WIT host function '{name}' is missing from sdk/abi_status.json")
    for name in sorted(functions.keys() - wit):
        errors.append(f"sdk/abi_status.json records '{name}', which is not a WIT host function")

    claimed_symbols = set()
    for name, entry in sorted(functions.items()):
        missing = [field for field in REQUIRED_FIELDS if field not in entry]
        if missing:
            errors.append(f"'{name}': ledger entry is missing {', '.join(missing)}")
            continue

        expected_c = "edge_app_" + name.replace("-", "_")
        if entry["c_sdk"] != expected_c:
            errors.append(f"'{name}': ledger c_sdk is {entry['c_sdk']!r}, expected {expected_c!r}")
        if expected_c not in c_sdk:
            errors.append(f"'{name}': sdk/c/edge_app_sdk.h does not declare {expected_c}()")

        status = host.get(name.replace("-", "_"))
        if status is None:
            errors.append(f"'{name}': modules/edge_app_host/src/app_host_server.c has no dispatch branch")
        elif entry["host"] != status:
            errors.append(f"'{name}': ledger host is {entry['host']!r}, dispatch is {status!r}")

        symbol = entry["wasm_symbol"]
        if symbol is not None:
            claimed_symbols.add(symbol)
        if symbol is not None and symbol not in wasm_symbols:
            errors.append(f"'{name}': ledger wasm_symbol is {symbol!r}, but cmd/app-host/main.c does not register it")
        if entry["whitelist"] != (symbol in whitelist if symbol else False):
            errors.append(
                f"'{name}': ledger whitelist is {entry['whitelist']!r}, "
                f"foundation/src/edge_wasm_imports.c entry for {symbol!r} is "
                f"{'present' if symbol in whitelist else 'absent'}"
            )

        if entry["host"] == "implemented" and (symbol is None or not entry["whitelist"]):
            errors.append(f"'{name}': host is implemented but the wasm path is missing")

        if (
            entry["host"] == "not_supported"
            and entry.get("conformance_expects") == "success"
            and not entry.get("known_gap")
        ):
            errors.append(
                f"'{name}': not_supported but conformance expects success without a known_gap"
            )

        if entry.get("conformance_expects") is not None and name.replace("-", "_") not in vector_fns:
            errors.append(f"'{name}': ledger records conformance_expects but sdk/conformance/vectors.json never calls it")

        if entry.get("known_gap"):
            warnings.append(f"known gap [{name}]: {entry['known_gap']}")

    # An implemented extra_host_json name is a hand-written wasm import too, so
    # it claims the native symbol and whitelist entry like a WIT function does.
    claimed_symbols |= {name for name, status in extra.items() if status == "implemented"}

    for symbol in sorted(wasm_symbols - claimed_symbols):
        errors.append(f"cmd/app-host/main.c registers '{symbol}', which no ledger function claims")
    for name in sorted(whitelist - claimed_symbols):
        # The whitelist also serves legacy host_*/ext_* imports; only app ABI
        # names (point_*, app_*) must be claimed by the ledger.
        if name.startswith(("point_", "app_")):
            errors.append(
                f"foundation/src/edge_wasm_imports.c whitelists '{name}', which no ledger function claims"
            )
    abi_imports = {name.replace("-", "_") for name in functions}

    wit_dispatch = {name.replace("-", "_") for name in wit}
    for name, status in sorted(extra.items()):
        if name in wit_dispatch:
            errors.append(f"'{name}': recorded under extra_host_json but it is a WIT host function")
        elif name not in host:
            errors.append(f"'{name}': extra_host_json records a name that app_host_server.c does not dispatch")
        elif extra[name] != host[name]:
            errors.append(f"'{name}': ledger extra_host_json is {status!r}, dispatch is {host[name]!r}")
    for name, status in sorted(host.items()):
        if name not in wit_dispatch and name not in extra:
            errors.append(f"host dispatch branch '{name}' is in neither WIT nor extra_host_json")

    for name in sorted(vector_fns):
        if name not in abi_imports:
            errors.append(f"sdk/conformance/vectors.json calls '{name}', which is missing from sdk/abi_status.json")

    return errors, warnings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".")
    root = os.path.abspath(parser.parse_args().root)
    try:
        errors, warnings = check(root)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"check_app_abi_surface: {exc}")
        return 1

    for warning in warnings:
        print(f"check_app_abi_surface: warning: {warning}")
    if errors:
        print(f"check_app_abi_surface: {len(errors)} drift(s)")
        for error in errors:
            print(f"  - {error}")
        return 1
    print(f"check_app_abi_surface: ok ({len(json.loads(read(root, LEDGER))['functions'])} functions)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
