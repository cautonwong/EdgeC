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

    # 2. Submit Raw IO request via IEMS REST API
    print(f"🚀 [Step 2] Submitting Raw IO to IEMS: {tx_payload_hex}")
    api_url = f"http://localhost:{matrix.edgec.rest_port}/api/v1/io/raw"
    
    # 注意：requests 是同步的，在 asyncio 中建议使用 httpx 或跑在 run_in_executor
    # 但由于这是测试，且量小，直接用 requests 简单处理
    loop = asyncio.get_event_loop()
    resp = await loop.run_in_executor(None, lambda: requests.post(api_url, json={
        "bus_id": bus_id,
        "payload": tx_payload_hex
    }))
    assert resp.status_code == 200
    
    # 3. Wait for the echo task to complete
    await echo_task
    
    # 4. Verify received data via IEMS REST API
    print("🚀 [Step 4] Verifying Result via IEMS REST...")
    latest_url = f"http://localhost:{matrix.edgec.rest_port}/api/v1/io/raw/latest"
    
    success = False
    for _ in range(10):
        resp = await loop.run_in_executor(None, lambda: requests.get(latest_url))
        data = resp.json()
        if data.get("payload") == rx_echo_hex:
            print(f"✅ [SUCCESS] IEMS captured the echo: {rx_echo_hex}")
            success = True
            break
        await asyncio.sleep(0.5)
        
    assert success, f"IEMS failed to capture echo. Last state: {resp.text}"
