import os
import subprocess
import time
import requests

# 调试 IEMS 启动
def debug_iems():
    print("🛠️ DEBUG: Starting IEMS Engine Diagnostics...")
    
    # 彻底清理
    os.system("pkill -9 -f iems")
    
    env = os.environ.copy()
    env["OMNIS_VBUS_PATH"] = "/tmp/vbus0.sock"
    env["OMNIS_ADC_PATH"] = "/tmp/v_adc_sim.sock"
    env["LD_LIBRARY_PATH"] = os.path.join(os.getcwd(), ".local/linux-x86_64/lib")
    
    # 手动创建一个空的 UDS 监听，防止 IEMS 因为找不到总线而长时间阻塞
    import socket
    dummy_vbus = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        if os.path.exists("/tmp/vbus0.sock"): os.unlink("/tmp/vbus0.sock")
        dummy_vbus.bind("/tmp/vbus0.sock")
        dummy_vbus.listen(1)
    except: pass

    # 前台拉起 IEMS 并捕获其 stdout/stderr
    print("🚀 Launching IEMS binary...")
    proc = subprocess.Popen(
        ["./build/products/iems/bin/iems"],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    time.sleep(5)
    
    print("--- [IEMS STDOUT] ---")
    # 读取目前产生的输出
    while True:
        line = proc.stdout.readline()
        if not line: break
        print(f"OUT: {line.strip()}")
        if "System Online" in line: break # 启动成功标志
        if "ERROR" in line: print(f"❌ DETECTED ERROR: {line.strip()}")

    print("--- [IEMS STDERR] ---")
    print(proc.stderr.read())
    
    proc.terminate()

if __name__ == "__main__":
    debug_iems()
