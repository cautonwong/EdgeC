import yaml
import os

class ContractParser:
    """
    Omnis 契约解析器：直接读取由 contract-compiler 生成的最终版 YAML。
    """
    def __init__(self, generated_yaml_path):
        if not os.path.exists(generated_yaml_path):
            raise FileNotFoundError(f"Generated contract not found at {generated_yaml_path}. Did you build the project?")
            
        with open(generated_yaml_path, 'r') as f:
            self.data = yaml.safe_load(f)
            
        self.tags = {t['name']: t for t in self.data.get('tags', [])}
        self.id_map = {t['id']: t for t in self.data.get('tags', [])}

    def get_tag(self, name):
        return self.tags.get(name)

    def get_tag_by_id(self, tag_id):
        return self.id_map.get(tag_id)

    def get_all_tags(self):
        return self.tags

    def get_product_name(self):
        return self.data.get('product', 'unknown')

    def get_config(self, key):
        configs = {c['name']: c['value'] for c in self.data.get('configs', [])}
        return configs.get(key)

if __name__ == "__main__":
    # 自测：读取 IEMS 生成的契约
    path = "build/products/iems/generated/contracts.yaml"
    if os.path.exists(path):
        parser = ContractParser(path)
        print(f"Product: {parser.get_product_name()}")
        v_a = parser.get_tag("VOLTAGE_PHASE_A")
        print(f"Found Tag {v_a['name']} with ID {v_a['id']}")
    else:
        print("Build artifacts not found, skip self-test.")
