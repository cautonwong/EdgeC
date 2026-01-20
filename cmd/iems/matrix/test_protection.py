import pytest
import requests
import time

MATRIX_URL = "http://localhost:50051/api/v1"
EDGEC_URL = "http://localhost:9091/api/v1"

def test_overvoltage_reporting():
    """
    测试用例：模拟电表电压超过阈值，验证 DataHub 是否正确更新。
    """
    # 1. 向模拟器发送指令：设置电压为 265.5V (严重过压)
    resp = requests.post(f"{MATRIX_URL}/actor/set", json={
        "dev_id": "slave_1", # 对应 modbus.py 中的 Slave ID
        "tag": "voltage",
        "value": 265.5
    })
    assert resp.status_code == 200
    
    # 2. 等待采集周期 (Modbus 轮询通常是 1s)
    time.sleep(2)
    
    # 3. 从 EdgeC 的 REST API 读取结果
    # 假设 Tag 1001 对应 IEMS 的主采集电压
    resp = requests.get(f"{EDGEC_URL}/data/latest/1001")
    assert resp.status_code == 200
    data = resp.json()
    
    print(f"✅ EdgeC Reported Value: {data['value']} V")
    assert 265.0 <= data['value'] <= 266.0
