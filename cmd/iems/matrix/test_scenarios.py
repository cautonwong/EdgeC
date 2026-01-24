import pytest
import os
import glob

# 动态发现该产品下的所有场景文件
product_dir = os.path.dirname(os.path.abspath(__file__))
scenario_files = glob.glob(os.path.join(product_dir, "scenarios/*.yaml"))

@pytest.mark.parametrize("scen_path", scenario_files, ids=lambda p: os.path.basename(p))
@pytest.mark.asyncio
async def test_run_manifest_scenario(matrix, scen_path):
    """
    剧本自动驾驶：加载并执行 scenarios/ 目录下的每一个 YAML 剧本。
    """
    print(f"\n🎬 Executing Scenario: {os.path.basename(scen_path)}")
    
    # 运行场景 (内部会自动调用控制 API 注入数据并断言结果)
    await matrix.run_scenario(scen_path)