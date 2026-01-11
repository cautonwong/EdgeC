import requests
import sqlite3
import time
import os

class OmnisObserver:
    def __init__(self, rest_port, db_path):
        self.rest_port = rest_port
        self.db_path = db_path
        self.base_url = f"http://localhost:{rest_port}/api/v1"

    def get_stats(self):
        try:
            resp = requests.get(f"{self.base_url}/system/stats", timeout=2)
            if resp.status_code == 200:
                return resp.json()
        except: pass
        return None

    def get_tag_value(self, tag_id):
        url = f"{self.base_url}/data/latest/{tag_id}"
        # 内部重试机制：应对网络库与进程启动的时序差
        for _ in range(5):
            try:
                resp = requests.get(url, timeout=2)
                if resp.status_code == 200:
                    data = resp.json()
                    if data.get("status") != "not_found":
                        return data.get("value")
                    else:
                        # 数据还未采集到，继续重试
                        time.sleep(0.5)
                        continue
                elif resp.status_code == 404:
                    return None
            except requests.exceptions.ConnectionError:
                time.sleep(0.5)
            except Exception as e:
                print(f"DEBUG: API error: {e}")
        return None

    def check_db_value(self, tag_id, min_val=None, max_val=None, count=1):
        if not os.path.exists(self.db_path): return False
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            query = "SELECT value FROM telemetry WHERE tag_id = ? ORDER BY ts DESC LIMIT ?"
            cursor.execute(query, (tag_id, count))
            rows = cursor.fetchall()
            conn.close()
            if not rows or len(rows) < count: return False
            for row in rows:
                val = row[0]
                if min_val is not None and val < min_val: return False
                if max_val is not None and val > max_val: return False
            return True
        except: return False

    def wait_for_condition(self, check_func, timeout=10, interval=0.5):
        start = time.time()
        while time.time() - start < timeout:
            try:
                if check_func(): return True
            except: pass
            time.sleep(interval)
        return False
