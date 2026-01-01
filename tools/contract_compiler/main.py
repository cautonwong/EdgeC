import os
import argparse
import yaml
from .core.merger import merge_contract
from .plugins.modbus import ModbusGenerator
from .plugins.dlms import DlmsGenerator
from .plugins.iec104 import Iec104Generator
from .plugins.dlt645 import Dlt645Generator
from .plugins.dlt698 import Dlt698Generator

def get_h_tpl():
    return """/* AUTO-GENERATED. DO NOT EDIT. */
#ifndef EDGE_CONTRACTS_H
#define EDGE_CONTRACTS_H
#include \"contract_data_types.h\"

// [Section 1] Feature Gates
__GATES__

// [Section 2] Tag ID Enum
typedef enum {
    TAG_INVALID = 0,
__ENUMS__
    TAG_MAX_ID
} edge_tag_enum_t;

// [Section 3] Protocol Structs
__STRUCTS__

// [Section 4] APIs
const tag_meta_t* edge_get_meta(edge_tag_id_t id);
edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale);
edge_tag_id_t edge_lookup_dlms(const char* obis, float *out_scale);
edge_tag_id_t edge_lookup_iec104(int ioa);
edge_tag_id_t edge_lookup_dlt698(const char* oad, float *out_scale);
edge_tag_id_t edge_lookup_dlt645(const char* di, float *out_scale);
#endif"""

def get_s_tpl():
    return """#include \"contracts.h\"
#include <string.h>

const tag_meta_t g_tag_meta_table[] = {
__META_ROWS__
};

const tag_meta_t* edge_get_meta(edge_tag_id_t id) {
    if (id >= TAG_MAX_ID) return NULL;
    return &g_tag_meta_table[id];
}

__PROTOCOL_FUNCS__"""

def get_mb_tpl():
    return """
static const modbus_map_t s_modbus_map[] = {
__ROWS__
};
static const int s_modbus_len = __COUNT__;

edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale) {
    int left = 0, right = s_modbus_len - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (s_modbus_map[mid].addr == reg) {
            if (out_scale) *out_scale = s_modbus_map[mid].scale;
            return s_modbus_map[mid].id;
        }
        if (s_modbus_map[mid].addr < reg) left = mid + 1;
        else right = mid - 1;
    } return TAG_INVALID;
}"""

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--schema', required=True)
    parser.add_argument('--product', required=True)
    parser.add_argument('--local')
    parser.add_argument('--out', required=True)
    parser.add_argument('--base')
    args = parser.parse_args()

    base = args.base if args.base else os.getcwd()
    contract = merge_contract(args.schema, args.product, args.local, base)
    active_tags = sorted(contract['active_tags'], key=lambda x: x['id'])
    
    if not os.path.exists(args.out): os.makedirs(args.out)
    with open(os.path.join(args.out, "contracts.yaml"), "w", encoding="utf-8") as f:
        yaml.dump(contract, f, sort_keys=False)

    plugins = {
        "MODBUS": ModbusGenerator(), "DLMS": DlmsGenerator(), 
        "IEC104": Iec104Generator(), "DLT645": Dlt645Generator(),
        "DLT698": Dlt698Generator()
    }

    p_data = {}
    for tag in active_tags:
        for b in tag.get('bindings', []):
            pt = b.get('protocol_type', b.get('protocol', 'UNKNOWN')).upper()
            if pt not in p_data: p_data[pt] = []
            item = b.copy(); item['tag_name'] = tag['name']
            p_data[pt].append(item)

    # 1. Header
    active_names = set(t['name'] for t in active_tags)
    gates = "\n".join(["#define TAG_" + t['name'] + "_ENABLED " + str(1 if t['name'] in active_names else 0) 
                       for t in sorted(contract['all_tags'], key=lambda x: x['name'])])
    enums = "\n".join(["    TAG_" + t['name'] + " = " + str(t['id']) + "," for t in active_tags])
    structs = "\n".join([plugins[p].get_c_struct_def() for p in sorted(p_data.keys()) if p in plugins])

    h_final = get_h_tpl().replace("__GATES__", gates).replace("__ENUMS__", enums).replace("__STRUCTS__", structs)

    # 2. Source
    meta = "\n".join(['    [TAG_' + t['name'] + '] = { \"' + t['name'] + '\", \"' + t.get('unit','') + '\", ' + ('true' if t.get('attributes', {}).get('persist') else 'false') + ', ' + ('true' if t.get('attributes', {}).get('upload') else 'false') + ' },' for t in active_tags])
    
    impls = []
    if 'MODBUS' in p_data:
        mbs = sorted(p_data['MODBUS'], key=lambda x: x['addr'])
        rows = "\n".join(['    { ' + str(m['addr']) + ', TAG_' + m['tag_name'] + ', ' + str(m.get('scale', 1.0)) + 'f },' for m in mbs])
        impls.append(get_mb_tpl().replace("__ROWS__", rows).replace("__COUNT__", str(len(mbs))))
    else:
        impls.append("\nedge_tag_id_t edge_lookup_modbus(uint16_t r, float *s) { (void)r; (void)s; return TAG_INVALID; }")

    # DLT698 / DLT645 实现
    for proto in ["DLT698", "DLT645"]:
        if proto in p_data:
            data = p_data[proto]
            field = 'oad' if proto == 'DLT698' else 'di'
            struct_name = proto.lower() + '_map_t'
            rows = "\n".join(['    { \"' + str(m.get(field, '')) + '\", TAG_' + m['tag_name'] + ', ' + str(m.get('scale', 1.0)) + 'f },' for m in data])
            impls.append("\nstatic const " + struct_name + " s_" + proto.lower() + "_map[] = {\n" + rows + "\n};")
            impls.append("static const int s_" + proto.lower() + "_len = " + str(len(data)) + ";")
            impls.append("edge_tag_id_t edge_lookup_" + proto.lower() + "(const char* o, float *s) {")
            impls.append("    for(int i=0; i<s_" + proto.lower() + "_len; i++) {")
            impls.append("        if(strcmp(s_" + proto.lower() + "_map[i]." + field + ", o) == 0) { if(s)*s=s_" + proto.lower() + "_map[i].scale; return s_" + proto.lower() + "_map[i].id; }")
            impls.append("    } return TAG_INVALID; }")
        else:
            impls.append("\nedge_tag_id_t edge_lookup_" + proto.lower() + "(const char* o, float *s) { (void)o; (void)s; return TAG_INVALID; }")

    impls.append("\nedge_tag_id_t edge_lookup_dlms(const char* o, float *s) { (void)o; (void)s; return TAG_INVALID; }")
    impls.append("edge_tag_id_t edge_lookup_iec104(int ioa) { (void)ioa; return TAG_INVALID; }")
    
    s_final = get_s_tpl().replace("__META_ROWS__", meta).replace("__PROTOCOL_FUNCS__", "\n".join(impls))

    with open(os.path.join(args.out, "contracts.h"), "w", encoding="utf-8") as f: f.write(h_final)
    with open(os.path.join(args.out, "contracts.c"), "w", encoding="utf-8") as f: f.write(s_final)
    print("Build success: " + contract['product_name'])

if __name__ == "__main__":
    main()