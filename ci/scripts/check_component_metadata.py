#!/usr/bin/env python3
"""Validate component.meta against the machine-readable architecture record."""

import argparse
import os
import re
import sys

REQUIRED = [
    "name", "version", "maturity", "owner", "subdomain", "language", "abi_stability",
    "foundation", "ports_required", "allowed_owners", "rom_bytes_max", "ram_bytes_max",
    "stack_bytes_max", "heap", "lts",
]
MATURITY = {"incubating", "stable", "maintenance", "deprecated"}
SUBDOMAIN = {"core", "supporting"}
ABI = {"source-only", "abi-stable"}
HEAP = {"none", "bounded"}
SEMVER = re.compile(r"^\d+\.\d+\.\d+$")
BASELINE = os.path.join("ci", "baseline", "component_boundary.txt")


def load_meta(path):
    data = {}
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            key, _, value = line.partition("=")
            data[key.strip()] = value.strip()
    return data


def split_list(value):
    return [item.strip() for item in value.split(",") if item.strip()]


def baseline_owners(root, component):
    path = os.path.join(root, BASELINE)
    if not os.path.exists(path):
        return set()
    owners = set()
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if line and not line.startswith("#"):
                name, owner = line.split("|", 1)
                if name == component:
                    owners.add(owner)
    return owners


def validate(path, root):
    component = os.path.basename(os.path.dirname(path))
    data = load_meta(path)
    problems = []
    for key in REQUIRED:
        if key not in data:
            problems.append(f"missing field: {key}")
    if data.get("name") != component:
        problems.append(f"name must be {component}")
    if not SEMVER.match(data.get("version", "")):
        problems.append("version must be MAJOR.MINOR.PATCH")
    if data.get("maturity") not in MATURITY:
        problems.append("maturity is invalid")
    if not data.get("owner"):
        problems.append("owner is required")
    if data.get("subdomain") not in SUBDOMAIN:
        problems.append("subdomain must be core or supporting")
    if not data.get("language"):
        problems.append("language is required")
    if data.get("abi_stability") not in ABI:
        problems.append("abi_stability is invalid")
    if not data.get("foundation"):
        problems.append("foundation is required")
    for key in ("rom_bytes_max", "ram_bytes_max", "stack_bytes_max"):
        try:
            int(data.get(key, ""))
        except ValueError:
            problems.append(f"{key} must be an integer")
    if data.get("heap") not in HEAP:
        problems.append("heap must be none or bounded")
    if data.get("lts") not in {"true", "false"}:
        problems.append("lts must be true or false")

    declared = set(split_list(data.get("allowed_owners", "")))
    recorded = baseline_owners(root, component)
    if recorded and declared != recorded:
        problems.append(
            f"allowed_owners {sorted(declared)} do not match baseline {sorted(recorded)}"
        )
    return problems


def main():
    parser = argparse.ArgumentParser(description="Validate component.meta files.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    problems = []
    modules = os.path.join(root, "modules")
    for module in sorted(os.listdir(modules)) if os.path.isdir(modules) else []:
        path = os.path.join(modules, module, "component.meta")
        if os.path.exists(path):
            for problem in validate(path, root):
                problems.append(f"modules/{module}/component.meta|{problem}")

    if problems:
        print("❌ [COMPONENT_METADATA_GUARD] violations:")
        for problem in problems:
            print(f"   - {problem}")
        return 1

    print("✅ [COMPONENT_METADATA_GUARD] clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
