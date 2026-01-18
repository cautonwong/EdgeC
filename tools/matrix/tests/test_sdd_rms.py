import pytest
import asyncio
import os
from tools.matrix.simulators.manual_injector import ADCSimulationRunner

@pytest.mark.asyncio
async def test_rms_algorithm_accuracy(matrix):
    """
    SDD 集成测试：
    1. 启动 IEMS (由 matrix fixture 自动完成)
    2. 注入包含 30% 骤降和谐波干扰的高频信号
    3. 实时比对 C 算法输出与 Python 金标准
    4. 断言精度误差 < 0.5%
    """
    # 这里的 matrix fixture 会自动启动 EdgeC 进程并分配 9091 端口
    # 我们使用其分配的 rest_port
    edge_url = f"http://127.0.0.1:{matrix.edgec.rest_port}"
    
    runner = ADCSimulationRunner(edge_url=edge_url)
    
    # 运行一个 3 秒的紧凑场景 (包含 1 秒的骤降工况)
    from tools.matrix.simulators.manual_injector import logger
    logger.info("🎬 Starting SDD Accuracy Test Scenario...")
    
    await runner.run_scenario(duration_s=3)
    
    # 获取统计结果
    history = runner.verifier.history
    assert len(history) > 0, "No data collected from EdgeC"
    
    passed_count = sum(1 for r in history if r['passed'])
    pass_rate = passed_count / len(history)
    
    # 断言 100% 的帧都必须通过精度校验
    assert pass_rate == 1.0, f"Accuracy test failed! Pass rate: {pass_rate:.2%}"
    logger.info(f"✨ SDD Test Passed. Verified {len(history)} frames with 100% precision.")
