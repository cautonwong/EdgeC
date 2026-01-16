import pytest
import asyncio
import os
import random
import logging
import shutil
from datetime import datetime
import pytest_asyncio # Added for explicit async fixture marking
from tools.matrix.engine.orchestrator import MatrixOrchestrator
from tools.matrix.engine.edgec import MatrixEdgeC
from tools.matrix.engine.scenario import ScenarioRunner
from tools.matrix.engine.transport import UDSTransport

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
        """
        专业等待逻辑：直到 predicate() 返回 True 或超时。
        """
        start = asyncio.get_event_loop().time()
        while asyncio.get_event_loop().time() - start < timeout:
            if await predicate():
                return True
            await asyncio.sleep(interval)
        return False

    async def wait_for(self, tag_name, expect, timeout=15):
        """语义化等待：等待点位到达目标值"""
        tag_id = self.orch.loader.name_to_id.get(tag_name, tag_name)
        async def check_condition():
            val = self.edgec.get_tag(tag_id)
            return val is not None and abs(val - expect) < 0.05
        return await self.wait_until(check_condition, timeout=timeout)

@pytest_asyncio.fixture # Changed to pytest_asyncio.fixture
async def matrix(request):
    # 自动识别产品名称
    product_name = os.environ.get("MATRIX_PRODUCT", "iems")
    schema_path = f"build/products/{product_name}/generated/shadow_schema.json"
    bin_path = f"build/products/{product_name}/bin/{product_name}"
    
    # 建立隔离的测试报告目录
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    case_name = request.node.name
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
    
    # 覆盖 EdgeC 的 start 逻辑以支持多总线环境变量
    def start_with_multibus():
        env = os.environ.copy()
        # 兼容单总线默认变量
        if 0 in orch.buses:
            env["OMNIS_VBUS_PATH"] = orch.buses[0].transport.path
            
        for bid, bus in orch.buses.items():
            if isinstance(bus.transport, UDSTransport):
                env[f"OMNIS_VBUS_PATH_{bid}"] = bus.transport.path
        
        import subprocess
        project_root = os.path.abspath(os.getcwd())
        env["LD_LIBRARY_PATH"] = os.path.join(project_root, ".local/linux-x86_64/lib")
        env["EDGE_REST_PORT"] = str(edgec.rest_port)
        
        log_f = open("build/matrix_edgec.log", "w")
        edgec.process = subprocess.Popen([edgec.bin_path], env=env, stdout=log_f, stderr=log_f, preexec_fn=os.setsid)
        edgec._wait_ready()

    edgec.start = start_with_multibus
    
    # 环境自举
    edgec.bootstrap_environment(orch.loader)
    edgec.start()
    
    ctx = MatrixContext(orch, edgec)
    yield ctx
    
    # 3. 清理与归档
    edgec.stop()
    matrix_task.cancel()
    try: await matrix_task
    except asyncio.CancelledError: pass
    
    if os.path.exists("build/matrix_edgec.log"):
        shutil.move("build/matrix_edgec.log", os.path.join(report_dir, "edgec.log"))
    
    with open(os.path.join(report_dir, "index.html"), "w") as f:
        f.write(f"<html><body><h1>Case: {case_name}</h1>")
        f.write(f"<p>Product: {product_name}</p>")
        f.write(f"<ul><li><a href='vbus_traffic.log'>VBus Traffic (Raw)</a></li>")
        f.write(f"<li><a href='edgec.log'>EdgeC Console Log</a></li></ul>")
        f.write("</body></html>")