import subprocess
import time
import os
import signal
import json
import random
import socket
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
        # 根据产品名确定 DB 路径
        short_name = os.path.basename(os.path.dirname(self.product_dir))
        self.db_path = f"{short_name}.db"

    def _find_binary(self):
        for f in os.listdir(self.bin_dir):
            path = os.path.join(self.bin_dir, f)
            if os.path.isfile(path) and os.access(path, os.X_OK):
                return path
        raise FileNotFoundError(f"No executable found in {self.bin_dir}")

    def start_edgec(self):
        print(f"🎬 [Director] Starting EdgeC: {self.bin_path} on port {self.rest_port}...")
        
        env = os.environ.copy()
        project_root = os.path.abspath(os.path.join(self.product_dir, "../../../.."))
        local_lib = os.path.join(project_root, ".local/linux-x86_64/lib")
        env["LD_LIBRARY_PATH"] = f"{local_lib}:{env.get('LD_LIBRARY_PATH', '')}"
        env["EDGE_REST_PORT"] = str(self.rest_port)
        
        if os.path.exists(self.db_path): os.remove(self.db_path)

        log_file = open(f"omnis_{self.product_name}.log", "w")
        self.process_edgec = subprocess.Popen(
            [self.bin_path], 
            stdout=log_file, 
            stderr=log_file, 
            env=env, 
            preexec_fn=os.setsid
        )
        time.sleep(2)
        return self.process_edgec.poll() is None

    def spawn_actor(self, protocol, name=None):
        print(f"🎭 [Director] Spawning {protocol} Actor...")
        log_file = open(f"omnis_actor_{protocol}.log", "w")
        
        env = os.environ.copy()
        env["PYTHONPATH"] = "."
        env["OMNIS_PRODUCT"] = os.path.basename(os.path.dirname(self.product_dir))
        
        proc = subprocess.Popen(
            ["python3", "-m", f"tests.integration.actors.{protocol.lower()}"],
            stdout=log_file, 
            stderr=log_file, 
            env=env,
            preexec_fn=os.setsid
        )
        self.actors.append(proc)
        time.sleep(1.5)
        return proc

    def inject_value(self, tag_name, value):
        cmd = {"action": "set", "tag": tag_name, "value": value}
        for actor in self.actors:
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
        print("🛑 [Director] Shutting down all processes...")
        if self.process_edgec:
            try:
                os.killpg(os.getpgid(self.process_edgec.pid), signal.SIGTERM)
                self.process_edgec.wait(timeout=5)
            except: pass
        
        for actor in self.actors:
            try:
                ctrl_path = f"/tmp/mimic_modbus_{actor.pid}.ctrl"
                os.killpg(os.getpgid(actor.pid), signal.SIGTERM)
                actor.wait(timeout=2)
                if os.path.exists(ctrl_path): os.remove(ctrl_path)
            except: pass
        self.actors = []
