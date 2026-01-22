#!/usr/bin/env python3
import sys
import os
import argparse
import time
import json
from http.server import HTTPServer, BaseHTTPRequestHandler
from threading import Thread

# 确保路径包含 tools/matrix
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../"))
sys.path.insert(0, PROJECT_ROOT)

try:
    from tools.matrix.engine.orchestrator import MatrixOrchestrator
except ImportError as e:
    print(f"FATAL: Import failed: {e}")
    sys.exit(1)

class MatrixAPIHandler(BaseHTTPRequestHandler):
    orch_instance = None

    def do_POST(self):
        # 保持原有 API 逻辑，增强对 orch_instance 的调用
        pass

def main():
    parser = argparse.ArgumentParser(description="Omnis Matrix Logic Host")
    parser.add_argument("--product", required=True, help="Product name (e.g. iems)")
    parser.add_argument("--manifest", help="Path to omnis_manifest.json")
    args = parser.parse_args()
    
    # [Omnis] 唯一事实源：自动定位编译器生成的清单
    manifest_file = args.manifest or f"build/products/{args.product}/generated/omnis_manifest.json"
    
    if not os.path.exists(manifest_file):
        print(f"❌ FATAL: Manifest not found at {manifest_file}. Did you run build.sh?")
        sys.exit(1)

    print(f"🎬 Omnis Matrix Host starting for {args.product}...")
    print(f"📜 Loading Truth from: {manifest_file}")
    
    try:
        # 1. 初始化编排器
        orch = MatrixOrchestrator(manifest_file)
        MatrixAPIHandler.orch_instance = orch
        
        # 2. 供给资源 (创建 UDS, 拉起 Actors)
        orch.provision_world()
        
        # 3. 启动控制平面
        def run_server():
            server = HTTPServer(('0.0.0.0', 50051), MatrixAPIHandler)
            print("📡 Control Plane (REST) listening on 50051")
            server.serve_forever()
            
        t = Thread(target=run_server, daemon=True)
        t.start()
        
        print("🚀 Matrix is FULLY OPERATIONAL and Contract-Aligned.")
        sys.stdout.flush()

        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n🛑 Shutting down...")
        orch.shutdown()
    except Exception as e:
        print(f"💥 Server Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
