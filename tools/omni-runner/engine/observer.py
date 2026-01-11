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
        try:
            resp = requests.get(url, timeout=2)
            if resp.status_code == 200:
                data = resp.json()
                # 只有当 API 明确返回成功且有值时才返回数值
                if "value" in data:
                    return data["value"]
        except:
            pass
        return None # 数据未就绪、404 或连接失败一律返回 None

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

    def wait_for_condition(self, check_func, timeout=30, interval=0.5):
        start = time.time()
        while time.time() - start < timeout:
            try:
                res = check_func()
                if res: return True
            except: pass
            time.sleep(interval)
        return False