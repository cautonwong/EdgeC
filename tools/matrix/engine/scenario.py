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
            "edgec.wait_for": self._handle_wait_for,
            "matrix.auto_validate": self._handle_auto_validate,
            "chaos.config": self._handle_chaos_config,
        }

    async def _execute_step(self, step: Dict[str, Any]):
        action = step.get("action")
        handler = self._action_handlers.get(action)
        if handler:
            await handler(step.get("params", {}))
        else:
            logger.error(f"Unknown action: {action}")

    async def _handle_device_inject(self, params):
        dev = self.ctx.device(params.get("device"))
        if dev:
            dev.inject(params.get("tag"), params.get("value"), self.ctx.orch.loader)

    async def _handle_wait_for(self, params):
        tag_name = params.get("tag")
        expect = params.get("expect")
        timeout = params.get("timeout", 10)
        tag_id = self.ctx.orch.loader.name_to_id.get(tag_name, tag_name)
        
        start = asyncio.get_event_loop().time()
        while asyncio.get_event_loop().time() - start < timeout:
            val = self.ctx.edgec.get_tag(tag_id)
            if val is not None and abs(val - expect) < 0.05:
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