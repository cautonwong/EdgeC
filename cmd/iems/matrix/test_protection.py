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

