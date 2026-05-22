import sqlite3
import json
import logging
import os
import requests
import signal
import atexit
import time
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
        atexit.register(self.stop)

    def bootstrap_environment(self, loader: 'SchemaLoader'):
        """同步数据库资产"""
        logger.info(f"Bootstrapping DB sandbox at {self.db_path}")
        if os.path.exists(self.db_path): os.remove(self.db_path)
        
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            cursor.execute("CREATE TABLE inventory (id TEXT PRIMARY KEY, type TEXT, addr INT, protocol TEXT, model TEXT, extra_config TEXT, enabled INT DEFAULT 1)")
            cursor.execute("CREATE TABLE poll_tasks (id INTEGER PRIMARY KEY AUTOINCREMENT, device_id TEXT, tag_id INT, interval_ms INT, bus_id INT)")
            cursor.execute("CREATE TABLE matrix_meta (key TEXT PRIMARY KEY, val TEXT)")
            
            new_devices = loader.data.get("devices", [])
            tags_cfg = loader.data.get("tags", [])
            for dev in new_devices:
                extra_json = json.dumps(dev.get("config", {}))
                cursor.execute("INSERT INTO inventory VALUES (?, ?, ?, ?, ?, ?, ?)",
                             (dev["id"], "Simulator", dev["address"], dev["protocol"], dev["model"], extra_json, 1))
                
                # 协议归一化：ModbusRTU/TCP -> MODBUS
                dev_proto = dev["protocol"].upper()
                if "MODBUS" in dev_proto: dev_proto = "MODBUS"

                for tag in tags_cfg:
                    bindings = tag.get("bindings") or []
                    for b in bindings:
                        tag_proto = b["protocol"].upper()
                        if tag_proto == dev_proto:
                            cursor.execute("INSERT INTO poll_tasks (device_id, tag_id, interval_ms, bus_id) VALUES (?, ?, ?, ?)",
                                         (dev["id"], tag["id"], 1000, dev["bus_id"]))
            
            cursor.execute("INSERT INTO matrix_meta VALUES ('is_ready', '1')")
            conn.commit()
            conn.close()
            logger.info("DB Bootstrap: Committed.")
        except Exception as e:
            logger.error(f"DB Bootstrap Failed: {e}")

    def start(self, cwd: str = None):
        import subprocess
        env = os.environ.copy()
        
        # 透传所有 OMNIS_BUS_* 环境变量给子进程
        for k, v in os.environ.items():
            if k.startswith("OMNIS_BUS_"):
                env[k] = v

        env["EDGE_REST_PORT"] = str(self.rest_port)
        env["EDGE_DB_PATH"] = self.db_path
        if "EDGE_SHM_NAME" in os.environ: env["EDGE_SHM_NAME"] = os.environ["EDGE_SHM_NAME"]
        
        # ASAN 优化：崩溃时立刻退出，不等待交互
        env["ASAN_OPTIONS"] = "detect_leaks=0:abort_on_error=1:log_path=stdout"
        
        project_root = cwd or os.path.abspath(os.getcwd())
        env["LD_LIBRARY_PATH"] = os.path.join(project_root, ".local/linux-x86_64/lib")

        log_path = os.path.join(project_root, "build/matrix_edgec.log")
        # [整改] 使用追加模式，防止多用例覆盖日志
        self.log_file = open(log_path, "a")

        logger.info(f"🚀 Launching EdgeC (Port: {self.rest_port})")
        # 移除 stdout/stderr 映射，直接定向到文件，防止管道阻塞
        self.process = subprocess.Popen(
            [self.bin_path], env=env, stdout=self.log_file, stderr=self.log_file, cwd=project_root
        )
        try:
            self._wait_ready()
        except Exception as e:
            self.log_file.flush()
            self.stop()
            raise RuntimeError(f"EdgeC failed to start: {e}")

    @retry(stop=stop_after_delay(40), wait=wait_fixed(1))
    def _wait_ready(self):
        if self.process.poll() is not None: raise RuntimeError("Process exited")
        url = f"http://127.0.0.1:{self.rest_port}/api/v1/system/health"
        try:
            resp = requests.get(url, timeout=0.5)
            resp.raise_for_status()
            logger.info("🟢 EdgeC REST API is online.")
        except:
            raise ConnectionError("REST Offline")

    def stop(self):
        if self.process:
            try:
                self.process.terminate()
                self.process.wait(timeout=2)
            except:
                try: self.process.kill()
                except: pass
            self.process = None
        if hasattr(self, 'log_file'): self.log_file.close()

    def get_tag(self, tag_id: int):
        url = f"http://127.0.0.1:{self.rest_port}/api/v1/data/latest/{tag_id}"
        try:
            resp = requests.get(url, timeout=0.5)
            return resp.json().get("value") if resp.status_code == 200 else None
        except: return None