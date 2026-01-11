import subprocess
import time
import os
import signal
import json
import random
import socket
import shutil
from tests.integration.engine.parser import ContractParser

class OmnisDirector:
    def __init__(self, product_dir):
        self.product_dir = os.path.abspath(product_dir)
        manifest_path = os.path.join(self.product_dir, "omnis_manifest.json")
        
        with open(manifest_path, "r") as f:
            self.manifest = json.load(f)
            
        self.product_name = self.manifest["product"]
        self.contract_path = os.path.join(self.product_dir, self.manifest["contract"])
        self.parser = ContractParser(self.contract_path)
        
        self.bin_dir = os.path.abspath(os.path.join(self.product_dir, "..", "bin"))
        self.bin_path = self._find_binary()
        
        self.process_edgec = None
        self.actors = []
        self.rest_port = random.randint(15000, 25000)
        self.db_path = f"{os.path.basename(os.path.dirname(self.product_dir))}.db"
        
        # [日志管理中心]
        self.run_id = f"{int(time.time())}_{self.rest_port}"
        self.log_dir = os.path.join(os.getcwd(), "build/omnis_logs", self.run_id)
        if os.path.exists(self.log_dir): shutil.rmtree(self.log_dir)
        os.makedirs(self.log_dir, exist_ok=True)

    def _find_binary(self):
        for f in os.listdir(self.bin_dir):
            path = os.path.join(self.bin_dir, f)
            if os.path.isfile(path) and os.access(path, os.X_OK):
                return path
        raise FileNotFoundError(f"No executable found in {self.bin_dir}")

    def start_edgec(self):
        print(f"🎬 [Director] Starting EdgeC on port {self.rest_port}. Logs: {self.log_dir}/edgec.log")
        
        env = os.environ.copy()
        project_root = os.path.abspath(os.path.join(self.product_dir, "../../../.."))
        local_lib = os.path.join(project_root, ".local/linux-x86_64/lib")
        env["LD_LIBRARY_PATH"] = f"{local_lib}:{env.get('LD_LIBRARY_PATH', '')}"
        env["EDGE_REST_PORT"] = str(self.rest_port)
        
        if os.path.exists(self.db_path): os.remove(self.db_path)

        # 物理隔离的日志文件
        self.log_edgec_f = open(os.path.join(self.log_dir, "edgec.log"), "w")
        self.process_edgec = subprocess.Popen(
            [self.bin_path], 
            stdout=self.log_edgec_f, 
            stderr=self.log_edgec_f, 
            env=env, 
            preexec_fn=os.setsid
        )
        
        # 等待 REST Server 启动
        time.sleep(2)
        return self.process_edgec.poll() is None

    def spawn_actor(self, protocol):
        print(f"🎭 [Director] Spawning {protocol} Actor. Logs: {self.log_dir}/mimic_{protocol}.log")
        log_f = open(os.path.join(self.log_dir, f"mimic_{protocol}.log"), "w")
        
        env = os.environ.copy()
        env["PYTHONPATH"] = "."
        env["OMNIS_PRODUCT"] = os.path.basename(os.path.dirname(self.product_dir))
        
        proc = subprocess.Popen(
            ["python3", "-m", f"tests.integration.actors.{protocol.lower()}"],
            stdout=log_f, 
            stderr=log_f, 
            env=env,
            preexec_fn=os.setsid
        )
        self.actors.append((proc, log_f))
        time.sleep(1.5)
        return proc

    def inject_value(self, tag_name, value):
        cmd = {"action": "set", "tag": tag_name, "value": value}
        for actor, _ in self.actors:
            ctrl_path = f"/tmp/mimic_modbus_{actor.pid}.ctrl"
            if os.path.exists(ctrl_path):
                try:
                    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    s.connect(ctrl_path)
                    s.send(json.dumps(cmd).encode())
                    s.close()
                    print(f"📡 [Director] Injected {tag_name} -> {value}")
                except Exception as e:
                    print(f"❌ [Director] Injection failed: {e}")

    def stop_all(self):
        if self.process_edgec:
            try:
                os.killpg(os.getpgid(self.process_edgec.pid), signal.SIGTERM)
                self.process_edgec.wait(timeout=2)
            except: pass
        
        for actor, log_f in self.actors:
            try:
                os.killpg(os.getpgid(actor.pid), signal.SIGTERM)
                actor.wait(timeout=2)
                log_f.close()
            except: pass
        
        if hasattr(self, 'log_edgec_f'): self.log_edgec_f.close()
        self.actors = []