#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
EdgeOS Contract Compiler v3.0 (Descriptor Support)
"""

import yaml
import json
import os
import sys
import argparse
import datetime

def load_yaml(path):
    if not path or not os.path.exists(path): return None
    try:
        with open(path, 'r', encoding='utf-8') as f: return yaml.safe_load(f)
    except Exception as e: 
        print(f"[ERROR] Failed to load {path}: {e}"); sys.exit(1)

def build_effective_contract(schema_path, product_path, local_path, base_path):
    schema_data = load_yaml(schema_path)
    all_tags_map = {t['name']: t for t in schema_data['tags']}
    if local_path:
        local_data = load_yaml(local_path)
        if local_data and 'tags' in local_data:
            for t in local_data['tags']:
                name = t['name']
                if name in all_tags_map: all_tags_map[name].update(t)
                else: all_tags_map[name] = t
    prod_conf = load_yaml(product_path)
    local_data = load_yaml(local_path) if local_path else None
    if 'includes' in prod_conf:
        for inc_path in prod_conf['includes']:
            real_path = inc_path
            if not os.path.isabs(real_path) and base_path: real_path = os.path.join(base_path, inc_path)
            if not os.path.exists(real_path): continue
            bind_data = load_yaml(real_path)
            proto = bind_data.get('protocol', 'UNKNOWN').upper()
            mapping_list = bind_data.get('mappings', bind_data.get('map', []))
            for m in mapping_list:
                tn = m.get('tag')
                if not tn or tn not in all_tags_map: continue
                target_tag = all_tags_map[tn]
                if 'bindings' not in target_tag: target_tag['bindings'] = []
                m['protocol_type'] = proto
                target_tag['bindings'].append(m)
    prod_enabled = set(prod_conf.get('enabled_tags', []))
    local_enabled = set(local_data.get('enabled_tags', []) if (local_path and local_data) else [])
    enabled_names = prod_enabled.union(local_enabled)
    active_tags = []
    overrides = prod_conf.get('overrides', {})
    for name in sorted(list(enabled_names)):
        if name not in all_tags_map: continue
        info = all_tags_map[name]; ovr = overrides.get(name, {})
        if ovr.get('enabled') is False: continue
        final_tag = info.copy()
        if 'attributes' not in final_tag: final_tag['attributes'] = {}
        for k, v in ovr.items():
            if k == 'enabled': continue
            if isinstance(v, dict) and k in final_tag['attributes']: final_tag['attributes'][k].update(v)
            else: final_tag['attributes'][k] = v
        active_tags.append(final_tag)
    return {
        "product_name": prod_conf.get('product', 'Unknown'),
        "timestamp": str(datetime.datetime.now()),
        "ui_config": prod_conf.get('ui_config', {}), # [新增] 透传 UI 配置
        "active_tags": active_tags,
        "all_tags": list(all_tags_map.values())
    }

def gen_header(contract, out_path):
    active_tags = sorted(contract['active_tags'], key=lambda x: x['id'])
    active_names_set = set(t['name'] for t in active_tags)
    lines = ["/* AUTO-GENERATED. DO NOT EDIT. */", "#ifndef EDGE_CONTRACTS_H", "#define EDGE_CONTRACTS_H", "", "#include \"contract_data_types.h\"", ""]
    for name in sorted([t['name'] for t in contract['all_tags']]):
        val = 1 if name in active_names_set else 0
        lines.append("#define TAG_%s_ENABLED %d" % (name, val))
    lines.append("\ntypedef enum {\n    TAG_INVALID = 0,")
    for t in active_tags: lines.append("    TAG_%s = %d," % (t['name'], t['id']))
    lines.append("    TAG_MAX_ID\n} edge_tag_enum_t;\n")
    lines.append("const tag_meta_t* edge_get_meta(edge_tag_id_t id);")
    lines.append("edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale);")
    lines.append("edge_tag_id_t edge_lookup_dlms(const char* obis, float *out_scale);")
    lines.append("edge_tag_id_t edge_lookup_iec104(int ioa);")
    lines.append("\n#endif")
    with open(out_path, 'w') as f: f.write("\n".join(lines) + "\n")

def gen_source(contract, out_path):
    active_tags = sorted(contract['active_tags'], key=lambda x: x['id'])
    proto_map = {'MODBUS':[], 'DLMS':[], 'IEC104':[], 'CAN':[]}
    for t in active_tags:
        for b in t.get('bindings', []):
            b_merged = b.copy(); b_merged['tag_name'] = t['name']; b_merged['tag_id'] = t['id']
            ptype = b.get('protocol_type', b.get('protocol', 'UNKNOWN')).upper()
            if ptype in proto_map: proto_map[ptype].append(b_merged)
    lines = ["#include \"contracts.h\"", "#include <string.h>", "", "const tag_meta_t g_tag_meta_table[] = {"]
    for t in active_tags:
        p = 'true' if t.get('attributes', {}).get('persist') else 'false'
        u = 'true' if t.get('attributes', {}).get('upload') else 'false'
        lines.append("    [TAG_%s] = { \"%s\", \"%s\", %s, %s }," % (t['name'], t['name'], t.get('unit',''), p, u))
    lines.append("};\nconst tag_meta_t* edge_get_meta(edge_tag_id_t id) { if (id >= TAG_MAX_ID) return NULL; return &g_tag_meta_table[id]; }\n")
    m_list = sorted(proto_map['MODBUS'], key=lambda x: x.get('addr', 0))
    lines.append("static const modbus_map_t s_modbus_map[] = {")
    for m in m_list: lines.append("    { %d, TAG_%s, %ff }," % (m['addr'], m['tag_name'], m.get('scale', 1.0)))
    lines.append("};\nstatic const int s_modbus_len = %d;" % len(m_list))
    lines.append("edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale) { for(int i=0; i < s_modbus_len; i++) if(s_modbus_map[i].addr == reg) { if(out_scale) *out_scale=s_modbus_map[i].scale; return s_modbus_map[i].id; } return 0; }\n")
    for p in ['DLMS']:
        d_list = proto_map[p]
        lines.append("static const dlms_map_t s_%s_map[] = {" % p.lower())
        for d in d_list:
            lines.append("    { \"%s\", TAG_%s, 0, 0, 0, %ff }," % (d.get('obis',''), d['tag_name'], d.get('scaler', 1.0)))
        lines.append("};\nstatic const int s_%s_len = %d;" % (p.lower(), len(d_list)))
        lines.append("edge_tag_id_t edge_lookup_%s(const char* key, float *out_scale) { for(int i=0; i < s_%s_len; i++) if(strcmp(s_%s_map[i].obis, key) == 0) { if(out_scale) *out_scale=s_%s_map[i].scaler; return s_%s_map[i].id; } return 0; }\n" % (p.lower(), p.lower(), p.lower(), p.lower(), p.lower()))
    lines.append("edge_tag_id_t edge_lookup_iec104(int ioa) { (void)ioa; return 0; }")
    with open(out_path, 'w') as f: f.write("\n".join(lines) + "\n")

def gen_ui_descriptor(contract, out_path):
    active_tags = contract['active_tags']
    categories = set()
    for t in active_tags:
        cat = t.get('attributes', {}).get('category')
        if cat: categories.add(cat)
    menu = [{"id": "dashboard", "label": "Dashboard", "icon": "LayoutDashboard", "visible": True}]
    if "Metrology" in categories: menu.append({"id": "data", "label": "Telemetry", "icon": "Activity", "visible": True})
    if "Storage" in categories: menu.append({"id": "history", "label": "History", "icon": "History", "visible": True})
    if "EVSE" in categories: menu.append({"id": "evse", "label": "EV Charger", "icon": "Zap", "visible": True})
    menu.append({"id": "system", "label": "Settings", "icon": "Settings", "visible": True})
    descriptor = {"product": contract['product_name'], "timestamp": contract['timestamp'], "features": list(categories), "menu": menu}
    with open(out_path, 'w', encoding='utf-8') as f: json.dump(descriptor, f, indent=2)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--schema', required=True); parser.add_argument('--product', required=True)
    parser.add_argument('--local'); parser.add_argument('--out', required=True); parser.add_argument('--base')
    args = parser.parse_args()
    contract = build_effective_contract(args.schema, args.product, args.local, args.base)
    if not os.path.exists(args.out): os.makedirs(args.out)
    with open(os.path.join(args.out, "contracts.yaml"), 'w', encoding='utf-8') as f:
        yaml.dump({
            "product": contract['product_name'], 
            "ui_config": contract['ui_config'], # [新增]
            "tags": contract['active_tags']
        }, f, sort_keys=False, allow_unicode=True)
    gen_header(contract, os.path.join(args.out, "contracts.h"))
    gen_source(contract, os.path.join(args.out, "contracts.c"))
    gen_ui_descriptor(contract, os.path.join(args.out, "ui_descriptor.json"))