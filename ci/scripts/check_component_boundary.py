#!/usr/bin/env python3
"""Component boundary guard.

A component may include its own headers, system headers, and the owners in the
shrink-only baseline. New owners fail. A component-specific forbidden list
locks the headers removed by a migration so they cannot come back unnoticed.
"""

import argparse
import os
import re
import sys

COMPONENTS = {
    "edge_stream": {"datahub.h", "datahub_router.h"},
    "edge_algo": {"datahub.h", "datahub_router.h"},
    "edge_mgmt": {"datahub.h", "datahub_router.h"},
    "edge_mcp": {"datahub.h", "datahub_router.h"},
}

SYSTEM_HEADERS = {"autoconf.h", "wasm_export.h"}

FILE_FORBIDDEN = {
    "modules/edge_io/src/protocol_bridge.c": {"datahub.h"},
    "modules/edge_io/include/protocol_bridge.h": {"datahub.h"},
    "modules/edge_io/src/driver_radar_fmcw.c": {"datahub.h"},
    "modules/edge_io/include/driver_radar_fmcw.h": {"datahub.h"},
    "modules/edge_io/src/adc_driver.c": {"datahub.h"},
    "modules/edge_io/include/adc_driver.h": {"datahub.h"},
    "modules/edge_mcp/src/edge_mcp_dispatcher.c": {"datahub.h"},
    "modules/edge_mcp/include/edgeos/edge_mcp_dispatcher.h": {"datahub.h"},
}
BASELINE = os.path.join("ci", "baseline", "component_boundary.txt")
INCLUDE_RE = re.compile(r'^[ \t]*#\s*include\s+"([^"]+)"', re.MULTILINE)


def header_owners(root):
    owners = {}
    modules = os.path.join(root, "modules")
    for module in os.listdir(modules) if os.path.isdir(modules) else []:
        include = os.path.join(modules, module, "include")
        for dirpath, _, filenames in os.walk(include):
            for name in filenames:
                if name.endswith(".h"):
                    owners.setdefault(name, module)
    shared = os.path.join(root, "include")
    for dirpath, _, filenames in os.walk(shared):
        for name in filenames:
            if name.endswith(".h"):
                owners.setdefault(name, "ROOT")
    foundation = os.path.join(root, "foundation", "include")
    for dirpath, _, filenames in os.walk(foundation):
        for name in filenames:
            if name.endswith(".h"):
                owners.setdefault(name, "FOUNDATION")
    return owners


def load_baseline(root):
    path = os.path.join(root, BASELINE)
    if not os.path.exists(path):
        return set()
    allowed = set()
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if line and not line.startswith("#"):
                component, owner = line.split("|", 1)
                allowed.add((component, owner))
    return allowed


def scan(root):
    owners = header_owners(root)
    allowed = load_baseline(root)
    violations = []
    pinned = set()

    for component, forbidden in COMPONENTS.items():
        base = os.path.join(root, "modules", component)
        if not os.path.isdir(base):
            continue
        for dirpath, dirnames, filenames in os.walk(base):
            dirnames[:] = [d for d in dirnames if d not in {".git", "build"}]
            for name in filenames:
                if not name.endswith((".c", ".h")):
                    continue
                path = os.path.join(dirpath, name)
                rel = os.path.relpath(path, root)
                with open(path, encoding="utf-8", errors="ignore") as handle:
                    text = handle.read()
                for include in INCLUDE_RE.findall(text):
                    if os.path.exists(os.path.join(dirpath, include)):
                        continue
                    header = os.path.basename(include)
                    if header in SYSTEM_HEADERS or "/" in include:
                        continue
                    if include.startswith("modules/"):
                        owner = include.split("/")[1]
                    else:
                        owner = owners.get(header)
                    if owner is None:
                        violations.append(f"{rel}|{include}|UNRESOLVED")
                    elif owner == component or owner == "FOUNDATION":
                        continue
                    elif header in forbidden:
                        violations.append(f"{rel}|{include}|FORBIDDEN")
                    elif (component, owner) in allowed:
                        pinned.add((component, owner))
                    else:
                        violations.append(f"{rel}|{include}|OWNER:{owner}")

    for rel, forbidden in FILE_FORBIDDEN.items():
        path = os.path.join(root, rel)
        if not os.path.exists(path):
            continue
        with open(path, encoding="utf-8", errors="ignore") as handle:
            text = handle.read()
        for include in INCLUDE_RE.findall(text):
            if os.path.basename(include) in forbidden:
                violations.append(f"{rel}|{include}|FORBIDDEN")

    return violations, pinned


def main():
    parser = argparse.ArgumentParser(description="Check component include boundaries.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    violations, pinned = scan(root)
    if violations:
        print("❌ [COMPONENT_BOUNDARY_GUARD] violations:")
        for item in violations:
            print(f"   - {item}")
        return 1

    stale = sorted(load_baseline(root) - pinned)
    if stale:
        print(
            f"⚠️  [COMPONENT_BOUNDARY_GUARD] {len(stale)} baseline entr"
            f"{'y' if len(stale) == 1 else 'ies'} no longer reproduce — delete them:"
        )
        for component, owner in stale:
            print(f"   - {component}|{owner}")

    owners = ", ".join(sorted({owner for _, owner in pinned}))
    print(f"✅ [COMPONENT_BOUNDARY_GUARD] clean. Baseline owners: {owners or 'none'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
