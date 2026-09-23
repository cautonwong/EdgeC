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

    violations = []
    exemptions = data.get("exemptions", [])
    for ex in exemptions:
        ex_id = ex.get("id", "UNKNOWN")
        for field in REQUIRED_FIELDS:
            val = ex.get(field, "")
            if not val or not str(val).strip():
                violations.append(f"Exemption '{ex_id}' is missing required governance field '{field}'")
    return violations

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
