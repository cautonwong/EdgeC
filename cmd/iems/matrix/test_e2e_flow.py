import pytest
import asyncio
import logging

logger = logging.getLogger("Matrix.Test")

@pytest.mark.asyncio
async def test_iems_modbus_e2e_flow(matrix):
    """
    正式集成测试：验证从仿真器注入到 C 端 DataHub 的全链路数据流
    """
    test_value = 50.02
    tag_name = "GRID_FREQUENCY"

    print(f"\n🚀 [Step 1] Injecting {test_value}Hz into virtual device...")
    # 通过 Matrix 仿真器注入物理信号
    matrix.device("METER_MAIN").inject(tag_name, test_value, matrix.loader)

    print(f"⏳ [Step 2] Waiting for IEMS to poll and update DataHub...")
    # wait_for 内部会自动调用 REST API 轮询 C 程序的实时数据
    success = await matrix.wait_for(tag_name, expect=test_value, timeout=10)
    
    if success:
        print(f"🎉 [Step 3] Verification SUCCESS: IEMS DataHub now holds {test_value}")
    else:
        # 如果失败，尝试读取当前值以便调试
        current_val = matrix.get_val(tag_name)
        print(f"❌ [Step 3] Verification FAILED: Expected {test_value}, but DataHub has {current_val}")
    
    assert success

@pytest.mark.asyncio
async def test_iems_multi_tag_consistency(matrix):
    """
    验证多点位同时采集的一致性
    """
    data_map = {
        "VOLTAGE_PHASE_A": 220.5,
        "GRID_POWER": 1500.0,
        "GRID_FREQUENCY": 50.02
    }
    
    print("\n🚀 [Step 1] Injecting multiple values...")
    dev = matrix.device("METER_MAIN")
    for name, val in data_map.items():
        dev.inject(name, val, matrix.orch.loader)
        
    print("⏳ [Step 2] Verifying all tags in IEMS...")
    for name, val in data_map.items():
        success = await matrix.wait_for(name, expect=val, timeout=10)
        if not success:
            actual = matrix.get_val(name)
            print(f"❌ Tag {name} failed: Expected {val}, Got {actual}")
            assert False
        else:
            print(f"✅ Tag {name} verified: {val}")
        print(f"✅ Tag {name} verified: {val}")
