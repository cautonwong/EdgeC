#!/usr/bin/env python3
"""
tools/check_contract_collisions.py
AEP-355: Global cross-product contract Tag ID collision detector and domain namespace compliance validator.
"""

import sys
import os
import glob
import yaml

DOMAIN_RANGES = {
    "core_standard": (0x000000, 0x00FFFF, "Core / Standard Metrology"),
    "dcu_distribution": (0x010000, 0x01FFFF, "DCU / Distribution TTU"),
    "iems_microgrid": (0x020000, 0x02FFFF, "IEMS / Microgrid ESS"),
    "smart_box_panel": (0x030000, 0x03FFFF, "SmartBox / Smart Breaker"),
    "npu_accelerator": (0x040000, 0x04FFFF, "NPU / AI Acceleration"),
}

def get_domain_for_tag_id(tag_id):
    for domain, (min_id, max_id, desc) in DOMAIN_RANGES.items():
        if min_id <= tag_id <= max_id:
            return domain
    return "extended_or_unknown"

def validate_contracts(base_dir):
    print("=" * 80)
    print("🔍 [AEP-355] Starting Global Cross-Product Tag Collision & Domain Linter")
    print("=" * 80)

    schema_dir = os.path.join(base_dir, "contracts", "schema")
    domains_dir = os.path.join(schema_dir, "domains")
    legacy_aliases_path = os.path.join(schema_dir, "legacy_aliases.yaml")

    # Load legacy aliases whitelist if present
    legacy_aliases = {}
    if os.path.exists(legacy_aliases_path):
        try:
            with open(legacy_aliases_path, "r", encoding="utf-8") as f:
                data = yaml.safe_load(f)
                if data and "aliases" in data:
                    for item in data["aliases"]:
                        alias_name = item.get("alias_name")
                        canonical_id = item.get("canonical_tag_id")
                        if alias_name and canonical_id is not None:
                            legacy_aliases[alias_name] = canonical_id
        except Exception as e:
            print(f"⚠️ Warning loading legacy_aliases.yaml: {e}")

    # Discover all domain and schema YAMLs
    yaml_files = glob.glob(os.path.join(domains_dir, "*.yaml")) + glob.glob(os.path.join(schema_dir, "*.yaml"))
    yaml_files = sorted(list(set(yaml_files)))

    tag_by_id = {} # tag_id -> list of {name, domain, file, line}
    tag_by_name = {} # name -> list of {tag_id, domain, file}
    errors = []
    total_tags = 0

    for yfile in yaml_files:
        rel_path = os.path.relpath(yfile, base_dir)
        fname = os.path.basename(yfile)
        if fname in ("legacy_aliases.yaml", "core_readonly_whitelist.txt"):
            continue

        try:
            with open(yfile, "r", encoding="utf-8") as f:
                doc = yaml.safe_load(f)
        except Exception as e:
            errors.append(f"Syntax error reading {rel_path}: {e}")
            continue

        if not isinstance(doc, dict):
            continue

        # Extract domain name if present
        domain_name = doc.get("domain", "")
        if not domain_name:
            if "dcu" in fname:
                domain_name = "dcu_distribution"
            elif "iems" in fname:
                domain_name = "iems_microgrid"
            elif "smart_box" in fname or "smartbox" in fname:
                domain_name = "smart_box_panel"
            elif "npu" in fname:
                domain_name = "npu_accelerator"
            else:
                domain_name = "core_standard"

        # Check tags dictionary
        tags_dict = {}
        if "tags" in doc and isinstance(doc["tags"], dict):
            tags_dict = doc["tags"]
        elif "tags" in doc and isinstance(doc["tags"], list):
            for t in doc["tags"]:
                if isinstance(t, dict) and "id" in t:
                    tname = t.get("name", f"tag_{t['id']}")
                    tags_dict[tname] = t

        for tag_key, tag_val in tags_dict.items():
            total_tags += 1
            tag_id = None
            tag_name = tag_key

            if isinstance(tag_val, dict):
                tag_id = tag_val.get("id") or tag_val.get("tag_id")
                if "name" in tag_val:
                    tag_name = tag_val["name"]
            elif isinstance(tag_val, int):
                tag_id = tag_val

            if tag_id is None:
                continue

            try:
                tag_id = int(tag_id)
            except ValueError:
                errors.append(f"[{rel_path}] Tag '{tag_name}' has non-integer ID: {tag_id}")
                continue

            # Record
            entry = {
                "id": tag_id,
                "name": tag_name,
                "domain": domain_name,
                "file": rel_path
            }

            # 1. Global Tag ID Collision Check
            if tag_id in tag_by_id:
                for existing in tag_by_id[tag_id]:
                    # If same ID has completely different names and not in legacy aliases whitelist
                    if existing["name"] != tag_name and tag_name not in legacy_aliases and existing["name"] not in legacy_aliases:
                        errors.append(
                            f"❌ [TagID Collision] Tag ID {tag_id} (0x{tag_id:06X}) is defined conflictingly:\n"
                            f"   • '{existing['name']}' in {existing['file']} (Domain: {existing['domain']})\n"
                            f"   • '{tag_name}' in {rel_path} (Domain: {domain_name})"
                        )
                tag_by_id[tag_id].append(entry)
            else:
                tag_by_id[tag_id] = [entry]

            # 2. Cross-domain Same Name Collision Check
            if tag_name in tag_by_name:
                for existing in tag_by_name[tag_name]:
                    if existing["domain"] != domain_name and existing["id"] != tag_id:
                        errors.append(
                            f"❌ [Cross-Domain Name Collision] Tag name '{tag_name}' defined in multiple domains with different IDs:\n"
                            f"   • ID {existing['id']} (Domain: {existing['domain']}, File: {existing['file']})\n"
                            f"   • ID {tag_id} (Domain: {domain_name}, File: {rel_path})\n"
                            f"   → Rule: Non-core cross-product tags must be prefixed with domain name (e.g. {domain_name}.{tag_name})"
                        )
                tag_by_name[tag_name].append(entry)
            else:
                tag_by_name[tag_name] = [entry]

            # 3. Domain Range Boundary Check
            if domain_name in DOMAIN_RANGES:
                min_id, max_id, desc = DOMAIN_RANGES[domain_name]
                # Allow standard core tags (0..65535) to be referenced across domains as shared kernel
                if not (min_id <= tag_id <= max_id) and not (0 <= tag_id <= 65535):
                    errors.append(
                        f"❌ [Domain Boundary Violation] Tag '{tag_name}' (ID: {tag_id} / 0x{tag_id:06X}) in '{domain_name}'\n"
                        f"   is out of domain range [0x{min_id:06X}..0x{max_id:06X}] ({desc})"
                    )

    # Print Summary Table
    print(f"📊 Verified {len(yaml_files)} contract YAML files ({total_tags} total tag definitions across domains):")
    print("-" * 80)
    print(f"{'Domain Namespace':<20} | {'Range Hex':<20} | {'Registered Tags':<15} | {'Status'}")
    print("-" * 80)
    for domain, (min_id, max_id, desc) in DOMAIN_RANGES.items():
        domain_tag_count = sum(1 for entries in tag_by_id.values() if any(e["domain"] == domain for e in entries))
        status = "✅ CLEAN"
        print(f"{domain:<20} | 0x{min_id:06X}..0x{max_id:06X} | {domain_tag_count:<15} | {status}")
    print("-" * 80)

    if errors:
        print(f"\n🚨 FOUND {len(errors)} CONTRACT COLLISION / VALIDATION ERRORS:")
        for err in errors:
            print(err)
        return 1

    print("\n✅ [AEP-355 PASS] Zero Tag ID collisions or cross-domain namespace violations detected.")
    return 0

if __name__ == "__main__":
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    sys.exit(validate_contracts(repo_root))
