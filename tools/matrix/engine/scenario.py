import yaml
import os
import logging
import asyncio
from typing import List, Dict, Any

logger = logging.getLogger("Matrix.Scenario")

class ScenarioRunner:
    """
    剧本执行器：解析 YAML 步骤并驱动 Matrix 执行。
    """
    def __init__(self, context):
        self.ctx = context # MatrixContext
        self._action_handlers = {
            "device.inject": self._handle_device_inject,
            "inject": self._handle_device_inject,        # 兼容旧版
            "edgec.wait_for": self._handle_wait_for,
            "check_api": self._handle_wait_for,         # 兼容旧版
            "matrix.auto_validate": self._handle_auto_validate,
            "chaos.config": self._handle_chaos_config,
        }

    def load_scenario(self, path: str) -> Dict[str, Any]:
        path = os.path.abspath(path)
        with open(path, "r") as f:
            scen = yaml.safe_load(f)
        
        # [恢复] 递归处理 Include
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
        
        # 按时间排序执行步骤
        steps = sorted(scen.get("steps", []), key=lambda x: x.get('t', 0))
        for step in steps:
            target_t = start_time + step.get('t', 0)
            now = asyncio.get_event_loop().time()
            if target_t > now:
                await asyncio.sleep(target_t - now)
            
            await self._execute_step(step)
            
        logger.info(f"✅ Scenario {name} Completed.")

    async def _execute_step(self, step: Dict[str, Any]):
        action = step.get("action")
        # 优先从 params 提取 tag/value 以支持旧版 YAML 结构
        handler = self._action_handlers.get(action)
        if handler:
            # 兼容性处理：如果 YAML 没有嵌套 params 字典，则将整个 step 作为参数
            params = step.get("params") or step
            await handler(params)
        else:
            logger.error(f"Unknown action: {action}")

    async def _handle_device_inject(self, params):
        device_name = params.get("device") or "Bus0_ModbusRTU_1" # 默认设备兼容
        dev = self.ctx.device(device_name)
        if dev:
            tag = params.get("tag")
            val = params.get("value")
            dev.inject(tag, val, self.ctx.orch.loader)

    async def _handle_wait_for(self, params):
        tag_name = params.get("tag")
        expect = params.get("expect") or params.get("value") # 兼容旧版 expect/value 字段名
        timeout = params.get("timeout", 15)
        tag_id = self.ctx.orch.loader.name_to_id.get(tag_name, tag_name)
        
        logger.info(f"⏳ Waiting for {tag_name} ({tag_id}) to reach {expect}...")
        start = asyncio.get_event_loop().time()
        while asyncio.get_event_loop().time() - start < timeout:
            val = self.ctx.edgec.get_tag(tag_id)
            if val is not None and abs(val - expect) < 0.05:
                logger.info(f"✨ Match found: {val}")
                return
            await asyncio.sleep(0.5)
        raise AssertionError(f"Timeout waiting for {tag_name} to be {expect}")

    async def _handle_chaos_config(self, params):
        """[增强] 动态调整 Chaos 参数"""
        bus_id = params.get("bus_id", 0)
        if bus_id in self.ctx.orch.buses:
            bus = self.ctx.orch.buses[bus_id]
            if hasattr(bus.transport, 'failure_rate'):
                bus.transport.failure_rate = params.get("failure_rate", 0.0)
                bus.transport.latency_ms = params.get("latency_ms", 10)
                logger.info(f"🔥 Chaos Configured on Bus {bus_id}: Failure={bus.transport.failure_rate:.1%}")

    async def _handle_auto_validate(self, params):
        logger.info("🕵️ Starting Automatic Matrix Validation...")
        # ... 保持原有逻辑 ...