import pytest
import asyncio
import logging

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
    # MPC 计算出负值功率指令或切除
    success = await matrix.wait_for(pcs_tag, condition=lambda x: x < -0.1, timeout=10)

    if success:
        val = matrix.get_val(pcs_tag)
        print(f"✅ EdgeC Island Protection Triggered. PCS Output Command: {val} A")
    else:
        print(f"❌ Timeout waiting for MPC intervention on {pcs_tag}")
        
    assert success
