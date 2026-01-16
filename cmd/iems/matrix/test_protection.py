import pytest
import os
import asyncio

@pytest.mark.asyncio
async def test_slave_id_addressing_sequential(matrix):
    """
    分时验证多站号寻址：
    1. 注入站号 1 -> 验证。
    2. 注入站号 2 -> 验证。
    """
    # 步骤 1: 验证站号 1
    matrix.device("Bus0_ModbusRTU_1").inject("VOLTAGE_PHASE_A", 220.0, matrix.orch.loader)
    print("\n📡 [Test] Injected 220.0V into Device 1")
    assert await matrix.wait_for("VOLTAGE_PHASE_A", 220.0, timeout=10)
    print("✅ Device 1 addressed successfully.")

    # 步骤 2: 验证站号 2
    matrix.device("Bus0_ModbusRTU_2").inject("VOLTAGE_PHASE_A", 285.5, matrix.orch.loader)
    print("📡 [Test] Injected 285.5V into Device 2")
    assert await matrix.wait_for("VOLTAGE_PHASE_A", 285.5, timeout=10)
    print("✅ Device 2 addressed successfully.")
    
    print("\n🎉 Multi-SlaveID addressing 100% verified!")