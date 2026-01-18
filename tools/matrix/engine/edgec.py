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
                            interval = task.get("interval_ms", 1000)
                            cursor.execute(
                                "INSERT INTO poll_tasks (device_id, tag_id, interval_ms, bus_id) VALUES (?, ?, ?, ?)",
                                (dev_id, tag_id, interval, bus_id)
                            )
            
            # 3. 注入契约 Hash (存入通用参数表)
            if hasattr(loader, 'schema') and loader.schema:
                schema_str = json.dumps(loader.schema, sort_keys=True)
                chash = hashlib.md5(schema_str.encode()).hexdigest()
                cursor.execute("CREATE TABLE IF NOT EXISTS edge_settings (product TEXT, tag_id INT, val BLOB, PRIMARY KEY(product, tag_id))")
                cursor.execute("INSERT OR REPLACE INTO edge_settings (product, tag_id, val) VALUES (?, ?, ?)", ("GLOBAL", 9999, chash))
            
            conn.commit()
            conn.close()
            logger.info("DB Bootstrap Success.")
        except Exception as e:
            logger.error(f"DB Bootstrap Failed: {e}")

    def start(self, cwd: str = None):
        import subprocess
        env = os.environ.copy()
        env["OMNIS_VBUS_PATH"] = self.vbus_path
        env["EDGE_REST_PORT"] = str(self.rest_port)
        project_root = cwd or os.path.abspath(os.getcwd())
        env["LD_LIBRARY_PATH"] = os.path.join(project_root, ".local/linux-x86_64/lib")

        log_path = os.path.join(project_root, "build/matrix_edgec.log")
        log_f = open(log_path, "w")
        
        logger.info(f"🚀 Launching EdgeC: {self.bin_path} (Port: {self.rest_port})")
        self.process = subprocess.Popen(
            [self.bin_path], 
            env=env, 
            stdout=log_f, 
            stderr=log_f, 
            preexec_fn=os.setsid,
            cwd=project_root # 确保 C 进程能找到 config/zlog.conf
        )
        try:
            self._wait_ready()
        except Exception as e:
            # 如果启动失败，尝试读取最后几行日志
            if os.path.exists(log_path):
                with open(log_path, "r") as f:
                    last_logs = f.readlines()[-10:]
                    logger.error("--- Last EdgeC Logs ---")
                    for line in last_logs: logger.error(line.strip())
            raise RuntimeError(f"EdgeC failed to become ready at {self.rest_port}: {e}")

    @retry(stop=stop_after_delay(30), wait=wait_fixed(1))
    def _wait_ready(self):
        # 0. 存活检查
        if self.process:
            poll_res = self.process.poll()
            if poll_res is not None:
                raise RuntimeError(f"EdgeC process exited immediately with code {poll_res}")

        url = f"http://127.0.0.1:{self.rest_port}/api/v1/system/health"
        
        # 1. Socket 预检查 (探测端口是否监听)
        import socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(0.5)
            if s.connect_ex(('127.0.0.1', self.rest_port)) != 0:
                raise ConnectionError(f"Port {self.rest_port} not listening yet.")

        # 2. HTTP 业务就绪检查
        try:
            resp = requests.get(url, timeout=1)
            resp.raise_for_status()
            logger.info(f"🟢 EdgeC REST API is online (Port: {self.rest_port}).")
        except Exception as e:
            raise e

    def get_tag(self, tag_ref: Any, loader: 'SchemaLoader' = None):
        tag_id = tag_ref
        if isinstance(tag_ref, str) and loader:
            tag_id = loader.name_to_id.get(tag_ref, tag_ref)
        url = f"http://127.0.0.1:{self.rest_port}/api/v1/data/latest/{tag_id}"
        try:
            resp = requests.get(url, timeout=1)
            if resp.status_code == 200:
                return resp.json().get("value")
        except: return None
        return None

    def stop(self):
        if self.process:
            import signal
            try: os.killpg(os.getpgid(self.process.pid), signal.SIGTERM)
            except: pass
            self.process.wait(timeout=5)