#!/usr/bin/env python3
"""Keep the stream stage registry honest.

Every g_standard_stages[] entry must name a real, unique factory, and every
stage_*.c V2 factory must be registered or explicitly exempted.
"""

import argparse
import glob
import os
import re
import sys

REGISTRY = "modules/edge_stream/src/standard_stages.c"
STAGE_DIR = "modules/edge_stream/src"
STAGE_GLOB = "stage_*.c"

# Compiled into edge_stream but deliberately absent from the registry.
EXEMPT = {
    "stage_foc_core_create_v2": "alternate FOC core; registry 'foc' uses stage_foc_create_v2",
    "stage_vibration_features_create_v2": "direct-only API; tests call stage_vibration_features_create",
    "stage_wavelet_anomaly_create_v2": "direct-only API; tests call stage_wavelet_anomaly_create",
}

ENTRY = re.compile(r'^\{\s*"([^"]*)"\s*,\s*([A-Za-z_]\w*|NULL)\s*\}\s*,?\s*$')
# One entry per line; extend if the table ever wraps an entry.
FACTORY = re.compile(
    r"\bpipeline_stage_t\s*\*\s*(stage_\w+_create(?:_v2)?)\s*\([^;{]*\)\s*([{;])",
    re.S,
)


def read(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        return handle.read()


def line_of(text, index):
    return text.count("\n", 0, index) + 1


def factories(text):
    """Map symbol -> (kind, line); kind is 'defined' or 'declared'."""
    found = {}
    for match in FACTORY.finditer(text):
        kind = "defined" if match.group(2) == "{" else "declared"
        found.setdefault(match.group(1), (kind, line_of(text, match.start())))
    return found


def registry(text):
    """Return (table line, [(name, factory, line)]); name is None if unparseable."""
    lines = text.splitlines()
    start = next((i for i, line in enumerate(lines) if "g_standard_stages[]" in line), None)
    if start is None:
        return None, []
    entries = []
    for i in range(start + 1, len(lines)):
        stripped = lines[i].split("//", 1)[0].strip()
        if not stripped or stripped.startswith("#"):
            continue
        if stripped.startswith("};") or stripped in ("{NULL, NULL},", "{NULL, NULL}"):
            break
        match = ENTRY.match(stripped)
        if match:
            entries.append((match.group(1), match.group(2), i + 1))
        else:
            entries.append((None, None, i + 1))
    return start + 1, entries


def main():
    parser = argparse.ArgumentParser(description="Validate the stream stage registry.")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()
    root = os.path.abspath(args.root)

    registry_path = os.path.join(root, REGISTRY)
    if not os.path.exists(registry_path):
        print(f"{REGISTRY}:0: registry file not found")
        return 1

    text = read(registry_path)
    table_line, entries = registry(text)
    problems = []
    if table_line is None:
        problems.append((REGISTRY, 0, "g_standard_stages[] table not found"))

    names = {}
    for name, factory, line in entries:
        if name is None:
            problems.append((REGISTRY, line, "unparseable registry entry"))
            continue
        if not name:
            problems.append((REGISTRY, line, "registry name is empty"))
        elif name in names:
            problems.append(
                (REGISTRY, line, f"duplicate registry name '{name}' (first at line {names[name]})")
            )
        else:
            names[name] = line
        if factory == "NULL":
            problems.append((REGISTRY, line, f"registry entry '{name}' has a NULL factory"))

    known = factories(text)
    stage_defs = {}
    for path in sorted(glob.glob(os.path.join(root, STAGE_DIR, STAGE_GLOB))):
        rel = os.path.relpath(path, root).replace(os.sep, "/")
        stage_defs[rel] = {
            symbol: line
            for symbol, (kind, line) in factories(read(path)).items()
            if kind == "defined"
        }
        for symbol, line in stage_defs[rel].items():
            known.setdefault(symbol, ("defined", line))

    for name, factory, line in entries:
        if factory in (None, "NULL"):
            continue
        kind, decl_line = known.get(factory, (None, None))
        if kind == "defined":
            continue
        if kind == "declared" and decl_line < table_line:
            continue
        problems.append(
            (REGISTRY, line, f"factory '{factory}' is not defined or declared before use")
        )

    registered = {factory for _, factory, _ in entries if factory}
    for rel, defs in stage_defs.items():
        for symbol, line in sorted(defs.items(), key=lambda item: item[1]):
            if not symbol.endswith("_create_v2"):
                continue
            if symbol in registered or symbol in EXEMPT:
                continue
            problems.append(
                (
                    rel,
                    line,
                    f"'{symbol}' is not registered in g_standard_stages[]; "
                    "register it or add an EXEMPT reason",
                )
            )

    if problems:
        for path, line, reason in problems:
            print(f"{path}:{line}: {reason}")
        return 1

    print(f"stage registry clean: {len(entries)} entries, {len(EXEMPT)} exemptions.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
