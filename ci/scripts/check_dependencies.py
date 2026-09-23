#!/usr/bin/env python3
"""
check_dependencies.py - Immutable Dependency SHA Pinning Guard (AEP-792)

Enforces that all third-party external dependencies listed in ci/dependencies.json
are strictly pinned to a full 40-character commit SHA with valid repository and license metadata.
"""

import sys
import os
import json
import re
import argparse

SHA_PATTERN = re.compile(r'^[0-9a-fA-F]{40}$')

def check_dependencies_file(json_path):
    if not os.path.exists(json_path):
        return [f"Dependencies file not found: {json_path}"]

    with open(json_path, 'r', encoding='utf-8') as f:
        try:
            data = json.load(f)
        except Exception as e:
            return [f"Invalid JSON syntax in {json_path}: {e}"]

    violations = []
    deps = data.get("dependencies", [])
    if not deps:
        violations.append("No dependencies array defined in dependencies.json")

    for dep in deps:
        name = dep.get("name", "unknown")
        sha = dep.get("commit_sha", "")
        repo = dep.get("repository", "")
        license_name = dep.get("license", "")

        if not SHA_PATTERN.match(sha):
            violations.append(f"Dependency '{name}' does not have a valid 40-character commit SHA: '{sha}'")
        if not repo:
            violations.append(f"Dependency '{name}' is missing repository URL")
        if not license_name:
            violations.append(f"Dependency '{name}' is missing license metadata")

    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Dependency Pinning Guard")
    parser.add_argument("--file", default="ci/dependencies.json", help="Path to dependencies.json")
    args = parser.parse_args()

    violations = check_dependencies_file(args.file)
    if violations:
        print(f"❌ [DEPENDENCY_GUARD] Found {len(violations)} dependency pinning violation(s):")
        for v in violations:
            print(f"  - {v}")
        return 1
    else:
        print("✅ [DEPENDENCY_GUARD] All third-party dependencies are strictly locked with 40-char commit SHAs.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
