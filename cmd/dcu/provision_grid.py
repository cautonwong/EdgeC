import sqlite3
import random
import os

DB_PATH = "/workspaces/edgeos/EdgeC/build/products/dcu/bin/dcu_runtime.db"

def provision_grid():
    # 确保目录存在
    os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)
    
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    
    # 1. 创建符合专业架构的表
    cursor.execute("DROP TABLE IF EXISTS meter_inventory;")
    cursor.execute("""
        CREATE TABLE meter_inventory (
            address TEXT PRIMARY KEY, 
            protocol TEXT, 
            baud INTEGER, 
            channel INTEGER,
            wiring_mode INTEGER,
            collector_addr TEXT,
            asset_no TEXT,
            pt_ratio INTEGER,
            ct_ratio INTEGER
        );
    """)

    # 2. 批量生成 2000 只表
    meters = []
    for i in range(1, 2001):
        address = f"{10000000 + i}"
        
        # 协议比例：70% DLT698, 30% DLT645
        # 均匀分配到 32 个通道 (0..31)
        channel = (i - 1) % 32
        
        # 规约按通道分配，严禁混用 (0-21: 698, 22-31: 645)
        protocol = "DLT698" if channel < 22 else "DLT645"
        
        # 波特率统一 9600 (工业标准)
        baud = 9600
        
        # 工业元数据随机生成
        wiring_mode = random.choice([1, 3]) # 单相或三相
        collector_addr = f"{20000000 + channel}"
        asset_no = f"ASN-{i:05d}"
        pt_ratio = random.choice([1, 10, 100])
        ct_ratio = random.choice([1, 20, 50, 200])
        
        meters.append((address, protocol, baud, channel, wiring_mode, collector_addr, asset_no, pt_ratio, ct_ratio))

    # 3. 事务性入库
    cursor.executemany("INSERT INTO meter_inventory VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)", meters)
    conn.commit()
    
    print(f"📊 [PROVISION] 成功注入 2000 只电能表数字孪生档案！")
    print(f"   - 通道数量: 32 (每通道约 {2000//32} 只)")
    print(f"   - 规约分布: DLT698 (70%), DLT645 (30%)")
    print(f"   - 数据库路径: {DB_PATH}")
    
    conn.close()

if __name__ == "__main__":
    provision_grid()
