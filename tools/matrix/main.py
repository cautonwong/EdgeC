import asyncio
import logging
import argparse
import sys
import os

# 确保项目根目录在路径中
sys.path.insert(0, os.getcwd())

from tools.matrix.engine.orchestrator import MatrixOrchestrator

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("Matrix.Main")

async def run_server(args):
    """
    Standalone Mode: 启动仿真服务器并等待控制。
    """
    logger.info(f"Starting Matrix in SERVER mode (Schema: {args.schema})")
    orch = MatrixOrchestrator(args.schema)
    
    # 建立拓扑
    bus_paths = {0: "/tmp/vbus0.sock"}
    orch.build_topology(bus_paths)
    
    # 开启交互式 API
    await orch.start_interactive_api(args.control_port)
    
    # 运行仿真总线
    await orch.run()

async def run_test(args):
    """
    Test Mode: 启动并执行特定的 Pytest 用例。
    """
    import pytest
    logger.info(f"Starting Matrix in TEST mode (Pattern: {args.test_pattern})")
    sys.exit(pytest.main(["-c", "tools/matrix/pytest.ini", args.test_pattern, "-s"]))

async def main():
    parser = argparse.ArgumentParser(description="Matrix Unified Simulation Engine")
    parser.add_argument("--mode", choices=["server", "test"], default="server", help="Operational mode")
    parser.add_argument("--schema", default="build/products/iems/generated", help="Contract path")
    parser.add_argument("--control-port", type=int, default=9999, help="Interactive API port")
    parser.add_argument("--test-pattern", help="Pytest pattern for test mode")
    
    args = parser.parse_args()

    if args.mode == "server":
        await run_server(args)
    else:
        if not args.test_pattern:
            logger.error("Test mode requires --test-pattern")
            sys.exit(1)
        await run_test(args)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("Matrix exiting...")