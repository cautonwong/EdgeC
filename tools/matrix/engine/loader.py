import json
import os
from typing import List, Dict, Any

class SchemaLoader:
    """[Omnis] 契约驱动的新一代加载器"""
    def __init__(self, manifest_path: str):
        self.manifest_path = os.path.abspath(manifest_path)
        if not os.path.exists(self.manifest_path):
            raise FileNotFoundError(f"Manifest not found: {self.manifest_path}")
        with open(self.manifest_path, 'r') as f:
            self.data = json.load(f)
        
        self.tags = self.data.get("tags", [])
        self.name_to_id = {t["name"]: t["id"] for t in self.tags}
        self.id_to_name = {t["id"]: t["name"] for t in self.tags}
        self.connectivity = self.data.get("connectivity", [])
        self.devices = self.data.get("devices", [])

    def get_product_name(self) -> str:
        return self.data.get("product", "unknown")

    def resolve_os_dev(self, bus_id: int, mode: str = "simulation") -> str:
        """根据 BusID 获取物理设备路径"""
        for conn in self.connectivity:
            if conn["id"] == bus_id:
                if mode == "simulation":
                    path = conn.get("simulation", {}).get("endpoint", "")
                    if path: return path.replace("UDS:", "")
                else:
                    return conn.get("config", {}).get("path", "")
        return ""
