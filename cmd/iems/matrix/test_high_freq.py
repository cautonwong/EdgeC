import pytest
import asyncio
import time

@pytest.mark.asyncio
async def test_adc_to_protection_e2e(matrix):
    """
    [Sprint 5 E2E] 高频全链路验证：
    ADC 波形注入 -> RMS 计算 -> 触发过压保护 -> 继电器动作
    """
    try:
        # 1. 初始状态检查：电压应在正常范围 (220V RMS)
        print("\n🚀 [Step 1] Checking initial steady state...")
        # [Fix] 显式重置 ADC 到标准状态，防止被上一用例干扰
        matrix.adc_control("grid", {"freq": 50.0, "amp": 311.0})
        matrix.adc_control("h3", {"ratio": 0.0})
        
        # 给系统一点稳定时间
        await asyncio.sleep(2)
        
        val = matrix.get_val("ADC_VOLTAGE_RMS")
        print(f"  Steady State RMS: {val if val is not None else 0:.2f} V")
        assert val is not None and 215.0 < val < 225.0
        
        # 2. 模拟电网异常：通过 ADC 注入过压波形 (有效值从 220V 升至 280V)
        # 计算峰值: 280 * 1.414 = 395.9
        print("\n🚀 [Step 2] Injecting Waveform Surge via ADC Control...")
        matrix.adc_control("grid", {"amp": 395.9})
        
        # 3. 验证 C 端 RMS 计算结果
        print("⏳ Waiting for IEMS to compute new RMS...")
        start_wait = time.time()
        success = False
        while time.time() - start_wait < 30:
            val = matrix.get_val("ADC_VOLTAGE_RMS")
            print(f"  [Debug] Current ADC_VOLTAGE_RMS: {val if val is not None else 0:.2f} V")
            if val is not None and abs(val - 280.0) <= 5.0:
                success = True
                break
            await asyncio.sleep(1)

        if success:
            new_val = matrix.get_val("ADC_VOLTAGE_RMS")
            print(f"✅ IEMS Detected Surge: {new_val:.2f} V")
        else:
            print("❌ IEMS failed to detect voltage surge in time")
            assert False

        # 4. 验证保护管线动作：SOLAR_RELAY 应变为 0 (Open)
        # 过压阈值在 manifest.yaml 中定义为 264.0V
        print("\n🚀 [Step 3] Verifying Reflex Protection Action...")
        relay_success = await matrix.wait_for("SOLAR_RELAY", expect=0.0, timeout=5)
        
        if relay_success:
            print("🔥 [FINAL_SUCCESS] Protection triggered! SOLAR_RELAY is now OPEN.")
        else:
            print("❌ Protection failed to trigger or relay did not respond")
            
        assert relay_success
    finally:
        # 恢复物理环境与继电器状态
        matrix.adc_control("grid", {"freq": 50.0, "amp": 311.0})
        matrix.adc_control("h3", {"ratio": 0.0})

@pytest.mark.asyncio
async def test_harmonic_interference_rms(matrix):
    """
    验证谐波干扰下的 RMS 计算准确性
    """
    # 重置环境
    matrix.adc_control("grid", {"freq": 50.0, "amp": 311.0})
    await asyncio.sleep(1)

    print("\n🚀 [Step 1] Injecting 10% 3rd Harmonic...")
    # amp=311 (220V RMS), h3_ratio=0.10
    matrix.adc_control("h3", {"ratio": 0.10})
    
    # 理论 RMS = sqrt(V1^2 + V3^2) = sqrt(220^2 + (220*0.1)^2) = 221.1V
    expect_rms = 221.1
    
    print(f"⏳ Waiting for RMS calibration (Expected: {expect_rms}V)...")
    success = await matrix.wait_for("ADC_VOLTAGE_RMS", expect=expect_rms, tolerance=2.0, timeout=10)
    
    if success:
        val = matrix.get_val("ADC_VOLTAGE_RMS")
        print(f"✅ Harmonic RMS Verified: {val:.2f} V")
    
    assert success