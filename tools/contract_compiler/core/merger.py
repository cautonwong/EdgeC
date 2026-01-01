import yaml
import os
import sys

def load_yaml(path):
    if not path or not os.path.exists(path): return None
    with open(path, 'r', encoding='utf-8') as f:
        return yaml.safe_load(f)

def merge_contract(schema_path, product_path, local_path, base_dir=None):
    schema = load_yaml(schema_path)
    all_tags = {t['name']: t for t in schema['tags']}
    
    if local_path:
        local = load_yaml(local_path)
        if local and 'tags' in local:
            for t in local['tags']: all_tags[t['name']] = t
        
    product = load_yaml(product_path)
    
    if 'includes' in product:
        for inc in product['includes']:
            inc_abs = inc if os.path.isabs(inc) else os.path.join(base_dir, inc) if base_dir else inc
            data = load_yaml(inc_abs)
            if not data: continue
            proto = data.get('protocol', 'UNKNOWN').upper()
            mapping_list = data.get('mappings', data.get('map', []))
            for m in mapping_list:
                name = m.get('tag')
                if name in all_tags:
                    tag = all_tags[name]
                    if 'bindings' not in tag: tag['bindings'] = []
                    m_f = m.copy(); m_f['protocol_type'] = proto
                    tag['bindings'].append(m_f)

    enabled = set(product.get('enabled_tags', []))
    active_tags = []
    overrides = product.get('overrides', {})
    for name in sorted(list(all_tags.keys())):
        if name not in enabled: continue
        tag = all_tags[name].copy()
        ovr = overrides.get(name, {})
        if ovr.get('enabled') is False: continue
        if 'attributes' not in tag: tag['attributes'] = {}
        for k, v in ovr.items():
            if k == 'enabled': continue
            tag['attributes'][k] = v
        active_tags.append(tag)
        
    return {"product_name": product.get('product', 'Unknown'), "active_tags": active_tags, "all_tags": list(all_tags.values())}
