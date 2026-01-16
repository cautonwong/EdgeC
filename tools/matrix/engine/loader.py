import json
import os
import logging
import yaml

logger = logging.getLogger("Matrix.Loader")

class SchemaLoader:
    def __init__(self, schema_path: str, ext_path: str = None):
        self.tags = {}
        self.modbus_map = {}
        self.dlms_map = {}
        self.world_config = {}
        self.name_to_id = {}
        self.schema = {}
        
        # 1. 加载主契约
        if os.path.isdir(schema_path):
            self._load_from_dir(schema_path)
        else:
            self._load_from_file(schema_path)

        # 2. 加载扩展配置 (World Definition)
        if ext_path and os.path.exists(ext_path):
            with open(ext_path, "r") as f:
                ext_data = yaml.safe_load(f)
                if ext_data and "world" in ext_data:
                    self.world_config = ext_data["world"]
                elif ext_data and "topology" in ext_data:
                    self.world_config = ext_data # 兼容直接定义 topology 的情况

    def get_product_name(self):
        return self.schema.get("product_name") or self.schema.get("Product") or "iems"

    def _load_from_dir(self, directory: str):
        yaml_file = os.path.join(directory, "contracts.yaml")
        if os.path.exists(yaml_file):
            self._load_from_file(yaml_file)
        else:
            logger.error(f"contracts.yaml not found in {directory}")

    def _load_from_file(self, file_path: str):
        with open(file_path, "r") as f:
            data = yaml.safe_load(f) if file_path.endswith(".yaml") else json.load(f)
            self.schema = data
            self._parse_schema(data)

    def _parse_schema(self, data: dict):
        """核心解析逻辑：彻底兼容大小写和命名风格 (Snake/Camel)"""
        def get_fuzzy(d, key):
            if not d: return None
            # 1. 直接匹配
            if key in d: return d[key]
            # 2. 尝试常见变体
            lookups = [
                key.lower(), key.upper(), key.capitalize(),
                key.replace("_", ""), # activetags
                "".join(word.capitalize() for word in key.split("_")), # ActiveTags
            ]
            for l in lookups:
                if l in d: return d[l]
            return None

        active_tags = get_fuzzy(data, "active_tags") or get_fuzzy(data, "tags") or []
        
        logger.info(f"Parsing {len(active_tags)} tags from schema.")
        
        for tag_obj in active_tags:
            tid = get_fuzzy(tag_obj, "id")
            tname = get_fuzzy(tag_obj, "name")
            
            if tid is None or tname is None:
                continue
            
            self.tags[tid] = tag_obj
            self.name_to_id[tname] = tid
            
            # 提取绑定
            binds_list = get_fuzzy(tag_obj, "bindings") or []
            for b in binds_list:
                # shadow_schema.json 里的键名是 ProtocolType 和 TagName
                p = (get_fuzzy(b, "protocol_type") or get_fuzzy(b, "protocol") or "").upper()
                if p in ["MODBUS", "MODBUSRTU"]:
                    self.modbus_map[tid] = b
                    logger.info(f"  [Modbus] Found mapping for Tag {tname} (ID:{tid})")
                elif p == "DLMS":
                    self.dlms_map[tid] = b
        
        logger.info(f"Successfully loaded {len(self.tags)} tags and {len(self.modbus_map)} modbus bindings.")
