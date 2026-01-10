import subprocess
import time
import os
import signal
import requests
import random
from .parser import ContractParser

class OmnisDirector:
    def __init__(self, product="iems"):
        self.product = product
        self.base_dir = os.getcwd()
        self.bin_path = os.path.join(self.base_dir, f"build/products/{product}/bin/{product}")
        gen_yaml = os.path.join(self.base_dir, f"build/products/{product}/generated/contracts.yaml")
        self.parser = ContractParser(gen_yaml)
        self.process_edgec = None
        self.process_mimic = None
        self.port = random.randint(10000, 20000)

    def start_all(self):
        print(f"🎬 [Omnis] Starting EdgeC: {self.product} on port {self.port}...")
        
        subprocess.run(["pkill", "-9", "-f", self.product], stderr=subprocess.DEVNULL)
        time.sleep(1)

        env = os.environ.copy()
        local_lib = os.path.join(self.base_dir, ".local/linux-x86_64/lib")
        env["LD_LIBRARY_PATH"] = f"{local_lib}:{env.get('LD_LIBRARY_PATH', '')}"
        env["EDGE_REST_PORT"] = str(self.port)
        
        self.log_edgec = open(f"omnis_{self.product}.log", "w")
        self.process_edgec = subprocess.Popen([self.bin_path], stdout=self.log_edgec, stderr=self.log_edgec, env=env, preexec_fn=os.setsid)
        
        time.sleep(2)
        
        print(f"🎭 [Omnis] Starting Modbus Mimic...")
        self.log_mimic = open(f"omnis_mimic.log", "w")
        self.process_mimic = subprocess.Popen(["python3", "-m", "tests.integration.actors.modbus"], stdout=self.log_mimic, stderr=self.log_mimic, preexec_fn=os.setsid)
        
        time.sleep(2)
        return self.process_edgec.poll() is None

    def stop_all(self):
        for p, name in [(self.process_mimic, "Mimic"), (self.process_edgec, "EdgeC")]:
            if p:
                print(f"🛑 [Omnis] Stopping {name}...")
                try:
                    os.killpg(os.getpgid(p.pid), signal.SIGTERM)
                    p.wait(timeout=5)
                except:
                    pass
        if hasattr(self, 'log_edgec'): self.log_edgec.close()
        if hasattr(self, 'log_mimic'): self.log_mimic.close()

    def get_stats(self, retries=5):
        for i in range(retries):
            try:
                resp = requests.get(f"http://localhost:{self.port}/api/v1/system/stats", timeout=2)
                if resp.status_code == 200: return resp.json()
            except: pass
            time.sleep(1)
        return None

if __name__ == "__main__":
    director = OmnisDirector("iems")
    try:
        if director.start_all():
            print("🚀 [Omnis] Full Chain Linked. Monitoring for 10 seconds...")
            for i in range(10):
                stats = director.get_stats()
                if stats:
                    uptime = stats.get('uptime_sec_proof', 0)
                    writes = stats.get('datahub', {}).get('total_writes', 0)
                    
                    engine = stats.get('engine', {})
                    pipelines = engine.get('pipelines', [])
                    total_events = 0
                    active_stages = []
                    
                    for p in pipelines:
                        for s in p.get('stages', []):
                            ev = s.get('total_events', 0)
                            total_events += ev
                            if ev > 0:
                                active_stages.append(f"{s['name']}({int(ev)})")
                    
                    print(f"   [{i}] Uptime: {uptime:.1f}s | DH Writes: {writes} | Algorithm Events: {total_events}")
                    if active_stages:
                        print(f"       Active Stages: {', '.join(active_stages)}")
                    
                    if writes > 0 and total_events > 0:
                        print("🎉 [Omnis] SUCCESS: Full chain confirmed (IO -> DataHub -> Algorithm)!")
                        break
                time.sleep(1)
        else:
            print("❌ [Omnis] Failed to start system.")
    finally:
        director.stop_all()
