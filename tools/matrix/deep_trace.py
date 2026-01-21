import subprocess
import time
import os
import signal

def verify():
    print("🧪 [Omnis Deep Tracer] Initiating diagnostic loop...")
    env = os.environ.copy()
    env["PYTHONPATH"] = os.getcwd()
    env["OMNIS_VBUS_PATH"] = "/tmp/vbus0.sock"
    env["LD_LIBRARY_PATH"] = os.path.join(os.getcwd(), ".local/linux-x86_64/lib")

    # 1. 启动 Matrix Server
    srv_proc = subprocess.Popen(
        ["python3", "tools/matrix/server.py", "--product", "iems"],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )
    time.sleep(3)

    # 2. 启动 IEMS Engine
    eng_proc = subprocess.Popen(
        ["./build/products/iems/bin/iems"],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )

    print("🛰️  Listening for communication traces (5 seconds)...")
    start = time.time()
    all_output = []
    
    try:
        while time.time() - start < 8:
            # 尝试从 Server 读取 RX 报文
            srv_line = srv_proc.stdout.readline()
            if srv_line:
                print(f"  [Server]: {srv_line.strip()}")
                all_output.append(f"S: {srv_line}")
            
            # 尝试从 Engine 读取 TX 日志
            eng_line = eng_proc.stdout.readline()
            if eng_line:
                print(f"  [Engine]: {eng_line.strip()}")
                all_output.append(f"E: {eng_line}")
                
            if "IO-TX" in eng_line or "RX" in srv_line:
                print("💥 SUCCESS! Captured physical traffic.")
                break
    except Exception as e:
        print(f"Error during trace: {e}")
    finally:
        os.killpg(os.getpgid(srv_proc.pid), signal.SIGTERM) if srv_proc.pid else None
        os.killpg(os.getpgid(eng_proc.pid), signal.SIGTERM) if eng_proc.pid else None

if __name__ == "__main__":
    verify()
