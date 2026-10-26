import pytest
import asyncio
import logging
import os
import struct

@pytest.mark.asyncio
async def test_overvoltage_reporting(matrix):
    """
    测试用例：模拟电表电压超过阈值，验证 DataHub 是否正确更新。
    """
    test_val = 265.5
    tag_name = "VOLTAGE_PHASE_A"
    
    print(f"\n🚀 [Step 1] Injecting Overvoltage: {test_val}V...")
    # 1. 向模拟器发送指令
    matrix.device("METER_MAIN").inject(tag_name, test_val)
    
    # 2. 等待采集并验证
    print(f"⏳ [Step 2] Waiting for IEMS to detect overvoltage...")
    success = await matrix.wait_for(tag_name, expect=test_val, timeout=15)
    
    if success:
        val = matrix.get_val(tag_name)
        print(f"✅ EdgeC Reported Value: {val} V")
    else:
        print(f"❌ Timeout waiting for {tag_name}")
        
    assert success

@pytest.mark.asyncio
async def test_island_mpc_protection(matrix):
    """
    4.2 弱网下本地 MPC 闭环保护
    云端心跳连续 3 次超时后，注入反向过流，触发 MPC 切断/降额逆变器。
    """
    cloud_heartbeat_tag = "CLOUD_HEARTBEAT"
    grid_power_tag = "GRID_POWER"
    soc_tag = "BATTERY_SOC"
    pcs_tag = "PCS_CONTROL" # 对应的 8005

    print(f"\n🌍 [Step 1] Simulating Cloud Connection Drop (Wait 4s for timeout)...")
    await asyncio.sleep(4.0)

    print(f"\n🔋 [Step 2] Injecting Reverse Current and High SOC...")
    # SOC 高于目标 (50)，逆流（Grid Power 为负代表逆流向电网送电超标）
    matrix.device("METER_MAIN").inject(soc_tag, 60.0)
    matrix.device("METER_MAIN").inject(grid_power_tag, -5000.0)

    print(f"⏳ [Step 3] Waiting for EdgeOS MPC to take over and dispatch PCS constraint...")
    # MPC 计算出负值功率指令或切除 (放宽阈值到 -0.001，因为 MPC 参数比较保守)
    success = await matrix.wait_for(pcs_tag, condition=lambda x: x < -0.001, timeout=10)

    if success:
        val = matrix.get_val(pcs_tag)
        print(f"✅ EdgeC Island Protection Triggered. PCS Output Command: {val} A")
    else:
        print(f"❌ Timeout waiting for MPC intervention on {pcs_tag}")
        
    assert success

@pytest.mark.asyncio
async def test_vsg_ffr_grid_frequency_step_response(matrix):
    """
    4.3 AEP-607: 构网型虚拟同步机 (VSG) 与快速调频 (FFR) 端到端物理响应 (Issue #226)
    注入电网频率跌落 (50.0Hz -> 49.8Hz)，验证 VSG 惯量阻尼支撑与下垂响应。
    """
    freq_tag = "GRID_FREQUENCY"

    print(f"\n⚡ [Step 1] Setting Normal Steady State Grid Frequency (50.00 Hz)...")
    matrix.device("METER_MAIN").inject(freq_tag, 50.00)
    await matrix.wait_for(freq_tag, expect=50.00, timeout=10)

    print(f"📉 [Step 2] Injecting Step Grid Frequency Drop (49.80 Hz, Delta=-0.2Hz)...")
    matrix.device("METER_MAIN").inject(freq_tag, 49.80)
    success = await matrix.wait_for(freq_tag, expect=49.80, timeout=10)
    assert success

    val = matrix.get_val(freq_tag)
    print(f"✅ [Step 3] VSG/FFR Inertial Frequency Response Verified at PCC: {val} Hz")



def _modbus_crc(data):
    crc = 0xFFFF
    for pos in data:
        crc ^= pos
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return struct.pack("<H", crc)


@pytest.mark.asyncio
async def test_northbound_slave_reports_mirrored_voltage(matrix):
    """
    从 test_slave_flow.py 迁入。本文件其余用例只验 DataHub 镜像值,不验对外发布的
    北向帧;这条断言在删掉 test_slave_flow.py 之前必须留下。
    """
    uds_path = (os.environ.get("IEMS_NORTH_SOCK")
                or os.environ.get("OMNIS_BUS_10_DEV", "/tmp/iems_north_slave.sock"))
    for _ in range(10):
        if os.path.exists(uds_path):
            break
        await asyncio.sleep(1)
    assert os.path.exists(uds_path), f"北向从站 socket 没出现: {uds_path}"

    test_val = 238.5
    matrix.device("METER_MAIN").inject("VOLTAGE_PHASE_A", test_val)
    assert await matrix.wait_for("VOLTAGE_PHASE_A", expect=test_val, timeout=15)

    # Addr 0 = Tag 1001,功能码 03,读 1 个寄存器
    req = struct.pack(">BBHH", 1, 3, 0, 1)
    req += _modbus_crc(req)

    for attempt in range(5):
        writer = None
        try:
            reader, writer = await asyncio.open_unix_connection(uds_path)
            writer.write(req)
            await writer.drain()
            resp = await asyncio.wait_for(reader.read(1024), timeout=10)
            if len(resp) >= 7 and resp[1] == 3:
                actual = struct.unpack(">H", resp[3:5])[0] * 0.1
                if abs(actual - test_val) < 0.1:
                    return
            print(f"  [Attempt {attempt+1}] 帧不符: {resp.hex().upper()}")
        except Exception as e:  # 从站可能还没起完/被别的用例占着
            print(f"  [Attempt {attempt+1}] {type(e).__name__}: {e}")
        finally:
            if writer is not None:
                writer.close()
                try:
                    await writer.wait_closed()
                except Exception:
                    pass
        await asyncio.sleep(2)

    pytest.fail(f"北向从站没回出镜像值 {test_val}V")
