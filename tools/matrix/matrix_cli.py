#!/usr/bin/env python3
import cmd
import requests
import sys
import json
import os
import argparse
from tools.matrix.engine.loader import SchemaLoader

class MatrixShell(cmd.Cmd):
    intro = '\033[94m' + '='*60 + '\n' + \
            '🚀 Omnis Matrix Workstation - Interactive Control Center\n' + \
            'Type help or ? to list commands. Tab-completion enabled.\n' + \
            '='*60 + '\033[0m'
    prompt = '(omnis) '
    
    def __init__(self, host='localhost', port=50051, product='iems'):
        super().__init__()
        self.base_url = f"http://{host}:{port}/api/v1"
        self.product = product
        
        # 加载契约元数据用于补全
        project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../"))
        manifest_path = os.environ.get("OMNIS_MANIFEST")
        if not manifest_path:
            for c in [
                os.path.join(project_root, f"build/products/{product}/generated/omnis_manifest.json"),
                os.path.join(project_root, f"cmd/{product}/omnis_manifest.json"),
                os.path.join(project_root, f"cmd/{product}/matrix/omnis_manifest.json"),
            ]:
                if os.path.exists(c):
                    manifest_path = c
                    break
        if not manifest_path:
            manifest_path = os.path.join(project_root, f"build/products/{product}/generated/omnis_manifest.json")
        try:
            self.loader = SchemaLoader(manifest_path)
            self.device_ids = [d["id"] for d in self.loader.devices]
            self.tag_names = list(self.loader.name_to_id.keys())
        except Exception as e:
            print(f"\033[33mWarning: Could not load manifest for completion: {e}\033[0m")
            self.device_ids = []
            self.tag_names = []

    def do_ls(self, arg):
        """List resources: ls devices | ls buses | ls tags"""
        if arg == "devices":
            resp = requests.get(f"{self.base_url}/devices")
            if resp.status_code == 200:
                print("\033[1mSimulated Devices (Logical IDs):\033[0m")
                for d in resp.json()["devices"]:
                    print(f"  • {d}")
        elif arg == "buses":
            resp = requests.get(f"{self.base_url}/connectivity")
            if resp.status_code == 200:
                print("\033[1mInfrastructure Channels:\033[0m")
                for b in resp.json()["buses"]:
                    print(f"  • [{b['id']}] {b['label']}")
        elif arg == "tags":
            print("\033[1mContract Tags (Available for injection):\033[0m")
            for name in sorted(self.tag_names):
                tag_id = self.loader.name_to_id[name]
                print(f"  {tag_id:<6} : {name}")
        else:
            print("Usage: ls devices | ls buses | ls tags")

    # --- Set Command with Completion ---
    def do_set(self, arg):
        """SET <DeviceID> <Tag> <Value>: Inject value into simulated device."""
        try:
            parts = arg.split()
            if len(parts) < 3:
                print("Usage: set <DeviceID> <Tag> <Value>")
                return
            
            dev_id, tag, val = parts[0], parts[1], parts[2]
            # 允许使用 Tag Name 或 ID
            payload = {"dev_id": dev_id, "tag": tag, "value": float(val)}
            resp = requests.post(f"{self.base_url}/actor/set", json=payload)
            
            if resp.status_code == 200:
                print(f"✅ Success: {dev_id}.{tag} -> {val}")
            else:
                print(f"❌ Error: {resp.text}")
        except Exception as e:
            print(f"Error: {e}")

    def complete_set(self, text, line, begidx, endidx):
        parts = line.split()
        # 补全第一个参数: DeviceID
        if len(parts) == 1 or (len(parts) == 2 and not line.endswith(' ')):
            return [d for d in self.device_ids if d.startswith(text)]
        # 补全第二个参数: TagName
        if len(parts) == 2 or (len(parts) == 3 and not line.endswith(' ')):
            return [t for t in self.tag_names if t.startswith(text)]
        return []

    def do_inspect(self, arg):
        """INSPECT <DeviceID>: Show contract bindings for a device."""
        if not arg:
            print("Usage: inspect <DeviceID>")
            return
        
        target_dev = None
        for d in self.loader.devices:
            if d["id"] == arg:
                target_dev = d
                break
        
        if not target_dev:
            print(f"Device '{arg}' not found in manifest.")
            return

        print(f"\033[1mDevice: {arg} (Model: {target_dev['model']}, Proto: {target_dev['protocol']})\033[0m")
        print(f"{'TAG':<25} {'BINDING':<15}")
        print("-" * 45)
        
        # 查找匹配该设备协议的所有测点
        proto = target_dev["protocol"].upper()
        if "MODBUS" in proto: proto = "MODBUS"
        
        for tag in self.loader.tags:
            for b in tag.get("bindings", []):
                if b["protocol"].upper() == proto:
                    attr = b["attributes"]
                    binding = f"Addr:{attr.get('addr')}" if "addr" in attr else f"DI:{attr.get('di')}"
                    print(f"{tag['name']:<25} {binding:<15}")

    def do_exit(self, arg):
        """Exit the workstation."""
        print("Goodbye.")
        return True

    def do_EOF(self, arg):
        print()
        return True

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=50051)
    parser.add_argument("--product", default="iems")
    parser.add_argument("command", nargs='*', help="Run a single command and exit")
    args = parser.parse_args()

    shell = MatrixShell(args.host, args.port, args.product)
    
    if args.command:
        # 非交互模式: ./matrix_cli.py set METER_MAIN GRID_POWER 1000
        shell.onecmd(" ".join(args.command))
    else:
        # 交互模式
        shell.cmdloop()