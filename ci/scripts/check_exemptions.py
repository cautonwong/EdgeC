#!/usr/bin/env python3
"""
check_exemptions.py - Architectural Exemptions & Governance Ledger Guard (AEP-792)

Enforces that every architectural exemption registered in ci/exemptions.json
contains required governance fields: id, module, rule, owner, reason, ticket, and exit_plan.
"""

import sys
import os
import json
import argparse

REQUIRED_FIELDS = ["id", "module", "rule", "owner", "reason", "ticket", "exit_plan"]

def check_exemptions_file(json_path):
    if not os.path.exists(json_path):
        return [f"Exemptions file not found: {json_path}"]

    with open(json_path, 'r', encoding='utf-8') as f:
        try:
            data = json.load(f)
        except Exception as e:
            return [f"Invalid JSON in {json_path}: {e}"]

    return [
        f"Exemption '{ex.get('id', 'UNKNOWN')}' is missing required governance field '{field}'"
        for ex in data.get("exemptions", [])
        for field in REQUIRED_FIELDS
        if not str(ex.get(field, "")).strip()
    ]

def main():
    parser = argparse.ArgumentParser(description="EdgeOS Exemptions Ledger Guard")
    parser.add_argument("--file", default="ci/exemptions.json", help="Path to exemptions.json")
    args = parser.parse_args()

    violations = check_exemptions_file(args.file)
    if violations:
        print(f"❌ [EXEMPTIONS_GUARD] Found {len(violations)} exemption ledger governance violation(s):")
        for v in violations:
            print(f"  - {v}")
        return 1
    else:
        print("✅ [EXEMPTIONS_GUARD] All registered architectural exemptions satisfy complete governance criteria.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
