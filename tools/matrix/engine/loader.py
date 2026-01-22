import yaml
import json
import os
from typing import List, Dict, Any

class WorldLoader:
    """[Legacy] 兼容旧版 world.yaml 加载逻辑"""
    def __init__(self, world_file: str):
        if not os.path.exists(world_file):
            raise FileNotFoundError(f"World configuration not found: {world_file}")
        with open(world_file, 'r') as f:
            self.raw_data = yaml.safe_load(f)

    def get_instances(self) -> List[Dict[str, Any]]:
        return self.raw_data.get('edge_instances', [])

    def get_instance_buses(self, instance_id: str) -> List[Dict[str, Any]]:
        for inst in self.get_instances():
            if inst['id'] == instance_id:
                return inst.get('buses', [])
        return []

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
        self.hardware = self.data.get("hardware", [])

    def get_product_name(self) -> str:
        return self.data.get("product", "unknown")

    def resolve_os_dev(self, bus_id: int, profile: str = "simulation") -> str:
        """根据 BusID 和 Profile 获取物理设备路径 (强制绝对路径)"""
        for bus in self.hardware:
            if bus["id"] == bus_id:
                prof_data = bus.get("profiles", {}).get(profile, {})
                path = prof_data.get("os_dev", "")
                if path:
                    # 处理 UDS: 前缀并转为绝对路径
                    clean_path = path.replace("UDS:", "")
                    if not clean_path.startswith("/"):
                        # 假设相对路径是相对于项目根目录 (即 manifest 所在目录的父级或当前目录)
                        abs_path = os.path.abspath(clean_path)
                        return f"UDS:{abs_path}" if path.startswith("UDS:") else abs_path
                return path
        return ""

    def get_topology(self, bus_id: int) -> List[Dict[str, Any]]:
        """获取某条总线上的模拟设备列表"""
        for bus in self.hardware:
            if bus["id"] == bus_id:
                return bus.get("topology") or []
        return []