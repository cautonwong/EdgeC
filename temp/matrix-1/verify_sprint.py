import asyncio
import os
import yaml
import socket
from tools.matrix.engine.orchestrator import MatrixOrchestrator

async def verify_massive_and_interactive():
    # 1. 伪造一个 extension.yaml
    ext_path = "/tmp/matrix_world.yaml"
    world_config = {
        "world": {
            "topology": [
                {
                    "bus_id": 0,
                    "transport": {"type": "uds", "path": "/tmp/matrix_massive.sock"},
                    "devices": [
                        {"protocol": "Modbus", "count": 100, "start_address": 1}
                    ]
                }
            ]
        }
    }
    with open(ext_path, "w") as f:
        yaml.dump(world_config, f)

    schema_path = "build/products/iems/generated/shadow_schema.json"
    if not os.path.exists(schema_path):
        print("Skipping: iems build product missing.")
        return

    # 2. 启动编排器
    orch = MatrixOrchestrator(schema_path, log_dir="build/matrix_massive_logs")
    # 强制重新加载 extension
    from tools.matrix.engine.loader import SchemaLoader
    orch.loader = SchemaLoader(schema_path, ext_path)
    
    orch.build_from_world()
    await orch.start_interactive_api(port=9999)
    
    # 启动后台任务
    matrix_task = asyncio.create_task(orch.run())
    await asyncio.sleep(1)

    print(f"✅ Massive Topology built: {len(orch.devices)} devices.")
    assert len(orch.devices) == 100
    assert "Bus0_Modbus_1" in orch.devices
    assert "Bus0_Modbus_100" in orch.devices

    # 3. 验证交互式 API
    try:
        reader, writer = await asyncio.open_connection('127.0.0.1', 9999)
        writer.write(b"SET Bus0_Modbus_1 VOLTAGE_PHASE_A 245.5\n")
        await writer.drain()
        
        resp = await reader.read(1024)
        print(f"Interactive API Response: {resp.decode().strip()}")
        assert b"OK" in resp
        
        # 验证值是否生效
        dev = orch.get_device("Bus0_Modbus_1")
        val = dev.datastore.get(1001) # VOLTAGE_PHASE_A = 1001
        print(f"Verified Value in DataStore: {val}")
        assert val == 245.5
        
        writer.close()
        await writer.wait_closed()
    except Exception as e:
        print(f"❌ Interactive API Test Failed: {e}")

    matrix_task.cancel()
    print("🎉 Sprint Matrix Evolution Verification PASSED!")

if __name__ == "__main__":
    asyncio.run(verify_massive_and_interactive())
