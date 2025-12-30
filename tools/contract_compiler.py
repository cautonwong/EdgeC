#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
EdgeOS Contract Compiler v3.1 (Bugfix)
------------------------------------------
修复：AttributeError: 'list' object has no attribute 'values'
"""

import yaml
import os
import sys
import argparse
import datetime

# ==============================================================================
# 1. 基础工具函数
# ==============================================================================

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
    # [FIX] 输入已经是 list，直接排序
    return sorted(tags_list, key=lambda x: x['id'])

# ==============================================================================
# 2. 逻辑层：合并策略 (The Merger)
# ==============================================================================

def build_effective_contract(schema_path, product_path, local_path):
    print(f"--- Loading Schema: {schema_path}")
    schema_data = load_yaml(schema_path)
    if not schema_data or 'tags' not in schema_data:
        print("[ERROR] Invalid Schema file")
        sys.exit(1)

    # 1. 构建全量 Tag 字典 (Name -> Info)
    all_tags_map = {t['name']: t for t in schema_data['tags']}
    
    # 2. 合并本地扩展 (Local Extension)
    if local_path:
        print(f"--- Loading Extension: {local_path}")
        local_data = load_yaml(local_path)
        if local_data and 'tags' in local_data:
            for t in local_data['tags']:
                if t['name'] in all_tags_map:
                    print(f"[WARN] Local extension overwriting global tag: {t['name']}")
                all_tags_map[t['name']] = t

    # 3. 加载产品配置
    print(f"--- Loading Product: {product_path}")
    prod_conf = load_yaml(product_path)
    
    # 4. 加载并注入协议绑定 (Bindings)
    if 'includes' in prod_conf:
        # 计算 includes 的相对路径基准
        # 假设 include 路径是相对于项目根目录的
        for inc_path in prod_conf['includes']:
            if not os.path.exists(inc_path):
                print(f"[WARN] Binding file not found: {inc_path}")
                continue
                
            binding_data = load_yaml(inc_path)
            proto = binding_data.get('protocol', 'UNKNOWN')
            
            for m in binding_data['mappings']:
                tag_name = m['tag']
                if tag_name not in all_tags_map:
                    continue
                
                # 注入协议信息
                target_tag = all_tags_map[tag_name]
                if 'bindings' not in target_tag:
                    target_tag['bindings'] = []
                
                # 标记协议类型
                m['protocol_type'] = proto
                target_tag['bindings'].append(m)

    # 5. 应用裁剪与覆盖 (Overrides)
    active_tags = [] # 这是一个 List
    
    # Get the set of enabled tags from both product and local files (opt-in)
    prod_enabled = set(prod_conf.get('enabled_tags', []))
    local_data = load_yaml(local_path) if local_path else None
    local_enabled = set(local_data.get('enabled_tags', []) if local_data else [])
    enabled_names = prod_enabled.union(local_enabled)

    # Also include tags defined inline in the local extension, as they are implicitly enabled
    if local_data and 'tags' in local_data:
        for t in local_data['tags']:
            enabled_names.add(t['name'])

    overrides = prod_conf.get('overrides', {})

    # Iterate only through the explicitly enabled tags
    for name in sorted(list(enabled_names)):
        if name not in all_tags_map:
            print(f"[WARN] Enabled tag '{name}' not found in global schema.")
            continue

        info = all_tags_map[name]
        ovr = overrides.get(name, {})
        
        # Still allow overrides to disable an enabled tag
        if ovr.get('enabled') is False:
            continue
            
        final_tag = info.copy()
        
        # Merge attributes from overrides
        if 'attributes' not in final_tag: final_tag['attributes'] = {}
        for k, v in ovr.items():
            if k != 'enabled': final_tag['attributes'][k] = v
            
        active_tags.append(final_tag)

    return {
        "product_name": prod_conf.get('product', 'Unknown'),
        "timestamp": str(datetime.datetime.now()),
        "all_tag_names": list(all_tags_map.keys()),
        "active_tags": active_tags # 返回 List
    }

# ==============================================================================
# 3. 生成层：Header 生成 (contracts.h)
# ==============================================================================

def gen_header(contract, out_path):
    print(f"--- Generating Header: {out_path}")
    active_tags = contract['active_tags']
    all_names = contract['all_tag_names']
    active_names_set = set(t['name'] for t in active_tags)

    with open(out_path, 'w') as f:
        f.write("/* AUTO-GENERATED FILE. DO NOT EDIT. */\n")
        f.write(f"/* Product: {contract['product_name']} */\n")
        f.write(f"/* Time:    {contract['timestamp']} */\n\n")
        f.write("#ifndef EDGE_CONTRACTS_H\n#define EDGE_CONTRACTS_H\n\n")
        f.write("#include <stdint.h>\n#include <stddef.h>\n#include <stdbool.h>\n\n")

        # 1. Feature Gates
        f.write("// [Section 1] Feature Gates (1=Enabled, 0=Disabled)\n")
        for name in sorted(all_names):
            val = 1 if name in active_names_set else 0
            f.write(f"#define TAG_{name}_ENABLED {val}\n")
        f.write("\n#define IS_ENABLED(x) ((x) == 1)\n\n")

        # 2. Tag ID Enum
        f.write("// [Section 2] Tag ID Enum\n")
        f.write("typedef enum {\n")
        f.write("    TAG_INVALID = 0,\n")
        for t in sort_tags_by_id(contract['active_tags']):
            f.write(f"    TAG_{t['name']} = {t['id']},\n")
        f.write("    TAG_MAX_ID\n")
        f.write("} edge_tag_id_t;\n\n")

        # 3. Data Structures
        f.write("// [Section 3] Data Structures\n")
        
        f.write("typedef struct {\n")
        f.write("    const char* name;\n")
        f.write("    const char* unit;\n")
        f.write("    bool persist;\n")
        f.write("    bool upload;\n")
        f.write("} tag_meta_t;\n\n")
        
        f.write("typedef struct {\n")
        f.write("    uint16_t addr;\n")
        f.write("    edge_tag_id_t id;\n")
        f.write("    float scale;\n")
        f.write("} modbus_map_t;\n\n")

        f.write("typedef struct {\n")
        f.write("    const char* obis;\n")
        f.write("    edge_tag_id_t id;\n")
        f.write("    float scaler;\n")
        f.write("} dlms_map_t;\n\n")

        f.write("typedef struct {\n")
        f.write("    int ioa;\n")
        f.write("    edge_tag_id_t id;\n")
        f.write("    int type_id;\n")
        f.write("} iec104_map_t;\n\n")

        f.write("typedef struct {\n")
        f.write("    uint32_t can_id;\n")
        f.write("    edge_tag_id_t id;\n")
        f.write("    int start;\n")
        f.write("    int len;\n")
        f.write("    float factor;\n")
        f.write("    float offset;\n")
        f.write("} can_map_t;\n\n")

        # 4. API
        f.write("// [Section 4] Globals & APIs\n")
        f.write("extern const tag_meta_t g_tag_meta_table[];\n")
        
        f.write("const tag_meta_t* edge_get_meta(edge_tag_id_t id);\n")
        f.write("edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale);\n")
        f.write("edge_tag_id_t edge_lookup_dlms(const char* obis, float *out_scale);\n")
        f.write("edge_tag_id_t edge_lookup_iec104(int ioa);\n")
        
        # CAN 查找比较复杂，通常是遍历或 Hash，这里生成数组供驱动遍历
        f.write("extern const can_map_t g_can_map[];\n")
        f.write("extern const int g_can_map_len;\n")

        f.write("\n#endif // EDGE_CONTRACTS_H\n")

# ==============================================================================
# 4. 生成层：Source 生成 (contracts.c)
# ==============================================================================

# ==============================================================================
# 4. 生成层：Source 生成 (contracts.c)
# ==============================================================================

def gen_source(contract, out_path):
    print(f"--- Generating Source: {out_path}")
    active_tags = sort_tags_by_id(contract['active_tags'])
    
    modbus_list = []
    dlms_list = []
    iec104_list = []
    can_list = []
    
    for t in active_tags:
        if 'bindings' in t:
            for b in t['bindings']:
                b_merged = b.copy()
                b_merged['tag_name'] = t['name']
                
                # [FIX] 兼容性修复：
                # 1. 'protocol_type' 是从 bindings/*.yaml 自动注入的内部字段
                # 2. 'protocol' 是用户在 extension.yaml 里手写的字段
                ptype = b.get('protocol_type', b.get('protocol'))
                
                if not ptype:
                    print(f"[WARN] Tag '{t['name']}' binding has no protocol defined. Skipping.")
                    continue
                
                # 统一转大写比较
                ptype = ptype.upper()

                if ptype == 'MODBUS': modbus_list.append(b_merged)
                elif ptype == 'DLMS': dlms_list.append(b_merged)
                elif ptype == 'IEC104': iec104_list.append(b_merged)
                elif ptype == 'CAN': can_list.append(b_merged)

    with open(out_path, 'w') as f:
        f.write('#include "contracts.h"\n')
        f.write('#include <string.h>\n\n')

        # 1. Metadata
        f.write("// --- Metadata Table ---\n")
        f.write("const tag_meta_t g_tag_meta_table[] = {\n")
        for t in active_tags:
            persist = 'true' if t.get('attributes', {}).get('persist') else 'false'
            upload = 'true' if t.get('attributes', {}).get('upload') else 'false'
            unit = t.get('unit', '')
            f.write(f"    [TAG_{t['name']}] = {{ \"{t['name']}\", \"{unit}\", {persist}, {upload} }},\n")
        f.write("};\n\n")
        
        f.write("const tag_meta_t* edge_get_meta(edge_tag_id_t id) {\n")
        f.write("    if (id >= TAG_MAX_ID) return NULL;\n")
        f.write("    return &g_tag_meta_table[id];\n")
        f.write("}\n\n")

        # 2. Modbus
        # 必须先排序才能二分查找
        modbus_list.sort(key=lambda x: x['addr'])
        f.write("// --- Modbus Map ---\n")
        f.write("static const modbus_map_t s_modbus_map[] = {\n")
        for m in modbus_list:
            scale = m.get('scale', 1.0)
            f.write(f"    {{ {m['addr']}, TAG_{m['tag_name']}, {scale}f }},\n")
        f.write("};\n")
        f.write(f"static const int s_modbus_len = {len(modbus_list)};\n\n")
        
        f.write("edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale) {\n")
        f.write("    int left = 0, right = s_modbus_len - 1;\n")
        f.write("    while (left <= right) {\n")
        f.write("        int mid = left + (right - left) / 2;\n")
        f.write("        if (s_modbus_map[mid].addr == reg) {\n")
        f.write("            if (out_scale) *out_scale = s_modbus_map[mid].scale;\n")
        f.write("            return s_modbus_map[mid].id;\n")
        f.write("        }\n")
        f.write("        if (s_modbus_map[mid].addr < reg) left = mid + 1;\n")
        f.write("        else right = mid - 1;\n")
        f.write("    }\n")
        f.write("    return TAG_INVALID;\n")
        f.write("}\n\n")

        # 3. DLMS
        f.write("// --- DLMS Map ---\n")
        f.write("static const dlms_map_t s_dlms_map[] = {\n")
        for m in dlms_list:
             f.write(f"    {{ \"{m['obis']}\", TAG_{m['tag_name']}, {m.get('scaler', 1.0)}f }},\n")
        f.write("};\n")
        f.write(f"static const int s_dlms_len = {len(dlms_list)};\n\n")
        
        f.write("edge_tag_id_t edge_lookup_dlms(const char* obis, float *out_scale) {\n")
        f.write("    for(int i=0; i<s_dlms_len; i++) {\n")
        f.write("        if(strcmp(s_dlms_map[i].obis, obis) == 0) {\n")
        f.write("            if (out_scale) *out_scale = s_dlms_map[i].scaler;\n")
        f.write("            return s_dlms_map[i].id;\n")
        f.write("        }\n")
        f.write("    }\n")
        f.write("    return TAG_INVALID;\n")
        f.write("}\n\n")

        # 4. IEC104
        iec104_list.sort(key=lambda x: x['ioa'])
        f.write("// --- IEC104 Map ---\n")
        f.write("static const iec104_map_t s_iec104_map[] = {\n")
        for m in iec104_list:
            f.write(f"    {{ {m['ioa']}, TAG_{m['tag_name']}, {m.get('type_id', 0)} }},\n")
        f.write("};\n")
        f.write(f"static const int s_iec104_len = {len(iec104_list)};\n\n")
        
        f.write("edge_tag_id_t edge_lookup_iec104(int ioa) {\n")
        f.write("    int left = 0, right = s_iec104_len - 1;\n")
        f.write("    while (left <= right) {\n")
        f.write("        int mid = left + (right - left) / 2;\n")
        f.write("        if (s_iec104_map[mid].ioa == ioa) return s_iec104_map[mid].id;\n")
        f.write("        if (s_iec104_map[mid].ioa < ioa) left = mid + 1;\n")
        f.write("        else right = mid - 1;\n")
        f.write("    }\n")
        f.write("    return TAG_INVALID;\n")
        f.write("}\n")
        
        # 5. CAN
        f.write("// --- CAN Map ---\n")
        f.write("const can_map_t g_can_map[] = {\n")
        for m in can_list:
            f.write(f"    {{ {m['can_id']}, TAG_{m['tag_name']}, {m['start_bit']}, {m['length']}, {m.get('factor', 1.0)}, {m.get('offset', 0.0)} }},\n")
        f.write("};\n")
        f.write(f"const int g_can_map_len = {len(can_list)};\n")
# ==============================================================================
# 5. 主程序入口
# ==============================================================================

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='EdgeOS Contract Compiler')
    parser.add_argument('--schema', required=True, help='Global tags schema path')
    parser.add_argument('--product', required=True, help='Product config path')
    parser.add_argument('--local', help='Local extension path (Optional)')
    parser.add_argument('--out', required=True, help='Output directory')
    
    args = parser.parse_args()
    
    # 1. 编译
    contract = build_effective_contract(args.schema, args.product, args.local)
    
    if not os.path.exists(args.out):
        os.makedirs(args.out)

    # 2. 导出中间态
    eff_yaml_path = os.path.join(args.out, "contracts.yaml")
    with open(eff_yaml_path, 'w', encoding='utf-8') as f:
        clean_tags = [] 
        for t in contract['active_tags']:
             clean_tags.append(t)
        
        yaml.dump({
            "product": contract['product_name'],
            "generated_at": str(datetime.datetime.now()),
            "tags": clean_tags
        }, f, sort_keys=False, allow_unicode=True)
    
    print(f"Generated Audit YAML: {eff_yaml_path}")

    # 3. 生成代码
    gen_header(contract, os.path.join(args.out, "contracts.h"))
    gen_source(contract, os.path.join(args.out, "contracts.c"))
    
    print(f"Generated C Code in: {args.out}")