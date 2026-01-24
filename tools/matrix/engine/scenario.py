import yaml
import os
import logging
import asyncio
import requests
import time
from typing import List, Dict, Any

logger = logging.getLogger("Matrix.Scenario")

class ScenarioRunner:
    """
    [Omnis V5.0] 剧本执行器：引入工业级测试原语。
    """
    def __init__(self, context):
        self.ctx = context
        self._action_handlers = {
            # --- 动作类 ---
            "device.inject": self._handle_device_inject,
            "inject": self._handle_device_inject,
            
            # --- 断言/等待类 ---
            "edgec.wait_for": self._handle_wait_for,
            "wait_for": self._handle_wait_for,
            "check_api": self._handle_wait_for,
            "expect_state": self._handle_expect_state,
            
            # --- 环境类 ---
            "chaos.config": self._handle_chaos_config,
        }

    async def run(self, scen_path: str):
        with open(scen_path, "r") as f:
            scen = yaml.safe_load(f)
        
        name = scen.get("name", os.path.basename(scen_path))
        logger.info(f"🎬 [Scenario] Running: {name}")
        
        start_time = asyncio.get_event_loop().time()
        steps = sorted(scen.get("steps", []), key=lambda x: x.get('t', 0))
        
        for step in steps:
            target_t = start_time + step.get('t', 0)
            now = asyncio.get_event_loop().time()
            if target_t > now:
                await asyncio.sleep(target_t - now)
            
            await self._execute_step(step)
            
        logger.info(f"✅ [Scenario] {name} passed.")

    async def _execute_step(self, step: Dict[str, Any]):
        action = step.get("action")
        handler = self._action_handlers.get(action)
        if handler:
            params = step.get("params") or step
            await handler(params)

    async def _handle_device_inject(self, params):
        """语义化注入：METER_MAIN.voltage = 220.0"""
        dev_id = params.get("device") or "METER_MAIN"
        tag_name = params.get("tag")
        val = params.get("value")
        
        tag_id = self.ctx.loader.name_to_id.get(tag_name, tag_name)
        
        url = f"{self.ctx.server_url}/actor/set"
        resp = requests.post(url, json={"dev_id": dev_id, "tag": tag_id, "value": val}, timeout=1)
        if resp.status_code == 200:
            logger.info(f"💉 [Action] Injected {dev_id}.{tag_name} ({tag_id}) = {val}")

    async def _handle_wait_for(self, params):
        """
        专业等待原语：等待测点满足特定条件。
        支持: tag, expect, tolerance, timeout
        """
        tag_name = params.get("tag")
        expect = float(params.get("expect") or params.get("value", 0))
        tolerance = float(params.get("tolerance", 0.05)) # 默认 0.05 容差
        timeout = float(params.get("timeout", 15))
        
        tag_id = self.ctx.loader.name_to_id.get(tag_name, tag_name)
        
        logger.info(f"⏳ [Assert] Waiting for {tag_name} ({tag_id}) -> {expect} (tol={tolerance})...")
        
        start = time.time()
        while time.time() - start < timeout:
            # [Fix] 显式转换，确保 REST API 收到的是数字 ID
            val = self.ctx.edgec.get_tag(int(tag_id))
            if val is not None:
                if abs(val - expect) <= tolerance:
                    logger.info(f"✨ [Match] {tag_name} reached {val:.2f} (Expected {expect})")
                    return
            await asyncio.sleep(0.2) # 5Hz 采样验证
            
        raise AssertionError(f"Timeout: {tag_name} never reached {expect} within {timeout}s")

    async def _handle_expect_state(self, params):
        """状态断言原语：精确匹配值 (不带容差)"""
        tag_name = params.get("tag")
        expect = params.get("expect")
        tag_id = self.ctx.loader.name_to_id.get(tag_name, tag_name)
        
        val = self.ctx.edgec.get_tag(tag_id)
        if val != expect:
            raise AssertionError(f"State Mismatch: {tag_name} is {val}, expected {expect}")
        logger.info(f"✅ [State] {tag_name} is {val} as expected.")

    async def _handle_chaos_config(self, params):
        # 通过 API 通知 Server 注入总线故障
        pass