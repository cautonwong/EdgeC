import pytest
import asyncio
import os
import random
import logging
from datetime import datetime
import pytest_asyncio
import requests
import shutil

from tools.matrix.engine.orchestrator import MatrixOrchestrator
from tools.matrix.engine.edgec import MatrixEdgeC
from tools.matrix.engine.scenario import ScenarioRunner
from tools.matrix.engine.scenario_engine import IonScenarioRunner
from tools.matrix.engine.loader import SchemaLoader

class VDeviceProxy:
    def __init__(self, dev_id, server_url, loader):
        self.dev_id = dev_id
        self.server_url = server_url
        self.loader = loader

    def inject(self, tag_name, value, loader=None):
        """向 Matrix Server 发送注入请求"""
        # 优先使用传入的 loader，否则使用初始化时的
        actual_loader = loader or self.loader
        
        # 转换 TagName 为 ID (如果需要)
        tag_id = tag_name
        if actual_loader and str(tag_name) in actual_loader.name_to_id:
            tag_id = actual_loader.name_to_id[str(tag_name)]

        url = f"{self.server_url}/actor/set"
        payload = {"dev_id": self.dev_id, "tag": tag_id, "value": float(value)}
        resp = requests.post(url, json=payload, timeout=2)
        if resp.status_code != 200:
            raise RuntimeError(f"Failed to inject to {self.dev_id}: {resp.text}")

class MatrixContext:
    def __init__(self, loader, edgec, server_url):
        self.loader = loader
        self.edgec = edgec
        self.server_url = server_url
        self.runner = IonScenarioRunner(self)
        # 模拟 orch 属性供测试用例使用
        self.orch = self

    def device(self, dev_id):
        return VDeviceProxy(dev_id, self.server_url, self.loader)

    def adc_control(self, target: str, params: dict):
        """发送控制指令给 ADC 模拟器"""
        import socket
        import json
        ctrl_path = "/tmp/matrix_adc_ctrl.sock"
        if os.path.exists(ctrl_path):
            with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as s:
                payload = json.dumps({"target": target, "params": params}).encode()
                s.sendto(payload, ctrl_path)
                print(f"  [Matrix] Sent ADC Control: {target} -> {params}")

    def get_val(self, tag_name):
        tag_id = tag_name
        if self.loader and tag_name in self.loader.name_to_id:
            tag_id = self.loader.name_to_id[tag_name]
        return self.edgec.get_tag(tag_id)

    async def wait_for(self, tag_name, expect, tolerance=0.05, timeout=30):
        """等待测点达到预期值"""
        tag_id = tag_name
        if self.loader and tag_name in self.loader.name_to_id:
            tag_id = self.loader.name_to_id[tag_name]
        
        start = datetime.now()
        while (datetime.now() - start).total_seconds() < timeout:
            val = self.edgec.get_tag(tag_id)
            if val is not None:
                if abs(float(val) - float(expect)) <= tolerance:
                    return True
            await asyncio.sleep(0.5)
        return False

    async def run_scenario(self, path):
        await self.runner.run(path)

@pytest_asyncio.fixture(scope="session")
async def matrix_env():
    server_host = os.environ.get("OMNIS_SERVER", "localhost:50051")
    product_name = os.environ.get("OMNIS_PRODUCT", "iems")
    base_url = f"http://{server_host}/api/v1"
    
    project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../"))
    manifest_path = os.path.join(project_root, f"build/products/{product_name}/generated/omnis_manifest.json")
    
    loader = SchemaLoader(manifest_path)
    yield {"loader": loader, "base_url": base_url, "project_root": project_root}

@pytest_asyncio.fixture
async def matrix(matrix_env, tmp_path):
    """
    [Task 5.1/5.2] 真正隔离的测试环境。
    使用 Pytest tmp_path 自动管理数据库文件生命周期。
    """
    loader = matrix_env["loader"]
    base_url = matrix_env["base_url"]
    project_root = matrix_env["project_root"]
    product_name = os.environ.get("OMNIS_PRODUCT", "iems")
    
    bin_path = os.path.join(project_root, f"build/products/{product_name}/bin/{product_name}")
    
    # 使用 Pytest tmp_path：不再产生孤儿 .db 文件
    # [整改] 强制使用绝对路径，防止子进程定位偏移
    db_path = os.path.abspath(str(tmp_path / "test_assets.db"))
    rest_port = random.randint(20000, 30000)
    
    # 我们还需要解决 SHM 共享内存重名问题
    shm_name = f"matrix_shm_{random.randint(1000,9999)}"
    
    # 注入 VBus 路径
    vbus_path = os.environ.get("OMNIS_BUS_0_DEV", "/tmp/vbus0.sock")
    
    # [Nexus] 清理全局观察者数据，防止测试干扰
    try:
        requests.post(f"{base_url}/observer/reset", timeout=1)
    except:
        pass

    edgec = MatrixEdgeC(bin_path, vbus_path, rest_port, db_path=db_path)
    edgec.bootstrap_environment(loader)
    
    # 启动进程时注入随机 SHM 名
    os.environ["EDGE_SHM_NAME"] = shm_name
    edgec.start(cwd=project_root)
    
    ctx = MatrixContext(loader, edgec, base_url)
    try:
        yield ctx
    finally:
        edgec.stop()
        # tmp_path 由 Pytest 自动管理回收，无需手动 remove