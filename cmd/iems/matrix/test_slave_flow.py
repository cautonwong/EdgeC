import pytest
import asyncio
import requests
import time

@pytest.mark.asyncio
async def test_slave_mirror_flow(matrix):
    """
    [Nexus E2E] 验证南向采集 -> 影子映射 -> 北向从站响应的全链路
    """
    # [Nexus Fix] 等待 IEMS 连接到北向 VBus 10。
    # 考虑到 _try_connect_buses 每 1s 运行一次，加上握手时间，我们等 3s。
    print("⏳ [Setup] Waiting for IEMS to establish Northbound connection...")
    await asyncio.sleep(3)

    test_val = 238.5
    tag_name = "VOLTAGE_PHASE_A"
    tag_id = 1001
    
    # 1. 向南向电表注入数据
    print(f"\n🚀 [Step 1] Injecting {test_val}V to SOUTHBOUND meter...")
    matrix.device("METER_MAIN").inject(tag_name, test_val)
    
    # 2. 验证 IEMS 核心已采集
    print("⏳ [Step 2] Waiting for IEMS Master to poll...")
    # 注意：高频 ADC 正在写 ADC_VOLTAGE_RMS，我们这里测的是慢速 Modbus 通路
    success = await matrix.wait_for(tag_name, expect=test_val, timeout=10)
    assert success
    
    # 3. 验证北向“上位机”观测到了相同的值
    print("⏳ [Step 3] Verifying NORTHBOUND Supervisor observation...")
    
    # 给 Supervisor 一点轮询时间
    found = False
    start_t = time.time()
    while time.time() - start_t < 30:
        # 通过 Matrix Server 的 Observer API 获取上位机拿到的值
        resp = requests.get(f"{matrix.server_url}/observer/latest/SUPERVISOR_SCADA/{tag_id}")
        if resp.status_code == 200:
            obs_val = resp.json().get("value")
            if obs_val is not None:
                print(f"  [Observer] Supervisor saw: {obs_val} V")
                if abs(obs_val - test_val) < 0.5:
                    found = True
                    break
            else:
                # [Nexus Debug] 没拿到具体值时，拉取所有观测数据看看 Key 对不对
                all_data = requests.get(f"{matrix.server_url}/observer/all").json()
                print(f"  [Debug] Current Observer Keys: {all_data.get('keys', [])}")
        await asyncio.sleep(1)
        
    if found:
        print("🔥 [FINAL_SUCCESS] Slave Mirror Flow Verified!")
    else:
        print("❌ Supervisor failed to see the mirrored value")
        
    assert found
