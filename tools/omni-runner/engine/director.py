import subprocess
import time
import os
import signal
import json
import random
import socket
import shutil
from .parser import ContractParser

import requests
from tenacity import retry, stop_after_delay, wait_fixed

class OmnisDirector:
    def __init__(self, product_dir, log_dir=None):
        self.product_dir = os.path.abspath(product_dir)
        # 兼容 shadow_schema.json
        manifest_path = os.path.join(self.product_dir, "generated/shadow_schema.json")
        
        with open(manifest_path, "r") as f:
            self.manifest = json.load(f)
            
        self.product_name = self.manifest.get("Product", "unknown")
        # 直接使用同目录下的 contracts.yaml
        c_path = os.path.join(os.path.dirname(manifest_path), "contracts.yaml")
        if not os.path.isabs(c_path):
             c_path = os.path.join(self.product_dir, "generated", c_path)
        self.contract_path = c_path
        self.parser = ContractParser(self.contract_path)
        
        self.bin_dir = os.path.abspath(os.path.join(self.product_dir, "bin"))
        self.bin_path = self._find_binary()
        
        self.process_edgec = None
        self.actors = [] # 存储 (process, control_path)
        self.rest_port = random.randint(15000, 25000)
        self.db_path = f"{self.product_name}.db"
        
        # [镜像化布局]
        if log_dir:
            self.log_dir = log_dir
        else:
            # 默认指向 build/products/<prod>/logs
            self.log_dir = os.path.join(self.product_dir, "logs")
            
        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir, exist_ok=True)

    def _find_binary(self):
        for f in os.listdir(self.bin_dir):
            path = os.path.join(self.bin_dir, f)
            if os.path.isfile(path) and os.access(path, os.X_OK):
                return path
        raise FileNotFoundError(f"No executable found in {self.bin_dir}")

    @retry(stop=stop_after_delay(10), wait=wait_fixed(0.5))
    def wait_until_ready(self):
        url = f"http://127.0.0.1:{self.rest_port}/api/v1/system/health"
        resp = requests.get(url, timeout=1)
        if resp.status_code == 200:
            print(f"✅ [Director] System ready at port {self.rest_port}")
            return True
        raise Exception("System not ready")

    def start_edgec(self):
        print(f"🎬 [Director] Starting EdgeC. Logs: {self.log_dir}/edgec.log")
        env = os.environ.copy()
        project_root = os.path.abspath(os.path.join(self.product_dir, "../../../.."))
        local_lib = os.path.join(project_root, ".local/linux-x86_64/lib")
        env["LD_LIBRARY_PATH"] = f"{local_lib}:{env.get('LD_LIBRARY_PATH', '')}"
        env["EDGE_REST_PORT"] = str(self.rest_port)
        
        if os.path.exists(self.db_path): os.remove(self.db_path)

        self.log_edgec_f = open(os.path.join(self.log_dir, "edgec.log"), "w")
        self.process_edgec = subprocess.Popen(
            [self.bin_path], 
            stdout=self.log_edgec_f, 
            stderr=self.log_edgec_f, 
            env=env, 
            preexec_fn=os.setsid
        )
        
        try:
            self.wait_until_ready()
            return True
        except Exception as e:
            print(f"❌ [Director] Startup timeout: {e}")
            self.stop_all()
            return False

    def spawn_actor(self, protocol):
        print(f"🎭 [Director] Spawning {protocol} Actor. Logs: {self.log_dir}/mimic_{protocol}.log")
        log_f = open(os.path.join(self.log_dir, f"mimic_{protocol}.log"), "w")
        
        env = os.environ.copy()
        current_tool_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        env["PYTHONPATH"] = f"{current_tool_dir}:{env.get('PYTHONPATH', '')}"
        env["OMNIS_PRODUCT"] = self.product_name
        env["OMNIS_CONTRACT_PATH"] = self.contract_path
        
        # [关键改进] 显式指定控制端口文件，不再依赖 PID 猜测
        ctrl_path = os.path.join(self.log_dir, f"mimic_{protocol}.ctrl")
        env["OMNIS_ACTOR_CTRL"] = ctrl_path

        proc = subprocess.Popen(
            ["python3", "-m", f"actors.{protocol.lower()}"],
            stdout=log_f, 
            stderr=log_f, 
            env=env,
            preexec_fn=os.setsid
        )
        self.actors.append((proc, ctrl_path))
        
        # 等待控制套接字就绪
        start = time.time()
        while time.time() - start < 5:
            if os.path.exists(ctrl_path): break
            time.sleep(0.2)
            
        return proc

    def inject_value(self, tag_name, value):
        cmd = {"action": "set", "tag": tag_name, "value": value}
        success = False
        for actor, ctrl_path in self.actors:
            if os.path.exists(ctrl_path):
                try:
                    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    s.settimeout(2)
                    s.connect(ctrl_path)
                    s.send(json.dumps(cmd).encode())
                    s.close()
                    print(f"📡 [Director] Injected {tag_name} -> {value} via {ctrl_path}")
                    success = True
                except Exception as e:
                    print(f"❌ [Director] Injection failed on {ctrl_path}: {e}")
        
        if not success:
            raise RuntimeError(f"Could not inject value {value} to tag {tag_name}: No active actor found")

    def stop_all(self):
        if self.process_edgec:
            try:
                os.killpg(os.getpgid(self.process_edgec.pid), signal.SIGTERM)
                self.process_edgec.wait(timeout=2)
            except: pass
        
        for actor, _ in self.actors:
            try:
                os.killpg(os.getpgid(actor.pid), signal.SIGTERM)
                actor.wait(timeout=2)
            except: pass
        
        if hasattr(self, 'log_edgec_f'): self.log_edgec_f.close()
        self.actors = []
