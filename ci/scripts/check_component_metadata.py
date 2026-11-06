#!/usr/bin/env python3
"""Validate component.yaml against the machine-readable architecture record."""

import argparse
import os
import re
import sys

import yaml

REQUIRED = ["name", "version", "maturity", "owner", "subdomain", "api", "requires", "budgets", "support"]
MATURITY = {"incubating", "stable", "maintenance", "deprecated"}
SUBDOMAIN = {"core", "supporting"}
ABI = {"source-only", "abi-stable"}
HEAP = {"none", "bounded"}
SEMVER = re.compile(r"^\d+\.\d+\.\d+$")
BASELINE = os.path.join("ci", "baseline", "component_boundary.txt")


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
    problems = []
    with open(path, encoding="utf-8") as handle:
        data = yaml.safe_load(handle) or {}

    for key in REQUIRED:
        if key not in data:
            problems.append(f"missing field: {key}")
    if data.get("name") != component:
        problems.append(f"name must be {component}")
    if not SEMVER.match(str(data.get("version", ""))):
        problems.append("version must be MAJOR.MINOR.PATCH")
    if data.get("maturity") not in MATURITY:
        problems.append("maturity is invalid")
    if not str(data.get("owner", "")).strip():
        problems.append("owner is required")
    if data.get("subdomain") not in SUBDOMAIN:
        problems.append("subdomain must be core or supporting")

    api = data.get("api") or {}
    if not api.get("language"):
        problems.append("api.language is required")
    if api.get("abi_stability") not in ABI:
        problems.append("api.abi_stability is invalid")

    requires = data.get("requires") or {}
    ports = requires.get("ports") or {}
    if not isinstance(ports.get("required", []), list):
        problems.append("requires.ports.required must be a list")
    if not requires.get("foundation"):
        problems.append("requires.foundation is required")

    budgets = data.get("budgets") or {}
    for key in ("rom_bytes_max", "ram_bytes_max", "stack_bytes_max"):
        if not isinstance(budgets.get(key), int):
            problems.append(f"budgets.{key} must be an integer")
    if budgets.get("heap") not in HEAP:
        problems.append("budgets.heap must be none or bounded")

    if not isinstance((data.get("support") or {}).get("lts"), bool):
        problems.append("support.lts must be a boolean")

    declared = set(requires.get("allowed_owners", []))
    recorded = baseline_owners(root, component)
    if recorded and declared != recorded:
        problems.append(
            f"allowed_owners {sorted(declared)} do not match baseline {sorted(recorded)}"
        )

    return problems


def main():
    parser = argparse.ArgumentParser(description="Validate component.yaml files.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    problems = []
    modules = os.path.join(root, "modules")
    for module in sorted(os.listdir(modules)) if os.path.isdir(modules) else []:
        path = os.path.join(modules, module, "component.yaml")
        if os.path.exists(path):
            for problem in validate(path, root):
                problems.append(f"modules/{module}/component.yaml|{problem}")

    if problems:
        print("❌ [COMPONENT_METADATA_GUARD] violations:")
        for problem in problems:
            print(f"   - {problem}")
        return 1

    print("✅ [COMPONENT_METADATA_GUARD] clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
