#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
EdgeOS Contract Compiler v2.0 (Decoupled)
------------------------------------------
遵循 core_v2.md 规范：
1. 移除生成的结构体定义，引用 contract_data_types.h
2. 实现多产品物理隔离生成
"""

import yaml
import os
import sys
import argparse
import datetime

def load_yaml(path):
    if not path or not os.path.exists(path):
        return None
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return yaml.safe_load(f)
    except Exception as e:
        print(f"[ERROR] Failed to load {path}: {e}")
        sys.exit(1)

def sort_tags_by_id(tags_list):
    return sorted(tags_list, key=lambda x: x['id'])

def build_effective_contract(schema_path, product_path, local_path):
    schema_data = load_yaml(schema_path)
    all_tags_map = {t['name']: t for t in schema_data['tags']}

    # 合并本地扩展
    if local_path:
        local_data = load_yaml(local_path)
        if local_data and 'tags' in local_data:
            for t in local_data['tags']:
                all_tags_map[t['name']] = t

    prod_conf = load_yaml(product_path)

    # 注入协议映射 (兼容 map 和 mappings 字段)
    if 'includes' in prod_conf:
        for inc_path in prod_conf['includes']:
            if not os.path.exists(inc_path): continue
            binding_data = load_yaml(inc_path)
            proto = binding_data.get('protocol', 'UNKNOWN')

            # 关键修复：兼容点表的不同命名
            mapping_list = binding_data.get('mappings', binding_data.get('map', []))

            for m in mapping_list:
                tag_name = m['tag']
                if tag_name not in all_tags_map: continue
                target_tag = all_tags_map[tag_name]
                if 'bindings' not in target_tag: target_tag['bindings'] = []
                m['protocol_type'] = proto
                target_tag['bindings'].append(m)

    # 处理使能逻辑
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

        # 深度拷贝基础定义
        final_tag = info.copy()

        # 应用产品级覆盖 (L3 Overrides)
        # 支持递归合并或简单的覆盖，此处采用字典更新
        if 'attributes' not in final_tag: final_tag['attributes'] = {}
        for k, v in ovr.items():
            if k == 'enabled': continue
            # 如果是字典，进行浅合并
            if isinstance(v, dict) and k in final_tag['attributes']:
                final_tag['attributes'][k].update(v)
            else:
                final_tag['attributes'][k] = v

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

        f.write("// [Section 1] Feature Gates\n")
        for name in sorted([t['name'] for t in contract['all_tags']]):
            val = 1 if name in active_names_set else 0
            f.write(f"#define TAG_{name}_ENABLED {val}\n")

        f.write("\n// [Section 2] Tag ID Enum\n")
        f.write("typedef enum {\n    TAG_INVALID = 0,\n")
        for t in sort_tags_by_id(active_tags):
            f.write(f"    TAG_{t['name']} = {t['id']},\n")
        f.write("    TAG_MAX_ID\n} edge_tag_enum_t;\n\n")

        f.write("// [Section 3] APIs\n")
        f.write("const tag_meta_t* edge_get_meta(edge_tag_id_t id);\n")
        f.write("edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale);\n")
        f.write("edge_tag_id_t edge_lookup_dlms(const char* obis, float *out_scale);\n")
        f.write("edge_tag_id_t edge_lookup_iec104(int ioa);\n")
        f.write("\n#endif\n")

def gen_source(contract, out_path):
    print(f"--- Generating Source: {out_path}")
    active_tags = sort_tags_by_id(contract['active_tags'])
    modbus_list = []
    dlms_list = []

    for t in active_tags:
        for b in t.get('bindings', []):
            b_merged = b.copy()
            b_merged['tag_name'] = t['name']
            ptype = b.get('protocol_type', b.get('protocol', '')).upper()
            if ptype == 'MODBUS': modbus_list.append(b_merged)
            elif ptype == 'DLMS': dlms_list.append(b_merged)

    with open(out_path, 'w') as f:
        f.write("#include \"contracts.h\"\n#include <string.h>\n\n")
        f.write("const tag_meta_t g_tag_meta_table[] = {\n")
        for t in active_tags:
            persist = 'true' if t.get('attributes', {}).get('persist') else 'false'
            upload = 'true' if t.get('attributes', {}).get('upload') else 'false'
            f.write(f"    [TAG_{t['name']}] = {{ \"{t['name']}\", \"{t.get('unit','')}\", {persist}, {upload} }},\n")
        f.write("};\n\n")

        # 二分查找实现的通用逻辑...
        modbus_list.sort(key=lambda x: x['addr'])
        f.write("static const modbus_map_t s_modbus_map[] = {\n")
        for m in modbus_list:
            f.write(f"    {{ {m['addr']}, TAG_{m['tag_name']}, {m.get('scale', 1.0)}f }},\n")
        f.write("};\n")
        f.write(f"static const int s_modbus_len = {len(modbus_list)};\n\n")

        # 查找函数实现 (此处保持逻辑简洁，由于是生成代码，可直接输出实现)
        f.write("\nedge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale) {\n    int left = 0, right = s_modbus_len - 1;\n    while (left <= right) {\n        int mid = left + (right - left) / 2;\n        if (s_modbus_map[mid].addr == reg) {\n            if (out_scale) *out_scale = s_modbus_map[mid].scale;\n            return s_modbus_map[mid].id;\n        }\n        if (s_modbus_map[mid].addr < reg) left = mid + 1;\n        else right = mid - 1;\n    }\n    return TAG_INVALID;\n}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--schema', required=True)
    parser.add_argument('--product', required=True)
    parser.add_argument('--local')
    parser.add_argument('--out', required=True)
    args = parser.parse_args()

    contract = build_effective_contract(args.schema, args.product, args.local)
    if not os.path.exists(args.out): os.makedirs(args.out)

    # [恢复] 导出中间态审计 YAML
    eff_yaml_path = os.path.join(args.out, "contracts.yaml")
    with open(eff_yaml_path, 'w', encoding='utf-8') as f:
        yaml.dump({
            "product": contract['product_name'],
            "generated_at": contract['timestamp'],
            "tags": contract['active_tags']
        }, f, sort_keys=False, allow_unicode=True)
    print(f"Generated Audit YAML: {eff_yaml_path}")

    gen_header(contract, os.path.join(args.out, "contracts.h"))
    gen_source(contract, os.path.join(args.out, "contracts.c"))
