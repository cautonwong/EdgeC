import pytest
import os
import glob

# 自动扫描当前产品下的场景
def get_local_scenarios():
    # 定位到 cmd/iems/scenarios 或 features/scenarios
    base_dir = os.path.dirname(os.path.dirname(__file__))
    patterns = [
        os.path.join(base_dir, "scenarios/*.yaml"),
        os.path.join(base_dir, "features/scenarios/*.yaml")
    ]
    found = []
    for p in patterns:
        found.extend(glob.glob(p))
    return found

@pytest.mark.parametrize("scen_path", get_local_scenarios())
@pytest.mark.asyncio
async def test_yaml_scenario(matrix, scen_path):
    """
    产品级 YAML 剧本执行器：复活所有存量场景
    """
    print(f"\n🎭 Running Scenario: {os.path.basename(scen_path)}")
    await matrix.run_scenario(scen_path)
