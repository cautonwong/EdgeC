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
    product_dir = scenario_context["product_dir"]
    scen_name = os.path.basename(scenario_context["scenario_path"]).replace(".yaml", "")
    
    # [镜像化布局] build/products/<prod>/test_reports/<scen>
    case_dir = os.path.join(product_dir, "test_reports", scen_name)
    
    if os.path.exists(case_dir):
        shutil.rmtree(case_dir)
    os.makedirs(case_dir, exist_ok=True)
    
    return {
        "log_dir": case_dir,
        "product_dir": product_dir
    }

@pytest.fixture
def director(run_env):
    d = OmnisDirector(run_env["product_dir"], log_dir=run_env["log_dir"])
    yield d
    d.stop_all()

@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    # 获取测试结果
    outcome = yield
    report = outcome.get_result()
    
    if report.when == "call" and report.failed:
        # 如果测试失败，尝试抓取 EdgeC 日志
        if "director" in item.funcargs:
            director = item.funcargs["director"]
            log_path = os.path.join(director.log_dir, "edgec.log")
            if os.path.exists(log_path):
                with open(log_path, "r") as f:
                    lines = f.readlines()
                    # 只取最后 50 行
                    tail = "".join(lines[-50:])
                    print(f"\n--- [EdgeC LOG TAIL ON FAILURE] ---\n{tail}\n--- END LOG ---")

@pytest.fixture
def observer(director):
    return OmnisObserver(director.rest_port, director.db_path)
