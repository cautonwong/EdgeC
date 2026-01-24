import os
import subprocess
import time
import json
import socket
import select
import threading
import sys
from .loader import SchemaLoader

class MatrixOrchestrator:
    def __init__(self, manifest_path: str):
        self.loader = SchemaLoader(manifest_path)
        self.active_simulators = []
        self.bus_relays = {} 
        self._bus_srv_sockets = {}
        self.instance_env = {}
        self.running = True

    def provision_world(self):
        """[Omnis] 按照 Standard 5.0 建立物理世界基础设施"""
        print(f"🌍 Omnis Provisioning: {self.loader.get_product_name()}...")
        
        # 1. 第一步：铺路 (Connectivity)
        for conn in self.loader.connectivity:
            bus_id = conn["id"]
            sim_cfg = conn.get("simulation", {})
            endpoint = sim_cfg.get("endpoint", "")
            
            if endpoint.startswith("UDS:"):
                sock_path = os.path.abspath(endpoint.replace("UDS:", ""))
                self._setup_uds_relay(bus_id, sock_path)
                # 记录环境变量，供 Task 3.2 对接
                self.instance_env[f"OMNIS_BUS_{bus_id}_DEV"] = sock_path
            
            elif sim_cfg.get("transport") == "MEMFD":
                # 对于 memfd，我们记录控制套接字路径
                ctrl_sock = os.path.abspath(sim_cfg.get("control_sock", "/tmp/v_adc.sock"))
                self.instance_env[f"OMNIS_BUS_{bus_id}_DEV"] = ctrl_sock
                self._launch_adc_simulator(ctrl_sock)

        # 2. 第二步：放车 (Devices)
        for dev in self.loader.devices:
            bus_id = dev["bus_id"]
            env_key = f"OMNIS_BUS_{bus_id}_DEV"
            if env_key in self.instance_env:
                sock_path = self.instance_env[env_key]
                self._launch_actor(sock_path, dev)

    def _setup_uds_relay(self, bus_id, sock_path):
        """创建虚拟总线中继器"""
        os.makedirs(os.path.dirname(sock_path), exist_ok=True)
        if os.path.exists(sock_path):
            try: os.unlink(sock_path)
            except: pass
        
        srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        srv.bind(sock_path)
        srv.listen(10)
        srv.setblocking(False)
        self._bus_srv_sockets[bus_id] = srv
        print(f"  [Bus {bus_id}] Virtual Backplane ACTIVE at {sock_path}")
        
        t = threading.Thread(target=self._relay_loop, args=(bus_id, srv), daemon=True)
        t.start()
        self.bus_relays[bus_id] = t

    def _relay_loop(self, bus_id, srv):
        clients = []
        while self.running:
            try:
                readable, _, _ = select.select([srv] + clients, [], [], 0.1)
                for s in readable:
                    if s is srv:
                        conn, _ = s.accept()
                        conn.setblocking(False)
                        clients.append(conn)
                        print(f"  [Relay] Bus {bus_id}: New connection (FD:{conn.fileno()}). Total: {len(clients)}")
                    else:
                        try:
                            data = s.recv(4096)
                            if data:
                                # [Debug Log] 打印转发详情
                                # hex_data = data.hex(' ').upper()
                                # print(f"  [Relay] Bus {bus_id}: FD {s.fileno()} -> {len(data)} bytes")
                                
                                for c in clients:
                                    if c is not s:
                                        try:
                                            c.sendall(data)
                                        except:
                                            if c in clients: clients.remove(c)
                            else:
                                print(f"  [Relay] Bus {bus_id}: FD {s.fileno()} closed.")
                                if s in clients: clients.remove(s)
                                s.close()
                        except Exception as e:
                            print(f"  [Relay] Bus {bus_id}: FD {s.fileno()} error: {e}")
                            if s in clients: clients.remove(s)
                            s.close()
            except: pass

    def _launch_actor(self, sock_path, dev_info):
        sim_cfg = dev_info.get("simulation") or {}
        actor_type = sim_cfg.get("actor", "modbus_actor")
        slave_id = dev_info.get("address") or 1
        init_data = sim_cfg.get("init", {})
        
        # 名字兼容性处理
        actor_name = actor_type.split('_')[0].lower()
        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        actor_script = os.path.join(base_dir, "actors", f"{actor_name}.py")
        print(f"    🔍 Checking Actor Script: {actor_script}")
        
        if os.path.exists(actor_script):
            env = os.environ.copy()
            env["MATRIX_INIT_DATA"] = json.dumps(init_data)
            # 使用 sys.executable 确保与 Server 使用相同的 Python 解释器 (.venv)
            proc = subprocess.Popen(
                [sys.executable, actor_script, "--socket", sock_path, "--id", str(slave_id), 
                 "--dev_id", dev_info["id"], "--manifest", self.loader.manifest_path],
                env=env, stdout=sys.stdout, stderr=sys.stderr, cwd=os.getcwd()
            )
            self.active_simulators.append(proc)
            print(f"    🚀 [Actor] {actor_name} (ID:{slave_id}) attached to Bus at {sock_path}")

    def _launch_adc_simulator(self, sock_path):
        """启动高频采样模拟器"""
        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        sim_script = os.path.join(base_dir, "simulators", "adc_simulator.py")
        if os.path.exists(sim_script):
            proc = subprocess.Popen(["python3", sim_script, "--socket", sock_path])
            self.active_simulators.append(proc)
            print(f"    🚀 [Sim] ADC Waveform Generator launched at {sock_path}")

    def update_device_tag(self, dev_id: str, tag_name: str, value: float) -> bool:
        """[Omnis] 通过逻辑主键 ID 更新设备仿真数据"""
        # 验证设备是否存在
        found = False
        for dev in self.loader.devices:
            if dev["id"] == dev_id:
                found = True
                break
        
        if not found:
            print(f"  ⚠️  Unknown device: {dev_id}")
            return False

        print(f"🎮 Command: Set {dev_id}.{tag_name} = {value}")
        # 写入控制文件给 Actor 消费
        # [补强] 使用追加模式，防止多点位同时注入时覆盖
        ctrl_file = f"/tmp/matrix_ctrl_{dev_id}.tmp"
        with open(ctrl_file, "a") as f:
            f.write(f"{tag_name}:{value}\n")
        return True

    def get_env(self):
        return self.instance_env

    def shutdown(self):
        self.running = False
        for proc in self.active_simulators:
            proc.terminate()
        for s in self._bus_srv_sockets.values():
            s.close()