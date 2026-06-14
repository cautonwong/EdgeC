import pytest
import asyncio
import requests
import socket
import os

@pytest.mark.asyncio
async def test_raw_io_loopback(matrix):
    """
    [Sprint 6] Raw I/O Passthrough Verification:
    1. Send hex 'DEADBEEF' via REST -> 2. Matrix captures and echoes 'CAFEBABE' -> 3. Verify via REST
    """
    bus_id = 0
    tx_payload_hex = "DEADBEEF"
    rx_echo_hex = "CAFEBABE"
    
    # 注入 VBus 路径 (从环境变量获取，与 Matrix Server 对齐)
    vbus_path = os.environ.get(f"OMNIS_BUS_{bus_id}_DEV", f"/tmp/vbus{bus_id}.sock")
    assert os.path.exists(vbus_path), f"Virtual bus socket {vbus_path} not found"

    print(f"\n🚀 [Step 1] Connecting to Virtual Bus {bus_id} at {vbus_path}...")
    
    # 使用 asyncio 打开 Unix 域套接字连接
    reader, writer = await asyncio.open_unix_connection(vbus_path)

    async def mock_device_echo():
        try:
            print(f"📡 [Mock Device] Waiting for request...")
            while True:
                # 持续读取直到发现 DEADBEEF
                data = await asyncio.wait_for(reader.read(1024), timeout=10.0)
                if not data: break
                
                hex_recv = data.hex().upper()
                if not hex_recv.startswith("0103"):
                    print(f"📥 [Mock Device] Received Raw: {hex_recv}")
                
                if tx_payload_hex in hex_recv:
                    print(f"📤 [Mock Device] MATCH! Sending Echo: {rx_echo_hex}")
                    writer.write(bytes.fromhex(rx_echo_hex))
                    await writer.drain()
                    break
        except Exception as e:
            print(f"❌ [Mock Device] Error: {e}")
        finally:
            writer.close()
            await writer.wait_closed()

    # 启动后台模拟设备任务
    echo_task = asyncio.create_task(mock_device_echo())

    print(f"🚀 [Step 2 & 4] Submitting Raw IO to IEMS and Verifying...")
    api_url = f"http://localhost:{matrix.edgec.rest_port}/api/v1/io/raw"
    latest_url = f"http://localhost:{matrix.edgec.rest_port}/api/v1/io/raw/latest"
    
    loop = asyncio.get_event_loop()
    success = False
    
    # 工业级柔性重试：为了抵御之前 Modbus 超时堆积的“垃圾报文”，我们允许重发
    for attempt in range(5):
        print(f"  [Attempt {attempt+1}] Sending DEADBEEF...")
        resp = await loop.run_in_executor(None, lambda: requests.post(api_url, json={
            "bus_id": bus_id,
            "payload": tx_payload_hex
        }))
        assert resp.status_code == 200
        
        await asyncio.sleep(1.0) # 等待 EdgeC 抓取并回填
        
        resp = await loop.run_in_executor(None, lambda: requests.get(latest_url))
        data = resp.json()
        if data.get("payload") == rx_echo_hex:
            print(f"✅ [SUCCESS] IEMS captured the echo: {rx_echo_hex}")
            success = True
            break
        else:
            print(f"  [Attempt {attempt+1}] Got garbage: {data.get('payload')}")
        
    # 等待后台任务平滑结束
    echo_task.cancel()
    
    assert success, f"IEMS failed to capture echo after 5 attempts."
