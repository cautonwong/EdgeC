import subprocess
import time
import requests
import sqlite3
import os
import json

IEMS_BIN = "./build/products/iems/bin/iems"
SIM_BIN = "./build/products/device_simulator/bin/device_simulator"
VBUS = "/tmp/vbus0.sock"
DB_PATH = "iems.db"

def run_test():
    scorecard = []
    print("\n🚀 [OMNIS] Starting Black-Box Integration Test...")
    
    if os.path.exists(VBUS): os.remove(VBUS)
    if os.path.exists(DB_PATH): os.remove(DB_PATH)

    # 1. 启动 IEMS (被测系统)
    iems_proc = subprocess.Popen([IEMS_BIN], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(2) # 等待 UDS Server 启动

    # 2. 模拟数据注入过程 (分步执行剧本逻辑)
    print("🎬 [SIM] Phase 1: Normal Operation (230V)...")
    subprocess.run([SIM_BIN, VBUS], timeout=10) # 模拟器目前是运行完剧本就退出的

    # 3. 验证 REST API (实时观测)
    print("🔍 [REST] Checking real-time status...")
    try:
        resp = requests.get("http://localhost:8081/api/v1/data/latest/1001", timeout=2)
        val = resp.json().get("value")
        if val == 275.0:
            scorecard.append("[PASS] REST API reported simulated Overvoltage.")
        else:
            scorecard.append(f"[FAIL] REST API value mismatch: {val}")
    except Exception as e:
        scorecard.append(f"[FAIL] REST API unreachable: {e}")

    # 4. 验证 SQLite (持久化观测)
    print("💾 [DB] Checking persistence and reflex arc...")
    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()
        # 检查是否因为过压反射触发了 Tag 8001 (湿度开关)
        cursor.execute("SELECT value FROM telemetry WHERE tag_id=8001 LIMIT 1")
        row = cursor.fetchone()
        if row:
            scorecard.append("[PASS] Reflex Arc triggered! Logic loop closed.")
        else:
            scorecard.append("[FAIL] No reflex command found in DB.")
        conn.close()
    except Exception as e:
        scorecard.append(f"[FAIL] DB check failed: {e}")

    # 5. 清理
    iems_proc.terminate()
    print("\n--- 📝 INTEGRATION SCORECARD ---")
    for item in scorecard: print(item)
    
    return all("[PASS]" in s for s in scorecard)

if __name__ == "__main__":
    success = run_test()
    exit(0 if success else 1)
