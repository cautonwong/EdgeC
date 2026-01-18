import asyncio
import logging
import os
import argparse
from .engine.orchestrator import MatrixOrchestrator

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("Matrix.Server")

async def main():
    parser = argparse.ArgumentParser(description="Matrix Simulation Server (Standalone Runtime)")
    parser.add_argument("--schema", default="build/products/iems/generated", help="Path to generated contract schema")
    parser.add_argument("--port", type=int, default=9999, help="Control API Port")
    parser.add_argument("--world", help="Optional world topology configuration (YAML)")
    args = parser.parse_args()

    logger.info("Initializing Matrix Orchestrator...")
    orchestrator = MatrixOrchestrator(args.schema)
    
    # 自动构建拓扑 (优先使用 Manifest/World 配置)
    if orchestrator.loader.world_config:
        orchestrator.build_from_world()
    else:
        logger.warning("No topology found in schema, falling back to legacy defaults.")
        bus_paths = {0: "/tmp/vbus0.sock"}
        orchestrator.build_topology(bus_paths)
    
    # 启动交互式 API
    logger.info(f"Starting Interactive API on port {args.port}...")
    await orchestrator.start_interactive_api(args.port)
    
    # 启动总线仿真
    logger.info("Matrix Engine is now running. Press Ctrl+C to exit.")
    await orchestrator.run()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("Matrix Server stopped by user.")
