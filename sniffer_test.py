import subprocess
import os
import time
import sys

def run():
    print("🚀 Starting Matrix Server...")
    srv = subprocess.Popen([".venv/bin/python3", "tools/matrix/server.py", "--product", "iems"], 
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(5)

    print("💉 Injecting Trigger Data...")
    subprocess.run([".venv/bin/python3", "tools/matrix/matrix_cli.py", "set", "METER_MAIN", "GRID_POWER", "5555.5"])

    env = os.environ.copy()
    env["OMNIS_BUS_0_DEV"] = "/tmp/vbus0.sock"
    env["OMNIS_BUS_200_DEV"] = "/tmp/v_adc.sock"
    env["LD_LIBRARY_PATH"] = env.get("LD_LIBRARY_PATH", "") + ":" + os.path.join(os.getcwd(), ".local/linux-x86_64/lib")
    
    print("🚀 Launching IEMS and Sniffing Output...")
    proc = subprocess.Popen(["./build/products/iems/bin/iems"], env=env, 
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    start = time.time()
    try:
        # 只读 15 秒
        while time.time() - start < 15:
            line = proc.stdout.readline()
            if line:
                print(f"[IEMS] {line.strip()}")
                sys.stdout.flush()
            if proc.poll() is not None:
                print("❌ IEMS Process exited unexpectedly")
                break
    except KeyboardInterrupt:
        pass
    finally:
        print("🛑 Cleaning up...")
        proc.terminate()
        srv.terminate()

if __name__ == "__main__":
    run()
