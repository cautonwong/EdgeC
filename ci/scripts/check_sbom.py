#!/usr/bin/env python3
"""check_sbom.py - third-party license gate and SBOM/NOTICE generator.

Every external dependency in ci/dependencies.json and every component in
ci/third-party.json must declare a license. Every directory under third-party/
must be declared in ci/third-party.json. With --out the script also writes
sbom.json and NOTICE for the release artifact.
"""

import argparse
import json
import os
import sys


def load_json(path, problems):
    if not os.path.exists(path):
        problems.append(f"missing file: {path}")
        return {}
    try:
        with open(path, "r", encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, ValueError) as exc:
        problems.append(f"invalid JSON in {path}: {exc}")
        return {}


def check_components(entries, source, problems):
    for entry in entries:
        name = entry.get("name", "")
        if not name:
            problems.append(f"{source}: component without a name")
        if not entry.get("version"):
            problems.append(f"{source}: component '{name}' has no version")
        if not entry.get("license"):
            problems.append(f"{source}: component '{name}' has no license")
        if not entry.get("repository"):
            problems.append(f"{source}: component '{name}' has no repository")


def main():
    parser = argparse.ArgumentParser(description="EdgeOS third-party license gate and SBOM generator")
    parser.add_argument("--root", default=".", help="Repository root")
    parser.add_argument("--dependencies", default="ci/dependencies.json",
                        help="Pinned external dependency manifest")
    parser.add_argument("--third-party", default="ci/third-party.json",
                        help="Vendored/submodule component manifest")
    parser.add_argument("--out", help="Directory for sbom.json and NOTICE; check only when omitted")
    args = parser.parse_args()

    problems = []
    deps = load_json(os.path.join(args.root, args.dependencies), problems).get("dependencies", [])
    third_party = load_json(os.path.join(args.root, args.third_party), problems).get("components", [])
    if not deps:
        problems.append(f"{args.dependencies}: no dependencies declared")
    check_components(deps, args.dependencies, problems)
    check_components(third_party, args.third_party, problems)

    declared = {os.path.normpath(entry.get("path", "")) for entry in third_party}
    tree = os.path.join(args.root, "third-party")
    if os.path.isdir(tree):
        for name in sorted(os.listdir(tree)):
            path = os.path.join(tree, name)
            rel = os.path.normpath(os.path.join("third-party", name))
            if name.startswith(".") or not os.path.isdir(path):
                continue
            if rel not in declared:
                problems.append(f"{rel} has no license entry in {args.third_party}")

    if problems:
        print(f"❌ [SBOM_CHECK] {len(problems)} license/SBOM violation(s):")
        for problem in problems:
            print(f"  - {problem}")
        return 1

    components = []
    for dep in deps:
        components.append({
            "name": dep["name"],
            "version": dep["version"],
            "license": dep["license"],
            "repository": dep["repository"],
            "source": "external",
        })
    for entry in third_party:
        component = {
            "name": entry["name"],
            "version": entry["version"],
            "license": entry["license"],
            "repository": entry["repository"],
            "path": os.path.normpath(entry["path"]),
            "source": "vendored",
        }
        if entry.get("note"):
            component["note"] = entry["note"]
        components.append(component)

    if args.out:
        os.makedirs(args.out, exist_ok=True)
        with open(os.path.join(args.out, "sbom.json"), "w", encoding="utf-8") as handle:
            json.dump({"format": "edgec-sbom-v1", "components": components}, handle, indent=2)
            handle.write("\n")
        with open(os.path.join(args.out, "NOTICE"), "w", encoding="utf-8") as handle:
            handle.write("EdgeC Third-Party NOTICE\n\n")
            for component in components:
                handle.write(f"{component['name']} {component['version']}\n")
                handle.write(f"License: {component['license']}\n")
                handle.write(f"Source: {component['repository']}\n")
                if component.get("note"):
                    handle.write(f"Note: {component['note']}\n")
                handle.write("\n")

    print(f"✅ [SBOM_CHECK] {len(components)} third-party component(s) have license entries.")
    if args.out:
        print(f"   SBOM: {os.path.join(args.out, 'sbom.json')}")
        print(f"   NOTICE: {os.path.join(args.out, 'NOTICE')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
