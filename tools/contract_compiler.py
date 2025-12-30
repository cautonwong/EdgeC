#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import yaml
import os
import sys
import argparse
import datetime

def load_yaml(path):
    if not path or not os.path.exists(path): return None
    try:
        with open(path, 'r', encoding='utf-8') as f: return yaml.safe_load(f)
    except Exception as e:
        print(f"[ERROR] Failed to load {path}: {e}")
        sys.exit(1)

def build_effective_contract(schema_path, product_path, local_path):
    schema_data = load_yaml(schema_path)
    all_tags_map = {t['name']: t for t in schema_data['tags']}
    if local_path:
        local_data = load_yaml(local_path)
        if local_data and 'tags' in local_data:
            for t in local_data['tags']: all_tags_map[t['name']] = t
    prod_conf = load_yaml(product_path)
    if 'includes' in prod_conf:
        for inc_path in prod_conf['includes']:
            if not os.path.exists(inc_path): continue
            binding_data = load_yaml(inc_path)
            proto = binding_data.get('protocol', 'UNKNOWN').upper()
            mapping_list = binding_data.get('mappings', binding_data.get('map', []))
            for m in mapping_list:
                if m['tag'] not in all_tags_map: continue
                target_tag = all_tags_map[m['tag']]
                if 'bindings' not in target_tag: target_tag['bindings'] = []
                m['protocol_type'] = proto
                target_tag['bindings'].append(m)
    prod_enabled = set(prod_conf.get('enabled_tags', []))
    local_data = load_yaml(local_path) if local_path else None
    local_enabled = set(local_data.get('enabled_tags', []) if local_data else [])
    enabled_names = prod_enabled.union(local_enabled)
    if local_data and 'tags' in local_data:
        for t in local_data['tags']: enabled_names.add(t['name'])
    active_tags = []
    overrides = prod_conf.get('overrides', {})
    for name in sorted(list(enabled_names)):
        if name not in all_tags_map: continue
        info = all_tags_map[name]
        ovr = overrides.get(name, {})
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
        "active_tags": active_tags,
        "all_tags": list(all_tags_map.values())
    }

def gen_header(contract, out_path):
    print(f"--- Generating Header: {out_path}")
    active_tags = contract['active_tags']
    active_names_set = set(t['name'] for t in active_tags)
    with open(out_path, 'w') as f:
        f.write("/* AUTO-GENERATED. DO NOT EDIT. */\n")
        f.write("#ifndef EDGE_CONTRACTS_H\n#define EDGE_CONTRACTS_H\n\n")
        f.write("#include \"contract_data_types.h\"\n\n")
        for name in sorted([t['name'] for t in contract['all_tags']]):
            val = 1 if name in active_names_set else 0
            f.write(f"#define TAG_{name}_ENABLED {val}\n")
        f.write("\ntypedef enum {\n    TAG_INVALID = 0,\n")
        for t in sorted(active_tags, key=lambda x: x['id']):
            f.write(f"    TAG_{t['name']} = {t['id']},\n")
        f.write("    TAG_MAX_ID\n} edge_tag_enum_t;\n\n")
        f.write("#endif\n")

def gen_source(contract, out_path):
    print(f"--- Generating Source: {out_path}")
    active_tags = sorted(contract['active_tags'], key=lambda x: x['id'])
    proto_lists = {'MODBUS':[], 'DLMS':[], 'DLT645':[], 'DLT698':[], 'IEC104':[], 'CAN':[]}
    for t in active_tags:
        for b in t.get('bindings', []):
            b_merged = b.copy(); b_merged['tag_name'] = t['name']
            ptype = b.get('protocol_type', b.get('protocol', '')).upper()
            if ptype in proto_lists: proto_lists[ptype].append(b_merged)

    with open(out_path, 'w') as f:
        f.write("#include \"contracts.h\"\n#include <string.h>\n\n")
        f.write("const tag_meta_t g_tag_meta_table[] = {\n")
        for t in active_tags:
            p = 'true' if t.get('attributes', {}).get('persist') else 'false'
            u = 'true' if t.get('attributes', {}).get('upload') else 'false'
            f.write(f"    [TAG_{t['name']}] = {{ \"{t['name']}\", \"{t.get('unit','')}\", {p}, {u} }},\n")
        f.write("};\n\n")
        f.write("const tag_meta_t* edge_get_meta(edge_tag_id_t id) {\n    if (id >= TAG_MAX_ID) return NULL;\n    return &g_tag_meta_table[id];\n}\n\n")

        # Modbus
        m_list = sorted(proto_lists['MODBUS'], key=lambda x: x.get('addr', 0))
        f.write("static const modbus_map_t s_modbus_map[] = {\n")
        for m in m_list: f.write(f"    {{ {m['addr']}, TAG_{m['tag_name']}, {m.get('scale', 1.0)}f }},\n")
        f.write("};\n")
        f.write(f"static const int s_modbus_len = {len(m_list)};\n\n")
        f.write("edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale) {\n")
        f.write("    for(int i=0; i < s_modbus_len; i++) {\n")
        f.write("        if(s_modbus_map[i].addr == reg) { if(out_scale) *out_scale = s_modbus_map[i].scale; return s_modbus_map[i].id; }\n")
        f.write("    }\n    return 0;\n}\n\n")

        # DLMS
        d_list = proto_lists['DLMS']
        f.write("static const dlms_map_t s_dlms_map[] = {\n")
        for d in d_list: f.write(f"    {{ \"{d.get('obis','')}\", TAG_{d['tag_name']}, {d.get('scaler', 1.0)}f }},\n")
        f.write("};\n")
        f.write(f"static const int s_dlms_len = {len(d_list)};\n\n")
        f.write("edge_tag_id_t edge_lookup_dlms(const char* obis, float *out_scale) {\n")
        f.write("    for(int i=0; i < s_dlms_len; i++) {\n")
        f.write("        if(strcmp(s_dlms_map[i].obis, obis) == 0) { if(out_scale) *out_scale = s_dlms_map[i].scaler; return s_dlms_map[i].id; }\n")
        f.write("    }\n    return 0;\n}\n\n")

        f.write("edge_tag_id_t edge_lookup_dlt645(const char* di, float *out_scale) { (void)di; (void)out_scale; return 0; }\n")
        f.write("edge_tag_id_t edge_lookup_dlt698(const char* oaid, float *out_scale) { (void)oaid; (void)out_scale; return 0; }\n")
        f.write("edge_tag_id_t edge_lookup_iec104(int ioa) { (void)ioa; return 0; }\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--schema', required=True); parser.add_argument('--product', required=True)
    parser.add_argument('--local'); parser.add_argument('--out', required=True)
    args = parser.parse_args()
    contract = build_effective_contract(args.schema, args.product, args.local)
    if not os.path.exists(args.out): os.makedirs(args.out)
    with open(os.path.join(args.out, "contracts.yaml"), 'w', encoding='utf-8') as f:
        yaml.dump({"product": contract['product_name'], "tags": contract['active_tags']}, f, sort_keys=False, allow_unicode=True)
    gen_header(contract, os.path.join(args.out, "contracts.h"))
    gen_source(contract, os.path.join(args.out, "contracts.c"))