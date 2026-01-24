import sys
import os
import subprocess
import time
import requests
import json

# 设置环境
project_root = os.path.abspath(os.getcwd())
sys.path.append(project_root)
os.environ["PYTHONPATH"] = project_root

def run_verify():
    print("🚀 Starting Integrated Omnis Verification...")
    
    # 1. 启动 Matrix Server
    server_proc = subprocess.Popen(
        ["python3", "tools/matrix/server.py", "--product", "iems"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    time.sleep(3) # 等待资源供给完成
    
    # 2. 启动 EdgeC (手动注入环境变量)
    env = os.environ.copy()
    env["OMNIS_VBUS_PATH"] = "/tmp/vbus0.sock"
    env["OMNIS_ADC_PATH"] = "/tmp/v_adc_sim.sock"
    env["LD_LIBRARY_PATH"] = os.path.join(project_root, ".local/linux-x86_64/lib")
    
    edgec_proc = subprocess.Popen(
        ["./build/products/iems/bin/iems"],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    
    time.sleep(5) # 等待 IEMS 采集几轮
    
    results = {"status": "FAILED", "details": []}
    
    try:
        # 3. 验证 1: 测试控制平面 API
        print("➡️ Testing Control Plane API (Set 265.5V)...")
        api_url = "http://localhost:50051/api/v1/actor/set"
        payload = {"dev_id": "METER_MAIN", "tag": "VOLTAGE_PHASE_A", "value": 265.5}
        resp = requests.post(api_url, json=payload, timeout=2)
        if resp.status_code == 200:
            print("✅ Control Plane accepted command.")
            results["details"].append("API_OK")
        
        time.sleep(2) # 等待采集
        
        # 4. 验证 2: 测试 DataHub 反映的值
        print("➡️ Checking EdgeOS DataHub via REST API...")
        rest_url = "http://localhost:9091/api/v1/data/latest/1001"
        resp = requests.get(rest_url, timeout=2)
        if resp.status_code == 200:
            val = resp.json().get("value")
            print(f"📊 EdgeOS Reported Value: {val}")
            if 265.0 <= val <= 266.0:
                print("✅ End-to-End Data Flow SUCCESS!")
                results["status"] = "SUCCESS"
                results["details"].append("DATA_FLOW_OK")
            else:
                print(f"❌ Data mismatch. Expected ~265.5, got {val}")
        
    except Exception as e:
        print(f"💥 Error during verification: {e}")
    
    # 清理
    server_proc.terminate()
    edgec_proc.terminate()
    print("\n🏁 Verification Finished.")
    return results

if __name__ == "__main__":
    res = run_verify()
    if res["status"] == "SUCCESS":
        sys.exit(0)
    else:
        sys.exit(1)
