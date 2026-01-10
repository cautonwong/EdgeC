import subprocess
import time
import os
import signal
import requests
from contract_parser import ContractParser

class OmnisDirector:
    def __init__(self, product="iems"):
        self.product = product
        self.base_dir = os.getcwd()
        self.bin_path = os.path.join(self.base_dir, f"build/products/{product}/bin/{product}")
        
        # 核心：加载 merge 后的最终契约
        gen_yaml = os.path.join(self.base_dir, f"build/products/{product}/generated/contracts.yaml")
        self.parser = ContractParser(gen_yaml)
        self.process = None

    def start_edgec(self):
        print(f"🎬 [Omnis] Starting EdgeC Binary: {self.product}...")
        env = os.environ.copy()
        local_lib = os.path.join(self.base_dir, ".local/linux-x86_64/lib")
        env["LD_LIBRARY_PATH"] = f"{local_lib}:{env.get('LD_LIBRARY_PATH', '')}"
        
        # 记录日志到文件以便追溯
        self.log_file = open(f"omnis_{self.product}.log", "w")
        
        self.process = subprocess.Popen(
            [self.bin_path],
            stdout=self.log_file,
            stderr=self.log_file,
            env=env,
            preexec_fn=os.setsid
        )
        time.sleep(2)
        if self.process.poll() is not None:
            print(f"❌ [Omnis] EdgeC failed to start. Check omnis_{self.product}.log")
            return False
        print("✅ [Omnis] EdgeC is running.")
        return True

    def stop_edgec(self):
        if self.process:
            print(f"🛑 [Omnis] Stopping EdgeC...")
            try:
                os.killpg(os.getpgid(self.process.pid), signal.SIGTERM)
                self.process.wait(timeout=5)
            except:
                os.killpg(os.getpgid(self.process.pid), signal.SIGKILL)
            self.log_file.close()
            print("✅ [Omnis] EdgeC stopped.")

    def check_health(self):
        port = 8081 if self.product == "iems" else 18081
        try:
            resp = requests.get(f"http://localhost:{port}/api/v1/system/health", timeout=2)
            return resp.status_code == 200
        except:
            return False

if __name__ == "__main__":
    # 验证 Stage 10.3
    try:
        director = OmnisDirector("iems")
        if director.start_edgec():
            health = director.check_health()
            print(f"🏥 [Omnis] System Health Check: {'PASS' if health else 'FAIL'}")
            director.stop_edgec()
    except Exception as e:
        print(f"❌ [Omnis] Setup failed: {e}")
