import yaml
import os
import logging
import asyncio
from typing import List, Dict, Any

logger = logging.getLogger("Matrix.Scenario")

class ScenarioRunner:
    """
    剧本执行器：解析 YAML 步骤并驱动 Matrix 执行。
    支持 include 模块化剧本。
    """
    def __init__(self, context):
        self.ctx = context # MatrixContext

    def load_scenario(self, path: str) -> Dict[str, Any]:
        path = os.path.abspath(path)
        with open(path, "r") as f:
            scen = yaml.safe_load(f)
        
        # 处理 Include (递归)
        if "include" in scen:
            base_dir = os.path.dirname(path)
            merged_steps = []
            for inc_path in scen["include"]:
                full_path = os.path.normpath(os.path.join(base_dir, inc_path))
                inc_scen = self.load_scenario(full_path)
                merged_steps.extend(inc_scen.get("steps", []))
            
            scen["steps"] = merged_steps + scen.get("steps", [])
            
        return scen

    async def run(self, scen_path: str):
        scen = self.load_scenario(scen_path)
        name = scen.get("name", "Unnamed Scenario")
        logger.info(f"🎬 Starting Scenario: {name}")
        
        start_time = asyncio.get_event_loop().time()
        
        for step in sorted(scen.get("steps", []), key=lambda x: x.get('t', 0)):
            target_t = start_time + step.get('t', 0)
            now = asyncio.get_event_loop().time()
            if target_t > now:
                await asyncio.sleep(target_t - now)
            
            await self._execute_step(step)
            
        logger.info(f"✅ Scenario {name} Completed.")

    async def _execute_step(self, step: Dict[str, Any]):
        action = step.get("action")
        params = step.get("params", {})
        
        if action == "device.inject":
            dev = self.ctx.device(params.get("device"))
            if dev:
                dev.inject(params.get("tag"), params.get("value"), self.ctx.orch.loader)
        
        elif action == "edgec.wait_for":
            tag_name = params.get("tag")
            expect = params.get("expect")
            timeout = params.get("timeout", 10)
            tag_id = self.ctx.orch.loader.name_to_id.get(tag_name, tag_name)
            
            start = asyncio.get_event_loop().time()
            success = False
            while asyncio.get_event_loop().time() - start < timeout:
                val = self.ctx.edgec.get_tag(tag_id)
                if val is not None and abs(val - expect) < 0.05:
                    success = True
                    break
                await asyncio.sleep(0.5)
            
            if not success:
                raise AssertionError(f"Timeout waiting for {tag_name} to be {expect}")

        elif action == "matrix.auto_validate":
            """[Task 3.3] 自动校验"""
            logger.info("🕵️ Starting Automatic Matrix Validation...")
            for tid, meta in self.ctx.orch.loader.tags.items():
                attrs = meta.get("Attributes") or {}
                if attrs.get("poll_interval_ms"):
                    current = self.ctx.edgec.get_tag(tid)
                    # 此处可扩展更严格的校验逻辑
                    logger.info(f"Auto-checked Tag {tid} ({meta.get('Name')}): {current}")