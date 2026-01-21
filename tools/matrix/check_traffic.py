import subprocess
import time
import os
import signal

def run():
    print("🛰️  Omnis Real-time Packet Sniffer starting...")
    os.environ["PYTHONPATH"] = os.getcwd()
    os.environ["OMNIS_VBUS_PATH"] = "/tmp/vbus0.sock"
    os.environ["LD_LIBRARY_PATH"] = os.path.join(os.getcwd(), ".local/linux-x86_64/lib")

    # 启动 Server
    srv = subprocess.Popen(["python3", "tools/matrix/server.py", "--product", "iems"], 
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    time.sleep(2)

    # 启动 Engine (核心：通过 PIPE 实时监控其每一行输出)
    eng = subprocess.Popen(["./build/products/iems/bin/iems"], 
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    print("🚀 Monitoring Engine Output (8s timeout)...")
    start = time.time()
    try:
        while time.time() - start < 8:
            line = eng.stdout.readline()
            if line:
                print(f"[Engine] {line.strip()}")
                if "IO-TX" in line:
                    print("💥 PACKET SENT! SUCCESS!")
                    break
            
            # 同时查一下 Server 是否有数据
            srv_line = srv.stdout.readline()
            if srv_line:
                print(f"[Server] {srv_line.strip()}")
                if "RX" in srv_line:
                    print("🎯 SERVER RECEIVED BYTES!")
                    break
    except:
        pass
    finally:
        os.killpg(os.getpgid(srv.pid), signal.SIGTERM)
        os.killpg(os.getpgid(eng.pid), signal.SIGTERM)

if __name__ == "__main__":
    run()
