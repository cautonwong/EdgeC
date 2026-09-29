#!/usr/bin/env python3
"""
check_exemptions.py - Zero-Exemptions Policy & Absolute Architecture Guard (AEP-792 / Issue #346)

Enforces:
1. Absolute Zero-Exemptions: No architectural exemptions or bypass whitelists are permitted;
2. Any exemption entry registered in ci/exemptions.json is an immediate CI red-line failure;
3. Validates ledger syntax and verifies that exemptions count is strictly 0.
"""

import sys
import os
import json
import argparse

def check_exemptions_file(json_path):
    if not os.path.exists(json_path):
        return [f"Exemptions file not found: {json_path}"]

    with open(json_path, 'r', encoding='utf-8') as f:
        try:
            data = json.load(f)
        except Exception as e:
            return [f"Invalid JSON in {json_path}: {e}"]

    violations = []
    exemptions = data.get("exemptions", [])
    if len(exemptions) > 0:
        for ex in exemptions:
            ex_id = ex.get("id", "UNKNOWN")
            mod = ex.get("module", "UNKNOWN")
            rule = ex.get("rule", "UNKNOWN")
            violations.append(
                f"Zero-Exemptions Policy Violation: Exemption '{ex_id}' (rule '{rule}' on '{mod}') is registered. "
                "All architectural exemptions have been permanently abolished."
            )
    return violations

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Zero-Exemptions Policy Guard")
    parser.add_argument("--file", default="ci/exemptions.json", help="Path to exemptions.json")
    args = parser.parse_args()

    violations = check_exemptions_file(args.file)
    if violations:
        print(f"❌ [EXEMPTIONS_GUARD] Found {len(violations)} Zero-Exemptions Policy violation(s):")
        for v in violations:
            print(f"  - {v}")
        return 1
    else:
        print("✅ [EXEMPTIONS_GUARD] Zero-Exemptions Policy verified: 0 exemptions registered across repository.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
