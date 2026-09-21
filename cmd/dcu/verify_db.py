import sqlite3
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.environ.get("EDGEC_DATA_DIR", os.path.join(SCRIPT_DIR, "../../build/products/dcu/bin/dcu_runtime.db"))
if not os.path.isabs(DB_PATH):
    DB_PATH = os.path.abspath(DB_PATH)

def verify_evidence():
    if not os.path.exists(DB_PATH):
        print(f"❌ [CRITICAL] 数据库文件不存在: {DB_PATH}")
        return

    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    
    print("--- 📂 EdgeC 6xxx 业务入库证据核查 ---")
    
    # 1. 核查档案加载 (6000/6001)
    cursor.execute("SELECT count(*) FROM meter_inventory;")
    meter_count = cursor.fetchone()[0]
    print(f"✅ [证据1] 档案表 (meter_inventory): 已存入 {meter_count} 只表的数字孪生档案。")

    # 2. 核查历史快照入库 (5xxx/6xxx 关联)
    # 我们之前的 stage_sqlite_sink 默认存入的表名可能是 telemetry_history 或 dcu_telemetry_latest
    # 检查所有存在的表
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
    tables = [t[0] for t in cursor.fetchall()]
    print(f"📡 [系统表列表]: {tables}")

    for table in ["telemetry", "dcu_telemetry_latest", "event_history"]:
        if table in tables:
            cursor.execute(f"SELECT count(*) FROM {table};")
            row_count = cursor.fetchone()[0]
            print(f"✅ [证据2] 数据表 ({table}): 已成功入库 {row_count} 条采集明细。")
            
            if row_count > 0:
                print(f"   >>> 最新 3 条数据抽样:")
                try:
                    cursor.execute(f"SELECT * FROM {table} LIMIT 3;")
                    for row in cursor.fetchall():
                        print(f"       {row}")
                except Exception as e:
                    print(f"       无法读取数据明细: {e}")

    conn.close()

if __name__ == "__main__":
    verify_evidence()
