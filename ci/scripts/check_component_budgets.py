#!/usr/bin/env python3
"""Check declared component ROM/RAM budgets against built archives."""

import argparse
import glob
import os
import re
import subprocess
import sys

META = re.compile(r"^([A-Za-z0-9_]+)=(.*)$")
TARGET = re.compile(r"add_library\(\s*([A-Za-z0-9_]+)")


def load_meta(path):
    data = {}
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            match = META.match(line.strip())
            if match:
                data[match.group(1)] = match.group(2).strip()
    return data


def component_target(component_dir):
    cmake = os.path.join(component_dir, "CMakeLists.txt")
    if not os.path.exists(cmake):
        return None
    with open(cmake, encoding="utf-8") as handle:
        match = TARGET.search(handle.read())
    return match.group(1) if match else None


def archive_sizes(archive):
    output = subprocess.check_output(["size", "-t", archive], text=True)
    for line in reversed(output.splitlines()):
        parts = line.split()
        if len(parts) >= 4 and parts[-1] == "(TOTALS)":
            text, data, bss = (int(parts[0]), int(parts[1]), int(parts[2]))
            return text + data, data + bss
    raise RuntimeError(f"size output has no TOTALS for {archive}")


def components(root):
    for kind in ("modules", "adapters"):
        for meta_path in sorted(glob.glob(os.path.join(root, kind, "*", "component.meta"))):
            yield kind, meta_path


def main():
    parser = argparse.ArgumentParser(description="Check component ROM/RAM budgets.")
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--build", default="build", help="build directory")
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    build = os.path.abspath(args.build)
    problems = []
    for kind, meta_path in components(root):
        component_dir = os.path.dirname(meta_path)
        name = os.path.basename(component_dir)
        meta = load_meta(meta_path)
        rom_max = int(meta.get("rom_bytes_max", "0"))
        ram_max = int(meta.get("ram_bytes_max", "0"))
        if rom_max <= 0 and ram_max <= 0:
            problems.append(f"{kind}/{name}: no ROM/RAM budget declared")
            continue
        target = component_target(component_dir)
        if not target:
            problems.append(f"{kind}/{name}: no add_library target")
            continue
        archive = os.path.join(build, kind, name, f"lib{target}.a")
        if not os.path.exists(archive):
            problems.append(f"{kind}/{name}: archive missing at {archive}")
            continue
        rom, ram = archive_sizes(archive)
        if rom_max > 0 and rom > rom_max:
            problems.append(f"{kind}/{name}: ROM {rom} > {rom_max}")
        if ram_max > 0 and ram > ram_max:
            problems.append(f"{kind}/{name}: RAM {ram} > {ram_max}")
    if problems:
        print("❌ [COMPONENT_BUDGET] violations:")
        for problem in problems:
            print(f"   - {problem}")
        return 1
    print("✅ [COMPONENT_BUDGET] clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
