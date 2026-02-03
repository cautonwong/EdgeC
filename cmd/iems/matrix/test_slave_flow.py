import pytest
import asyncio
import socket
import struct

def calculate_crc(data):
    crc = 0xFFFF
    for pos in data:
        crc ^= pos
        for i in range(8):
            if (crc & 1) != 0:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return struct.pack("<H", crc)

@pytest.mark.asyncio
async def test_slave_mirror_flow(matrix):
    """
    [Sprint 5] 真正的北向从站全链路验证：
    1. 注入南向数据 -> 2. IEMS 采集并快照 -> 3. 外部 Master 通过 Modbus 请求北向口 -> 4. 验证响应字节
    """
    print("\n⏳ [Setup] Waiting for IEMS Northbound to initialize...")
    # [Nexus] 使用唯一的、非冲突的路径
    uds_path = "/tmp/iems_north_slave.sock"
    
    # 等待文件出现
    import os
    for _ in range(10):
        if os.path.exists(uds_path): break
        await asyncio.sleep(1)
    
    assert os.path.exists(uds_path), "IEMS failed to create Northbound socket"

    test_val = 238.5
    tag_name = "VOLTAGE_PHASE_A"
    
    # 1. 向南向电表注入数据
    print(f"🚀 [Step 1] Injecting {test_val}V to SOUTHBOUND meter...")
    matrix.device("METER_MAIN").inject(tag_name, test_val)
    
    # 2. 确保 IEMS 已采集到 DataHub
    print("⏳ [Step 2] Waiting for IEMS Master to sync DataHub...")
    success = await matrix.wait_for(tag_name, expect=test_val, timeout=15)
    assert success

    # 3. 模拟外部 Master 轮询北向口
    print("🚀 [Step 3] Acting as External Master: Querying Northbound Port...")
    
    # 请求 Addr 0 (Tag 1001)
    req = struct.pack(">BBHH", 1, 3, 0, 1)
    req += calculate_crc(req)
    print(f"🚀 [Step 3] Acting as External Master: Querying Northbound Port (Sent: {req.hex().upper()})...")
    
    for attempt in range(5):
        try:
            await asyncio.sleep(1.0) # 增加等待时间，确保 IEMS 已经进入 uv_run
            reader, writer = await asyncio.open_unix_connection(uds_path)
            writer.write(req)
            await writer.drain()
            
            resp = await asyncio.wait_for(reader.read(1024), timeout=10) # 增加响应超时到 10 秒
            print(f"  [Attempt {attempt+1}] Received Raw: {resp.hex().upper()}")
            writer.close()
            await writer.wait_closed()
            
            if len(resp) >= 7 and resp[1] == 3:
                raw_val = struct.unpack(">H", resp[3:5])[0]
                actual_val = raw_val * 0.1
                print(f"  [Attempt {attempt+1}] Received: {resp.hex().upper()} -> {actual_val}V")
                if abs(actual_val - test_val) < 0.1:
                    print("🔥 [FINAL_SUCCESS] Northbound Slave E2E Flow Verified!")
                    return
            else:
                print(f"  [Attempt {attempt+1}] Response mismatch or short: {resp.hex()}")
        except Exception as e:
            import traceback
            print(f"  [Attempt {attempt+1}] Connection failed: {type(e).__name__}: {e}")
            # traceback.print_exc()
        
        await asyncio.sleep(2)
        
    pytest.fail("Failed to get correct Slave response after retries")