import pytest
import json
import os
import glob
import shutil
from engine.director import OmnisDirector
from engine.observer import OmnisObserver

def pytest_addoption(parser):
    parser.addoption("--build-dir", action="store", default="build", help="Path to build directory")

def pytest_generate_tests(metafunc):
    """
    扫描所有产品的 Manifest，动态生成测试用例。
    """
    if "scenario_context" in metafunc.fixturenames:
        build_dir = os.path.abspath(metafunc.config.getoption("--build-dir"))
        manifest_pattern = os.path.join(build_dir, "products/*/generated/shadow_schema.json")
        manifests = glob.glob(manifest_pattern)
        
        argvalues = []
        ids = []
        seen_scenarios = set()
        
        for m_path in manifests:
            try:
                with open(m_path, "r") as f:
                    manifest = json.load(f)
                
                product_name = manifest.get("Product", "unknown")
                product_dir = os.path.dirname(os.path.dirname(m_path))
                
                # [关键修复] 增加对 None 或空列表的防御
                scenarios = manifest.get("Scenarios") or []
                for scen_path in scenarios:
                    if not os.path.isabs(scen_path):
                        project_root = os.path.abspath(os.path.join(build_dir, "../.."))
                        scen_path = os.path.join(project_root, scen_path)
                    
                    scen_name = os.path.basename(scen_path).replace(".yaml", "")
                    
                    # 避免同一个产品重复加载同一个剧本 (比如软链接导致的)
                    unique_id = f"{product_name}:{scen_name}"
                    if unique_id in seen_scenarios: continue
                    seen_scenarios.add(unique_id)
                    
                    ctx = {
                        "product": product_name,
                        "product_dir": product_dir,
                        "scenario_path": scen_path,
                        "manifest": manifest
                    }
                    argvalues.append(ctx)
                    ids.append(unique_id)
            except Exception as e:
                print(f"Error loading manifest {m_path}: {e}")

        if argvalues:
            metafunc.parametrize("scenario_context", argvalues, ids=ids)

@pytest.fixture
def run_env(scenario_context):
    prod = scenario_context["product"]
    scen = os.path.basename(scenario_context["scenario_path"]).replace(".yaml", "")
    
    base_results = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../build/test_results"))
    case_dir = os.path.join(base_results, prod, scen)
    
    if os.path.exists(case_dir):
        shutil.rmtree(case_dir)
    os.makedirs(case_dir, exist_ok=True)
    
    return {
        "log_dir": case_dir,
        "product_dir": scenario_context["product_dir"]
    }

@pytest.fixture
def director(run_env, scenario_context):
    d = OmnisDirector(run_env["product_dir"], log_dir=run_env["log_dir"])
    yield d
    d.stop_all()

@pytest.fixture
def observer(director):
    return OmnisObserver(director.rest_port, director.db_path)
