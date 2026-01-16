import pytest
import asyncio
import os
import random
import logging
import shutil
import json
import glob
from datetime import datetime
import pytest_asyncio
from tools.matrix.engine.orchestrator import MatrixOrchestrator
from tools.matrix.engine.edgec import MatrixEdgeC
from tools.matrix.engine.scenario import ScenarioRunner
from tools.matrix.engine.transport import UDSTransport

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
                # 跳过没有 Scenarios 的产品
                if not manifest.get("Scenarios"):
                    continue

                for scen_path in manifest["Scenarios"]:
                    scen_name = os.path.basename(scen_path).replace(".yaml", "")
                    
                    unique_id = f"{product_name}:{scen_name}"
                    if unique_id in seen_scenarios: continue
                    seen_scenarios.add(unique_id)
                    
                    # 获取可执行文件路径：
                    # 通常在 bin/ 下，文件名可能与目录名一致 (iems) 
                    product_base_dir = os.path.dirname(os.path.dirname(m_path))
                    bin_dir = os.path.join(product_base_dir, "bin")
                    
                    # 尝试寻找可执行文件
                    bin_path = os.path.join(bin_dir, product_name)
                    if not os.path.exists(bin_path):
                        # 如果 IEMS_Controller_V1 不存在，尝试 iems (目录名)
                        dir_name = os.path.basename(product_base_dir)
                        bin_path = os.path.join(bin_dir, dir_name)

                    ctx = {
                        "product": product_name,
                        "scenario_path": scen_path,
                        "schema_path": m_path,
                        "bin_path": bin_path
                    }
                    argvalues.append(ctx)
                    ids.append(unique_id)
            except Exception as e:
                print(f"Error loading manifest {m_path}: {e}")

        if argvalues:
            metafunc.parametrize("scenario_context", argvalues, ids=ids)

class MatrixContext:
    def __init__(self, orch, edgec):
        self.orch = orch
        self.edgec = edgec
        self.product = orch.loader.get_product_name()
        self.runner = ScenarioRunner(self)

    def device(self, name):
        return self.orch.get_device(name)

    async def run_scenario(self, path):
        await self.runner.run(path)

    def get_val(self, tag_name):
        return self.edgec.get_tag(tag_name, self.orch.loader)

    async def wait_until(self, predicate, timeout=15, interval=0.2):
        start = asyncio.get_event_loop().time()
        while asyncio.get_event_loop().time() - start < timeout:
            if await predicate():
                return True
            await asyncio.sleep(interval)
        return False

    async def wait_for(self, tag_name, expect, timeout=15):
        """语义化等待：等待点位到达目标值"""
        async def check_condition():
            val = self.edgec.get_tag(tag_name, self.orch.loader)
            return val is not None and abs(val - expect) < 0.05
        return await self.wait_until(check_condition, timeout=timeout)

@pytest_asyncio.fixture
async def matrix(request, scenario_context):
    product_name = scenario_context["product"]
    schema_path = scenario_context["schema_path"]
    bin_path = scenario_context["bin_path"]
    
    # 建立隔离的测试报告目录
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    case_name = request.node.name.replace(":", "_")
    report_dir = f"build/products/{product_name}/matrix_reports/{timestamp}/{case_name}"
    os.makedirs(report_dir, exist_ok=True)

    # 1. 启动仿真矩阵 (优先从 extension 加载)
    ext_path = f"cmd/{product_name}/extension.yaml"
    orch = MatrixOrchestrator(schema_path, log_dir=report_dir)
    from tools.matrix.engine.loader import SchemaLoader
    orch.loader = SchemaLoader(schema_path, ext_path)
    
    orch.build_from_world()
    
    matrix_task = asyncio.create_task(orch.run())
    await asyncio.sleep(0.5)

    # 2. 启动 EdgeC
    rest_port = random.randint(20000, 30000)
    db_path = f"{product_name}.db"
    edgec = MatrixEdgeC(bin_path, "/tmp/dummy", rest_port, db_path=db_path)
    
    def start_with_multibus():
        env = os.environ.copy()
        if 0 in orch.buses:
            env["OMNIS_VBUS_PATH"] = orch.buses[0].transport.path
            
        for bid, bus in orch.buses.items():
            if isinstance(bus.transport, UDSTransport):
                env[f"OMNIS_VBUS_PATH_{bid}"] = bus.transport.path
        
        import subprocess
        project_root = os.path.abspath(os.getcwd())
        env["LD_LIBRARY_PATH"] = os.path.join(project_root, ".local/linux-x86_64/lib")
        env["EDGE_REST_PORT"] = str(edgec.rest_port)
        
        log_path = os.path.join(report_dir, "edgec_raw.log")
        log_f = open(log_path, "w")
        edgec.process = subprocess.Popen([edgec.bin_path], env=env, stdout=log_f, stderr=log_f, preexec_fn=os.setsid)
        edgec._wait_ready()

    edgec.start = start_with_multibus
    edgec.bootstrap_environment(orch.loader)
    edgec.start()
    
    ctx = MatrixContext(orch, edgec)
    yield ctx
    
    # 3. 清理与归档
    edgec.stop()
    matrix_task.cancel()
    try: await matrix_task
    except asyncio.CancelledError: pass
    
    with open(os.path.join(report_dir, "index.html"), "w") as f:
        f.write(f"<html><body><h1>Case: {case_name}</h1>")
        f.write(f"<p>Product: {product_name}</p>")
        f.write(f"<ul><li><a href='vbus_traffic.log'>VBus Traffic (Raw)</a></li>")
        f.write(f"<li><a href='edgec_raw.log'>EdgeC Console Log</a></li></ul>")
        f.write("</body></html>")