import sqlite3
import os
import sys

# 设置默认电表数量
NUM_METERS = 2000
NUM_CHANNELS = 32

db_path = "/workspaces/edgeos/EdgeC/build/products/dcu/bin/dcu_runtime.db"

def provision():
    # 确保目录存在
    os.makedirs(os.path.dirname(db_path), exist_ok=True)
    
    # 强制清理旧库及其 WAL 缓存
    for f in [db_path, db_path + "-wal", db_path + "-shm"]:
        if os.path.exists(f):
            os.remove(f)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # 档案表
    cursor.execute("""
    CREATE TABLE meter_inventory (
        address TEXT PRIMARY KEY, protocol TEXT, baud INTEGER, channel INTEGER, 
        wiring_mode INTEGER, collector_addr TEXT, asset_no TEXT, pt_ratio INTEGER, ct_ratio INTEGER
    )
    """)

    meters = []
    for i in range(1, NUM_METERS + 1):
        addr = f"{10000000 + i}"
        channel = (i - 1) % NUM_CHANNELS
        proto = "DLT698" if channel < 22 else "DLT645"
        meters.append((addr, proto, 2400, channel, 1, "123456789012", addr, 1, 1))

    cursor.executemany("INSERT INTO meter_inventory VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)", meters)
    
    # 业务表
    cursor.execute("CREATE TABLE IF NOT EXISTS telemetry (timestamp INTEGER, tag_id INTEGER, value REAL, device_id INTEGER, quality INTEGER)")
    cursor.execute("CREATE TABLE IF NOT EXISTS event_history (meter_address TEXT, oi INTEGER, start_time INTEGER, end_time INTEGER, snapshot_value REAL, PRIMARY KEY (meter_address, oi, start_time))")
    cursor.execute("CREATE TABLE IF NOT EXISTS dcu_freeze_history (meter_address TEXT, freeze_oad INTEGER, freeze_time INTEGER, source_oad INTEGER, frozen_value REAL, PRIMARY KEY (meter_address, freeze_oad, freeze_time, source_oad))")

    conn.commit()
    conn.close()

    print(f"📊 [PROVISION] 成功注入 {NUM_METERS} 只电能表数字孪生档案！")

if __name__ == "__main__":
    provision()
