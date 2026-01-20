import os
import subprocess
import time
from .loader import WorldLoader
from .edgec import MatrixEdgeC

class MatrixOrchestrator:
    def __init__(self, product: str, world_file: str):
        self.product = product
        self.loader = WorldLoader(world_file)
        self.instance_env = {}
        self.active_simulators = []
        self.actor_registry = {} 
        self.active_buses = set() # 记录已启动服务的总线

    def provision_world(self, instance_id: str):
        buses = self.loader.get_instance_buses(instance_id)
        print(f"🌍 Provisioning physical world for {instance_id}...")

        for bus in buses:
            bus_type = bus.get('type')
            os_dev = self.loader.resolve_os_dev(bus.get('os_dev'))

            if bus_type == "SERIAL" and "vbus" in os_dev:
                print(f"  [Bus] Virtual Serial Bus at {os_dev}")
                self.instance_env["OMNIS_VBUS_PATH"] = os_dev
                
                # 关键修复：每个 Socket 只启动一个 Actor
                if os_dev not in self.active_buses:
                    # 获取该总线上第一个设备的地址作为主模拟对象
                    topo = bus.get('topology', [])
                    slave_id = topo[0].get('address', 1) if topo else 1
                    self._launch_modbus_actor(os_dev, slave_id)
                    self.active_buses.add(os_dev)

            elif bus_type == "MEMORY_BUS":
                # 模拟高频采样总线
                print(f"  [Bus] Preparing Memory Bus (memfd) for ADC")
                adc_sock = "/tmp/v_adc_sim.sock"
                self.instance_env["OMNIS_ADC_PATH"] = adc_sock
                self._launch_adc_simulator(adc_sock)

    def _launch_modbus_actor(self, sock_path, slave_id):
        actor_path = "tools/matrix/actors/modbus.py"
        if os.path.exists(actor_path):
            proc = subprocess.Popen(["python3", actor_path, "--socket", sock_path, "--id", str(slave_id)])
            self.active_simulators.append(proc)
            print(f"  [Actor] Modbus Slave {slave_id} launched at {sock_path}")

    def _launch_adc_simulator(self, sock_path):
        """
        启动 ADC 模拟器进程
        """
        sim_path = "tools/matrix/simulators/adc_simulator.py"
        if os.path.exists(sim_path):
            proc = subprocess.Popen(["python3", sim_path, "--socket", sock_path])
            self.active_simulators.append(proc)
            print(f"  [Sim] ADC Simulator launched (PID: {proc.pid})")

    def get_env(self):
        return self.instance_env

    def shutdown(self):
        print("🛑 Shutting down Matrix environment...")
        for proc in self.active_simulators:
            proc.terminate()
