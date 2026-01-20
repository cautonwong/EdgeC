import os
import subprocess
import time
import signal

def run_diagnostics():
    print("🕵️  Matrix/EdgeC End-to-End Diagnostic Tool")
    os.system("pkill -9 -f iems")
    os.system("pkill -9 -f tools/matrix")
    
    env = os.environ.copy()
    env["PYTHONPATH"] = os.getcwd()
    env["OMNIS_VBUS_PATH"] = "/tmp/vbus0.sock"
    env["OMNIS_ADC_PATH"] = "/tmp/v_adc_sim.sock"
    env["LD_LIBRARY_PATH"] = os.path.join(os.getcwd(), ".local/linux-x86_64/lib")

    # 1. 启动 Server
    print("\n[1/2] Starting Matrix Server...")
    server = subprocess.Popen(
        ["python3", "tools/matrix/server.py", "--product", "iems"],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )
    
    # 等待 Server 就绪
    ready = False
    for _ in range(10):
        line = server.stdout.readline()
        print(f"Server: {line.strip()}")
        if "fully operational" in line:
            ready = True
            break
        time.sleep(0.5)
    
    if not ready:
        print("❌ Server failed to become operational.")
        return

    # 2. 启动 Engine
    print("\n[2/2] Starting IEMS Engine...")
    engine = subprocess.Popen(
        ["./build/products/iems/bin/iems"],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )

    # 捕获 10 秒输出
    start_time = time.time()
    success = False
    while time.time() - start_time < 15:
        line = engine.stdout.readline()
        if not line: break
        print(f"Engine: {line.strip()}")
        if "System Online" in line:
            print("\n✅ [SUCCESS] IEMS Engine is fully online!")
            success = True
        if "ERROR" in line or "Failed" in line:
            print(f"⚠️  Detected potential issue: {line.strip()}")

    if not success:
        print("\n❌ [FAIL] Engine timed out or crashed before going online.")

    # 3. 检查数据
    if success:
        print("\n[3/3] Checking real-time data flow...")
        import requests
        try:
            r = requests.get("http://localhost:9091/api/v1/data/latest/1003", timeout=2)
            print(f"Data Check (Tag 1003): {r.json()}")
        except Exception as e:
            print(f"Data Check Failed: {e}")

    # 清理
    print("\nCleaning up...")
    os.killpg(os.getpgid(server.pid), signal.SIGTERM)
    os.killpg(os.getpgid(engine.pid), signal.SIGTERM)

if __name__ == "__main__":
    run_diagnostics()
