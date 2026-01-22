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
        self.running = True

    def provision_world(self):
        """[Omnis] 建立数据交换机 (Data Switch)"""
        print(f"🌍 Omnis Provisioning: {self.loader.get_product_name()}...")
        
        for bus in self.loader.hardware:
            bus_id = bus["id"]
            env_path = os.environ.get("OMNIS_VBUS_PATH")
            sock_path = os.path.abspath(env_path if (bus_id == 0 and env_path) else self.loader.resolve_os_dev(bus_id, "simulation").replace("UDS:", ""))
            
            os.makedirs(os.path.dirname(sock_path), exist_ok=True)
            if os.path.exists(sock_path):
                try: os.unlink(sock_path)
                except: pass
            
            srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            srv.bind(sock_path)
            srv.listen(10)
            srv.setblocking(False)
            self._bus_srv_sockets[bus_id] = srv
            print(f"  [Bus {bus_id}] Data Switch ACTIVE at {sock_path}")
            
            t = threading.Thread(target=self._relay_loop, args=(bus_id, srv), daemon=True)
            t.start()
            self.bus_relays[bus_id] = t
            
            # 给中继器一点点时间就绪
            time.sleep(0.5)
            
            topology = self.loader.get_topology(bus_id)
            for dev in topology:
                self._launch_actor(sock_path, dev)

    def _relay_loop(self, bus_id, srv):
        """核心：全透明数据交换"""
        clients = []
        last_hb = time.time()
        while self.running:
            try:
                # 定期打心跳，证明中继器没死
                if time.time() - last_hb > 5:
                    # print(f" 💓 [Relay {bus_id}] Active. Client count: {len(clients)}")
                    last_hb = time.time()

                readable, _, _ = select.select([srv] + clients, [], [], 0.1)
                for s in readable:
                    if s is srv:
                        conn, _ = s.accept()
                        conn.setblocking(False)
                        clients.append(conn)
                        print(f"🔗 [Bus {bus_id}] New node joined. Total: {len(clients)}")
                    else:
                        try:
                            data = s.recv(4096)
                            if data:
                                # [埋点日志] 打印流量，一眼看出 Engine 有没有发包
                                # print(f"  📡 [Bus {bus_id}] Traffic: {data.hex().upper()}")
                                for c in clients:
                                    if c is not s:
                                        try: c.sendall(data)
                                        except: pass
                            else:
                                if s in clients: clients.remove(s)
                                s.close()
                                print(f"❌ [Bus {bus_id}] Node left. Total: {len(clients)}")
                        except:
                            if s in clients: clients.remove(s)
                            s.close()
            except Exception as e:
                if self.running: print(f"🔥 [Relay {bus_id}] Error: {e}")

    def _launch_actor(self, sock_path, dev_info):
        sim_cfg = dev_info.get("simulation")
        if sim_cfg is None: return # 注意：如果是 {} 也要继续

        actor_type = sim_cfg.get("actor", "modbus_actor")
        slave_id = dev_info.get("address") or 1
        init_data = sim_cfg.get("init", {})
        
        actor_name = actor_type.split('_')[0].lower()
        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        actor_script = os.path.join(base_dir, "actors", f"{actor_name}.py")
        
        if os.path.exists(actor_script):
            env = os.environ.copy()
            env["MATRIX_INIT_DATA"] = json.dumps(init_data)
            # [核心修复] 彻底解决 Pipe 阻塞：不再捕获 stdout，让它直接打在屏幕上
            proc = subprocess.Popen(
                ["python3", actor_script, "--socket", sock_path, "--id", str(slave_id), "--manifest", self.loader.manifest_path],
                env=env,
                stdout=None, 
                stderr=None
            )
            self.active_simulators.append(proc)
            print(f"    🚀 [Actor] {actor_name} ID:{slave_id} launched (PID:{proc.pid}).")

    def shutdown(self):
        self.running = False
        for proc in self.active_simulators:
            proc.terminate()
        for s in self._bus_srv_sockets.values():
            s.close()
