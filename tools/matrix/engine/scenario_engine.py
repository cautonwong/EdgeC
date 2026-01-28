import os
import yaml
import asyncio
import logging
import socket
import json
import requests
import numpy as np
from typing import Dict, Any, List
from tools.matrix.engine.scenario import ScenarioRunner
from tools.matrix.engine.verifier import RMSVerifier

logger = logging.getLogger("Matrix.IonEngine")

class IonScenarioRunner(ScenarioRunner):
    """
    [ION-3.1] 专门针对高频波形仿真的剧本执行器。
    扩展了对 AdcSimulator 控制套接字的支持。
    """
    def __init__(self, context, adc_ctrl_path="/tmp/matrix_adc_ctrl.sock"):
        super().__init__(context)
        self.adc_ctrl_path = adc_ctrl_path
        self.rms_verifier = RMSVerifier(tolerance=0.002) # 0.2% 精度要求
        
        # 注册 Ion 特有的动作处理器
        self._action_handlers.update({
            "adc.set_param": self._handle_adc_set_param,
            "adc.inject_fault": self._handle_adc_inject_fault,
            "adc.verify_rms": self._handle_adc_verify_rms,
        })

    def _send_adc_cmd(self, target: str, params: Dict[str, Any]):
        """通过 UDP 发送指令给 AdcSimulator"""
        if not os.path.exists(self.adc_ctrl_path):
            logger.warning(f"⚠️ ADC Ctrl socket {self.adc_ctrl_path} not found")
            return

        cmd = {"target": target, "params": params}
        with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as s:
            try:
                s.sendto(json.dumps(cmd).encode(), self.adc_ctrl_path)
                logger.info(f"🎮 [Ion-Action] Cmd sent to {target}: {params}")
            except Exception as e:
                logger.error(f"❌ Failed to send ADC cmd: {e}")

    async def _handle_adc_set_param(self, params):
        """设置波形基础参数 (freq, amp, imbalance)"""
        target = params.get("target", "grid")
        p = params.get("params") or params
        # 移除非参数字段
        p_clean = {k: v for k, v in p.items() if k not in ["action", "t", "target"]}
        self._send_adc_cmd(target, p_clean)

    async def _handle_adc_inject_fault(self, params):
        """注入暂态故障 (sag, swell)"""
        fault_type = params.get("type", "sag")
        ratio = float(params.get("ratio", 1.0))
        duration = float(params.get("duration_ms", 500)) / 1000.0
        
        # 1. 触发故障
        self._send_adc_cmd("transient", {"active": True, "factor": ratio})
        logger.info(f"💥 [Fault] Injected {fault_type} (ratio={ratio}) for {duration}s")
        
        # 2. 等待故障结束
        await asyncio.sleep(duration)
        
        # 3. 恢复正常
        self._send_adc_cmd("transient", {"active": False, "factor": 1.0})
        logger.info(f"🛡️ [Fault] {fault_type} recovered.")

    async def _handle_adc_verify_rms(self, params):
        """
        [ION-3.2] 自动化精度校验。
        它会计算模拟器当前的“理论有效值”并与 EdgeC 的实时输出对比。
        """
        tag_id = 2002 # ADC_VOLTAGE_RMS (from tags.yaml)
        expected_amp = float(params.get("expected_rms", 220.0))
        
        logger.info(f"🧐 [Verify] Comparing EdgeC RMS (Tag {tag_id}) against reference...")
        
        # 尝试从 EdgeC 获取最新的计算结果 (通过 REST API)
        try:
            # 假设 context 中有获取数据的接口
            val = self.ctx.edgec.get_tag(tag_id)
            if val is not None:
                self.rms_verifier.verify(val, expected_amp)
            else:
                logger.warning("⚠️ Could not fetch RMS result from EdgeC")
        except Exception as e:
            logger.error(f"❌ Verification error: {e}")

# 集成脚本启动器
if __name__ == "__main__":
    import argparse
    from tools.matrix.engine.orchestrator import MatrixContext
    
    parser = argparse.ArgumentParser()
    parser.add_argument("scenario", help="Path to YAML scenario file")
    args = parser.parse_args()

    # 模拟 Context (在真实 Matrix 环境中由 Orchestrator 传入)
    class MockContext:
        def __init__(self):
            from tools.matrix.engine.edgec import EdgeCAPI
            self.edgec = EdgeCAPI("http://127.0.0.1:9091")
            self.loader = type('obj', (object,), {'name_to_id': {}})()

    ctx = MockContext()
    runner = IonScenarioRunner(ctx)
    asyncio.run(runner.run(args.scenario))
