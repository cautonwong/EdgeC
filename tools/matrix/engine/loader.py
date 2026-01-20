import yaml
import os
from typing import List, Dict, Any

class WorldLoader:
    def __init__(self, world_file: str):
        if not os.path.exists(world_file):
            raise FileNotFoundError(f"World configuration not found: {world_file}")
        
        with open(world_file, 'r') as f:
            self.raw_data = yaml.safe_load(f)
            
        if self.raw_data.get('apiVersion') != 'v3':
            print(f"WARNING: Unexpected apiVersion {self.raw_data.get('apiVersion')}, expected v3")

    def get_instances(self) -> List[Dict[str, Any]]:
        return self.raw_data.get('edge_instances', [])

    def get_instance_buses(self, instance_id: str) -> List[Dict[str, Any]]:
        for inst in self.get_instances():
            if inst['id'] == instance_id:
                return inst.get('buses', [])
        return []

    def resolve_os_dev(self, os_dev_str: str) -> str:
        """
        解析 world.yaml 中的 os_dev 字符串。
        例如: 'UDS:/tmp/vbus0.sock' -> '/tmp/vbus0.sock'
        """
        if os_dev_str.startswith("UDS:"):
            return os_dev_str.split(":", 1)[1]
        if os_dev_str.startswith("memfd:"):
            return os_dev_str # 由 provisioner 特殊处理
        return os_dev_str