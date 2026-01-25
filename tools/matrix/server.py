#!/usr/bin/env python3
import sys
import os
import argparse
import time
import json
from http.server import HTTPServer, BaseHTTPRequestHandler
from threading import Thread

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../"))
sys.path.insert(0, PROJECT_ROOT)

try:
    from tools.matrix.engine.orchestrator import MatrixOrchestrator
    from tools.matrix.engine.edgec import MatrixEdgeC
except ImportError as e:
    print(f"FATAL: Import failed: {e}")
    sys.exit(1)

class MatrixAPIHandler(BaseHTTPRequestHandler):
    orch_instance = None
    observer_data = {} # { "dev_id:tag_id": value }

    def do_GET(self):
        """处理查询指令: ls devices, ls buses, get observed val"""
        if self.path == "/api/v1/devices":
            devs = [d["id"] for d in self.orch_instance.loader.devices]
            self._send_json({"devices": devs})
        elif self.path == "/api/v1/connectivity":
            conns = [{"id": c["id"], "label": c["label"]} for c in self.orch_instance.loader.connectivity]
            self._send_json({"buses": conns})
        elif self.path.startswith("/api/v1/observer/latest/"):
            # Format: /api/v1/observer/latest/SUPERVISOR_01/1001
            parts = self.path.split('/')
            if len(parts) >= 6:
                key = f"{parts[4]}:{parts[5]}"
                val = self.observer_data.get(key)
                self._send_json({"value": val})
            else:
                self.send_response(400)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        """处理控制指令与观测上报"""
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length)
        try:
            payload = json.loads(post_data)
            if self.path == "/api/v1/actor/set":
                dev_id = payload.get("dev_id")
                tag = payload.get("tag")
                val = payload.get("value")
                
                # 调用编排器更新模拟器状态
                if self.orch_instance.update_device_tag(dev_id, tag, val):
                    self._send_json({"status": "ok"})
                else:
                    self.send_response(404)
                    self.end_headers()
            elif self.path == "/api/v1/observer/record":
                dev_id = payload.get("dev_id")
                tag_id = str(payload.get("tag"))
                val = payload.get("value")
                key = f"{dev_id}:{tag_id}"
                self.observer_data[key] = val
                self._send_json({"status": "ok"})
        except Exception as e:
            self.send_response(500)
            self.end_headers()
            print(f"API Error: {e}")

    def _send_json(self, data):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--product", required=True)
    parser.add_argument("--manifest")
    args = parser.parse_args()
    
    manifest_file = args.manifest or f"build/products/{args.product}/generated/omnis_manifest.json"
    if not os.path.exists(manifest_file):
        print(f"❌ FATAL: Manifest not found: {manifest_file}")
        sys.exit(1)

    print(f"🎬 Omnis Matrix Host starting for {args.product}...")
    try:
        orch = MatrixOrchestrator(manifest_file)
        MatrixAPIHandler.orch_instance = orch
        orch.provision_world()
        
        db_path = os.path.join(os.getcwd(), f"{args.product}.db")
        bootstrapper = MatrixEdgeC(bin_path="", vbus_path="", rest_port=0, db_path=db_path)
        bootstrapper.bootstrap_environment(orch.loader)
        
        print("\n🚀 Matrix is UP. Environment variables for EdgeOS:")
        for k, v in orch.get_env().items(): print(f"  export {k}={v}")
        
        server = HTTPServer(('0.0.0.0', 50051), MatrixAPIHandler)
        print("\n📡 Control Plane (REST) online at 50051")
        Thread(target=server.serve_forever, daemon=True).start()
        
        while True: time.sleep(1)
    except KeyboardInterrupt:
        orch.shutdown()
    except Exception as e:
        print(f"💥 Server Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
