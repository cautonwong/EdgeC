import pytest
import os
import logging

logger = logging.getLogger("Matrix.Runner")

@pytest.mark.asyncio
async def test_scenarios(matrix, scenario_context):
    """
    通用测试入口：执行由 conftest.py 发现并生成的场景。
    """
    scen_path = scenario_context["scenario_path"]
    logger.info(f"🚀 Running Scenario: {os.path.basename(scen_path)}")
    
    # 执行剧本
    await matrix.run_scenario(scen_path)
    
    # 如果剧本中没有显式的 check，我们可以增加一个默认的健康检查
    # await matrix.wait_for("SYSTEM_STATUS", 1, timeout=5)
