import sqlite3
import json
import hashlib
import logging
import os
import requests
from typing import Any
from tenacity import retry, stop_after_delay, wait_fixed

logger = logging.getLogger("Matrix.EdgeC")

class MatrixEdgeC:
    def __init__(self, bin_path: str, vbus_path: str, rest_port: int, db_path: str = None):
        self.bin_path = os.path.abspath(bin_path)
        self.vbus_path = vbus_path
        self.rest_port = rest_port
        self.db_path = db_path or f"{os.path.basename(bin_path)}.db"
        self.process = None

    def bootstrap_environment(self, loader: 'SchemaLoader'):
        logger.info(f"Bootstrapping DB sandbox: {self.db_path}")
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            
            # 1. 强制创建并清理表
            cursor.execute("DROP TABLE IF EXISTS inventory")
            cursor.execute("CREATE TABLE inventory (id TEXT PRIMARY KEY, type TEXT, addr INT, protocol TEXT, enabled INT DEFAULT 1)")
            
            cursor.execute("DROP TABLE IF EXISTS poll_tasks")
            cursor.execute("CREATE TABLE poll_tasks (id INTEGER PRIMARY KEY AUTOINCREMENT, device_id TEXT, tag_id INT, interval_ms INT, bus_id INT)")
            
            # 2. 注入拓扑
            topology = loader.world_config.get("topology", [])
            poll_tasks_cfg = loader.world_config.get("poll_tasks", [])

            for bus_cfg in topology:
                bus_id = bus_cfg.get("bus_id", 0)
                devices_on_bus = bus_cfg.get("devices", []) # Devices for this specific bus

                for dev_cfg in devices_on_bus:
                    count = dev_cfg.get("count", 1)
                    proto = dev_cfg.get("protocol", "ModbusRTU")
                    if proto == "Modbus": proto = "ModbusRTU"
                    start_addr = dev_cfg.get("start_address", 1)
                    
                    for i in range(count):
                        addr = start_addr + i
                        dev_id = f"DEV_{bus_id}_{proto}_{addr}"
                        cursor.execute(
                            "INSERT INTO inventory (id, type, addr, protocol, enabled) VALUES (?, ?, ?, ?, ?)",
                            (dev_id, "Simulator", addr, proto, 1)
                        )
                        
                        for task in poll_tasks_cfg:
                            tag_name = task.get("tag")
                            tag_id = loader.name_to_id.get(tag_name, tag_name)
                            # 仿真环境下，强制提频到 500ms 以获得更快的反馈
                            interval = 500 
                            cursor.execute(
                                "INSERT INTO poll_tasks (device_id, tag_id, interval_ms, bus_id) VALUES (?, ?, ?, ?)",
                                (dev_id, tag_id, interval, bus_id)
                            )
            
            # 3. 注入契约 Hash (存入通用参数表)
            schema_str = json.dumps(loader.schema, sort_keys=True)
            chash = hashlib.md5(schema_str.encode()).hexdigest()
            cursor.execute("CREATE TABLE IF NOT EXISTS edge_settings (product TEXT, tag_id INT, val BLOB, PRIMARY KEY(product, tag_id))")
            cursor.execute("INSERT OR REPLACE INTO edge_settings (product, tag_id, val) VALUES (?, ?, ?)", ("GLOBAL", 9999, chash))
            
            conn.commit()
            conn.close()
            logger.info("DB Bootstrap Success.")
        except Exception as e:
            logger.error(f"DB Bootstrap Failed: {e}")

    @retry(stop=stop_after_delay(30), wait=wait_fixed(1))
    def _wait_ready(self):
        # 1. 等待 REST API 就绪
        url = f"http://127.0.0.1:{self.rest_port}/api/v1/system/health"
        requests.get(url, timeout=1).raise_for_status()
        
        # 2. 等待 VBus 连接成功 (通过日志判定)
        if hasattr(self, "log_path") and os.path.exists(self.log_path):
            with open(self.log_path, "r") as f:
                content = f.read()
                if "Connected to VBus" not in content:
                    raise Exception("VBus not connected yet")

    def start(self, log_path: str = "build/matrix_edgec_raw.log"):
        self.log_path = log_path
        import subprocess
        env = os.environ.copy()

    def get_tag(self, tag_ref: Any, loader: 'SchemaLoader' = None):
        tag_id = tag_ref
        if isinstance(tag_ref, str) and loader:
            tag_id = loader.name_to_id.get(tag_ref, tag_ref)
        
        url = f"http://127.0.0.1:{self.rest_port}/api/v1/data/latest/{tag_id}"
        try:
            resp = requests.get(url, timeout=1)
            if resp.status_code == 200:
                val = resp.json().get("value")
                logger.info(f"REST API: Tag {tag_ref} ({tag_id}) -> {val}")
                return val
            else:
                logger.warning(f"REST API: Tag {tag_id} returned status {resp.status_code}")
        except Exception as e:
            logger.error(f"REST API: Error fetching Tag {tag_id}: {e}")
            return None
        return None

    def stop(self):
        if self.process:
            import signal
            try: os.killpg(os.getpgid(self.process.pid), signal.SIGTERM)
            except: pass
            self.process.wait(timeout=5)