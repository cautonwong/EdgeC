#!/usr/bin/env python3
import sys
import os
import signal
import time
import argparse
import json
from http.server import HTTPServer, BaseHTTPRequestHandler
from threading import Thread

# 强行设置路径
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(PROJECT_ROOT))

try:
    from tools.matrix.engine.orchestrator import MatrixOrchestrator
except ImportError as e:
    print(f"FATAL: Import failed: {e}")
    sys.exit(1)

class MatrixAPIHandler(BaseHTTPRequestHandler):
    orch_instance = None # 使用类变量存储

    def do_POST(self):
        try:
            content_length = int(self.headers.get('Content-Length', 0))
            post_data = self.rfile.read(content_length)
            payload = json.loads(post_data)
            
            if self.path == "/api/v1/actor/set":
                dev_id = payload.get("dev_id")
                tag = payload.get("tag")
                val = payload.get("value")
                if MatrixAPIHandler.orch_instance and MatrixAPIHandler.orch_instance.update_tag(dev_id, tag, val):
                    self.send_response(200)
                    self.end_headers()
                    self.wfile.write(b'{"status": "ok"}')
                else:
                    self.send_response(500)
                    self.end_headers()
            else:
                self.send_response(404)
                self.end_headers()
        except Exception as e:
            self.send_response(500)
            self.end_headers()
            print(f"API Error: {e}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--product", required=True)
    parser.add_argument("--world")
    args = parser.parse_args()
    
    world_file = args.world or f"cmd/{args.product}/world.yaml"
    print(f"Starting Omnis Server for {args.product}...")
    print(f"Loading world: {world_file}")
    
    try:
        orch = MatrixOrchestrator(args.product, world_file)
        # 核心修复：在启动服务前绑定单例
        MatrixAPIHandler.orch_instance = orch
        
        def run_server():
            server = HTTPServer(('0.0.0.0', 50051), MatrixAPIHandler)
            print("📡 Control Plane listening on 50051")
            server.serve_forever()
            
        t = Thread(target=run_server, daemon=True)
        t.start()
        
        time.sleep(1) # 等待 API 就绪
        
        instance_id = f"{args.product}_node_1"
        orch.provision_world(instance_id)
        
        print("🚀 Matrix Server is FULLY OPERATIONAL.")
        sys.stdout.flush()

        while True:
            time.sleep(1)
    except Exception as e:
        print(f"FATAL: Server crashed during startup: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()