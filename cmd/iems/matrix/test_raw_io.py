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
    # 给 UDS Relay 足够的时间在事件循环中 accept 此连接
    await asyncio.sleep(0.1)

    stop_event = asyncio.Event()

    async def mock_device_echo():
        nonlocal reader, writer
        try:
            print(f"📡 [Mock Device] Waiting for request...")
            while not stop_event.is_set():
                try:
                    data = await asyncio.wait_for(reader.read(1024), timeout=0.5)
                except asyncio.TimeoutError:
                    continue
                if not data:
                    print(f"⚠️ [Mock Device] Connection EOF detected, reconnecting to {vbus_path}...")
                    try:
                        writer.close()
                        await writer.wait_closed()
                    except Exception:
                        pass
                    await asyncio.sleep(0.2)
                    try:
                        reader, writer = await asyncio.open_unix_connection(vbus_path)
                    except Exception as re_err:
                        print(f"⚠️ [Mock Device] Reconnect error: {re_err}")
                    continue
                
                hex_recv = data.hex().upper()
                if not hex_recv.startswith("0103"):
                    print(f"📥 [Mock Device] Received Raw: {hex_recv}")
                
                if tx_payload_hex in hex_recv:
                    print(f"📤 [Mock Device] MATCH! Sending Echo: {rx_echo_hex}")
                    writer.write(bytes.fromhex(rx_echo_hex))
                    await writer.drain()
        except asyncio.CancelledError:
            pass
        except Exception as e:
            print(f"❌ [Mock Device] Error: {e}")
        finally:
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass

    # 启动后台模拟设备任务
    echo_task = asyncio.create_task(mock_device_echo())

    print(f"🚀 [Step 2 & 4] Submitting Raw IO to IEMS and Verifying...")
    api_url = f"http://localhost:{matrix.edgec.rest_port}/api/v1/io/raw"
    latest_url = f"http://localhost:{matrix.edgec.rest_port}/api/v1/io/raw/latest"
    
    loop = asyncio.get_event_loop()
    success = False
    
    # 等待总线与 EdgeC 完全就绪
    await asyncio.sleep(1.5)

    for attempt in range(15):
        print(f"  [Attempt {attempt+1}] Sending DEADBEEF...")
        resp = await loop.run_in_executor(None, lambda: requests.post(api_url, json={
            "bus_id": bus_id,
            "payload": tx_payload_hex
        }))
        assert resp.status_code == 200
        
        # 轮询 latest 结果，最多等 3 秒 (容纳总线仲裁排队时间)
        for _ in range(30):
            await asyncio.sleep(0.1)
            resp = await loop.run_in_executor(None, lambda: requests.get(latest_url))
            data = resp.json()
            if data.get("payload") == rx_echo_hex:
                print(f"✅ [SUCCESS] IEMS captured the echo: {rx_echo_hex}")
                success = True
                break
        
        if success:
            break
        else:
            print(f"  [Attempt {attempt+1}] Got payload: {data.get('payload')}")
        
    # 等待后台任务平滑结束
    stop_event.set()
    echo_task.cancel()
    try:
        await echo_task
    except asyncio.CancelledError:
        pass
    
    assert success, f"IEMS failed to capture echo after 15 attempts."
