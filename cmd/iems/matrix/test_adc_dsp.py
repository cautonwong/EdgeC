import pytest
import asyncio
import numpy as np
import socket
import array
import os

@pytest.mark.asyncio
async def test_industrial_proof_dsp(matrix):
    """
    [重构版] 物理算法验证测试
    利用 matrix fixture 自动拉起的 EdgeC 和 UDS 链路进行验证
    """
    # 这里的 matrix 会自动处理连接和 FD 传递
    # 我们只需要观察 DataHub 里的结果
    
    test_val = 231.5
    tag_id = 1001 # Voltage_RMS
    
    print(f"\n🚀 [Step 1] Verifying SDD results for {test_val}V...")
    
    # 由于 manual_injector 已经在后台跑了，我们只需要等待结果
    # 注意：我们的 SDD 集成测试会自动注入数据，此处我们只需查询
    
    success = await matrix.wait_for("VOLTAGE_PHASE_A", expect=test_val, timeout=15)
    
    if success:
        val = matrix.get_val("VOLTAGE_PHASE_A")
        print(f"🔥 [FINAL_SUCCESS] Full Chain Verified via Hub: {val}")
    else:
        print("❌ [FAILED] Algorithm output mismatch or timeout")
        
    assert success