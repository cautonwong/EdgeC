import json
import os
import logging
import yaml

logger = logging.getLogger("Matrix.Loader")

class SchemaLoader:
    def __init__(self, schema_path: str, world_path: str = None):
        self.schema_path = schema_path
        self.world_path = world_path
        self.tags = {}
        self.name_to_id = {}
        self.modbus_map = {}
        self.dlms_map = {}
        self.world_config = {}
        self.schema = {}
        
        # 加载契约
        if os.path.isdir(schema_path):
            self._load_from_dir(schema_path)
        else:
            self._load_from_file(schema_path)

        # 加载世界配置 (extension.yaml)
        if world_path and os.path.exists(world_path):
            with open(world_path, "r") as f:
                self.world_config = yaml.safe_load(f).get("world", {})
                logger.info(f"Loaded world configuration from {world_path}")

    def get_product_name(self):
        return self.schema.get("Product", "unknown")

    def _load_from_dir(self, directory: str):
        # 优先读取 shadow_schema.json 以获取完整元数据
        json_file = os.path.join(directory, "shadow_schema.json")
        if os.path.exists(json_file):
            with open(json_file, "r") as f:
                self.schema = json.load(f)
                self._parse_schema(self.schema)
        
        yaml_file = os.path.join(directory, "contracts.yaml")
        if os.path.exists(yaml_file):
            with open(yaml_file, "r") as f:
                data = yaml.safe_load(f)
                # contracts.yaml 通常用于 C 驱动，这里作为补充
                pass

    def _load_from_file(self, file_path: str):
        with open(file_path, "r") as f:
            data = yaml.safe_load(f) if file_path.endswith(".yaml") else json.load(f)
            self.schema = data
            self._parse_schema(data)

    def _parse_schema(self, data: dict):
        # shadow_schema.json 实际结构： { "Product": "...", "ActiveTags": [ { "ID": 1001, "Name": "...", "Bindings": [...] } ] }
        active_tags = data.get("ActiveTags", [])
        
        if not active_tags and "active_tags" in data:
            # 兼容旧格式
            active_tags = data["active_tags"]

        for tag_obj in active_tags:
            tid = tag_obj.get("ID", tag_obj.get("id"))
            if tid is None: continue
            
            tid = int(tid)
            self.tags[tid] = tag_obj
            name = tag_obj.get("Name", tag_obj.get("name", str(tid)))
            self.name_to_id[name] = tid
            
            # 处理 Bindings
            bindings = tag_obj.get("Bindings", tag_obj.get("bindings", []))
            self._parse_bindings(tid, bindings)

    def _parse_bindings(self, tid, bindings):
        if not bindings: return
        for bind in bindings:
            # 兼容：ProtocolType (shadow_schema) vs Protocol (contracts.yaml)
            proto = bind.get("ProtocolType", bind.get("Protocol", bind.get("protocol", ""))).upper()
            attrs = bind.get("Attributes", bind.get("attributes", {}))
            
            if proto == "MODBUS":
                # 提取关键属性
                addr = attrs.get("addr", attrs.get("address"))
                if addr is None: continue
                
                # 归一化字段名以适配 Matrix 引擎
                self.modbus_map[tid] = {
                    "Attributes": attrs,
                    "addr": addr,
                    "scale": attrs.get("scaler", attrs.get("scale", 1.0)),
                    "Protocol": "MODBUS"
                }
                logger.debug(f"Mapped Tag {tid} to Modbus Addr {addr}")
            elif proto == "DLMS":
                self.dlms_map[tid] = bind
