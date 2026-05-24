import re
import yaml

with open('docs/698_parsed.md', 'r', encoding='utf-8') as f:
    lines = f.readlines()

oads = []
# Match pattern: 2000  3   电压   数据类型：long-unsigned，单位：V，换算：-1
# We also try to catch lines that might be formatted slightly differently.
pattern1 = re.compile(r'^([0-9A-F]{4})\s+(\d+)\s+([^\s]+.*?)\s+数据类型：([a-zA-Z\-]+)(?:[，,]单位：([^，,]+))?(?:[，,]换算：([-\d]+))?')
# Match pattern: 0010   2   正向有功总电能  double-long-unsigned
pattern2 = re.compile(r'^([0-9A-F]{4})\s+(\d+)\s+([^\s]+.*?)\s+(double-long-unsigned|long-unsigned|long|integer|octet-string|visible-string|array|structure)')

for line in lines:
    line = line.strip()
    m = pattern1.search(line)
    if m:
        oi = m.group(1)
        attr = m.group(2)
        desc = m.group(3).strip()
        dtype = m.group(4)
        unit = m.group(5) if m.group(5) else ""
        scale = m.group(6) if m.group(6) else "0"
        oads.append({
            'oi': oi, 'attr': attr, 'desc': desc, 'dtype': dtype, 'unit': unit, 'scale': scale
        })
        continue
        
    m2 = pattern2.search(line)
    if m2:
        oi = m2.group(1)
        attr = m2.group(2)
        desc = m2.group(3).strip()
        dtype = m2.group(4)
        oads.append({
            'oi': oi, 'attr': attr, 'desc': desc, 'dtype': dtype, 'unit': '', 'scale': '0'
        })

print(f"Found {len(oads)} basic OADs.")

# Generate YAML contract bindings
yaml_data = {
    'protocol': 'DLT698',
    'model': 'Standard_Full',
    'mappings': []
}

for o in oads:
    # 构造 tag 名字: OAD_2000_2
    tag_name = f"OAD_{o['oi']}_{o['attr']}"
    addr = f"0x{o['oi']}{int(o['attr']):02X}00" # default index 00
    
    scale_val = 1.0
    if o['scale'] and o['scale'] != '0':
        try:
            scale_val = 10 ** float(o['scale'])
        except:
            pass

    mapping = {
        'tag': tag_name,
        'addr': addr,
        'type': o['dtype'],
        'scale': scale_val,
        'desc': o['desc']
    }
    if o['unit']:
        mapping['unit'] = o['unit']
        
    yaml_data['mappings'].append(mapping)

with open('contracts/bindings/dlt698_full_oads.yaml', 'w', encoding='utf-8') as f:
    yaml.dump(yaml_data, f, allow_unicode=True, sort_keys=False)

print("Generated contracts/bindings/dlt698_full_oads.yaml")
