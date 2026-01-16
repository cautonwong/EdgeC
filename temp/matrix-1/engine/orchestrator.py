import logging
import asyncio
import json
import os
import requests
from typing import Dict, Optional, Any
from .loader import SchemaLoader
from .model import VBus, BaseVDevice
from .transport import UDSTransport, TCPTransport
from ..actors.modbus import ModbusRTUProtocol
from ..observer.sniffer import TrafficSniffer

logger = logging.getLogger("Matrix.Orchestrator")

class MatrixOrchestrator:
    def __init__(self, schema_path: str, log_dir: str = "build/matrix_logs"):
        os.makedirs(log_dir, exist_ok=True)
        self.loader = SchemaLoader(schema_path)
        self.buses: Dict[int, VBus] = {}
        self.devices: Dict[str, BaseVDevice] = {}
        self.sniffer = TrafficSniffer(log_dir)
        self._interactive_server = None
        self.edge_url = "http://127.0.0.1:9091"

    def build_topology(self, bus_paths: dict):
        """
        根据契约动态构建拓扑。
        """
        device_groups = {}
        # 支持多协议提取
        modbus_items = self.loader.modbus_map.items()
        
        logger.info(f"Building topology. Modbus map size: {len(modbus_items)}")
        
        for tid, bind in modbus_items:
            # 兼容性提取：如果没有 bus_id，默认为 0
            bus_id = bind.get("Attributes", {}).get("bus_id", 0)
            slave_id = bind.get("Attributes", {}).get("slave_id", 1)
            
            key = (bus_id, "ModbusRTU", slave_id)
            if key not in device_groups: device_groups[key] = {}
            device_groups[key][tid] = bind

        logger.info(f"Grouped into {len(device_groups)} virtual devices.")

        for (bus_id, proto_type, addr), binds in device_groups.items():
            if bus_id not in self.buses:
                path = bus_paths.get(bus_id, f"/tmp/vbus{bus_id}.sock")
                logger.info(f"Creating VBus {bus_id} on transport {path}")
                transport = UDSTransport(path)
                self.buses[bus_id] = VBus(bus_id, transport)
                self.buses[bus_id].add_observer(self.sniffer.on_traffic)
            
            dev_name = f"Bus{bus_id}_{proto_type}_{addr}"
            device = BaseVDevice(dev_name, addr)
            device.protocol = ModbusRTUProtocol(addr, binds)
            self.buses[bus_id].attach_device(device)
            self.devices[dev_name] = device
            logger.info(f"Initialized Device: {dev_name} with {len(binds)} tag bindings.")

    def get_device(self, name: str) -> Optional[BaseVDevice]:
        return self.devices.get(name)

    def build_from_world(self):
        """
        根据 world_config (来自 extension.yaml) 构建仿真拓扑。
        这允许仿真环境拥有与契约定义不同的拓扑（例如大规模压测）。
        """
        topology = self.loader.world_config.get("topology", [])
        if not topology:
            logger.warning("No topology found in world_config, falling back to contract-based build.")
            self.build_topology({})
            return

        for bus_cfg in topology:
            bus_id = bus_cfg.get("bus_id", 0)
            transport_cfg = bus_cfg.get("transport", {"type": "uds", "path": f"/tmp/vbus{bus_id}.sock"})
            
            if bus_id not in self.buses:
                if transport_cfg["type"] == "uds":
                    transport = UDSTransport(transport_cfg["path"])
                else:
                    transport = TCPTransport(transport_cfg.get("host", "0.0.0.0"), transport_cfg.get("port", 502))
                
                self.buses[bus_id] = VBus(bus_id, transport)
                self.buses[bus_id].add_observer(self.sniffer.on_traffic)

            for dev_cfg in bus_cfg.get("devices", []):
                proto_name = dev_cfg.get("protocol", "Modbus")
                count = dev_cfg.get("count", 1)
                start_addr = dev_cfg.get("start_address", 1)

                for i in range(count):
                    addr = start_addr + i
                    dev_name = f"Bus{bus_id}_{proto_name}_{addr}"
                    
                    device = BaseVDevice(dev_name, addr)
                    # 从契约中提取该协议的所有绑定信息
                    if proto_name == "Modbus":
                        device.protocol = ModbusRTUProtocol(addr, self.loader.modbus_map)
                    
                    self.buses[bus_id].attach_device(device)
                    self.devices[dev_name] = device

    async def start_interactive_api(self, port: int = 9999):
        try:
            # 关键修复：允许端口重用，避免 TIME_WAIT 导致的启动失败
            server = await asyncio.start_server(self._handle_interactive, '0.0.0.0', port, reuse_address=True)
            self._interactive_server = server
            logger.info(f"Interactive API enabled on 0.0.0.0:{port}")
        except Exception as e:
            logger.error(f"Failed to start Interactive API: {e}")
            raise

    async def _handle_interactive(self, reader, writer):
        peer = writer.get_extra_info('peername')
        logger.info(f"New connection: {peer}")
        
        while True:
            try:
                data = await reader.read(1024)
                if not data: break
                
                cmd_line = data.decode().strip()
                if not cmd_line: continue
                
                parts = cmd_line.split()
                cmd = parts[0].upper()
                
                if cmd == "SET" and len(parts) >= 4:
                    dev_name, tag_ref, val = parts[1], parts[2], parts[3]
                    device = self.devices.get(dev_name)
                    if device:
                        try:
                            tag_id = int(tag_ref)
                        except: tag_id = tag_ref
                        device.inject(tag_id, float(val), self.loader)
                        writer.write(f"OK: Set {tag_ref}={val}\n".encode())
                    else:
                        writer.write(f"ERR: Device {dev_name} unknown\n".encode())
                
                elif cmd == "MONITOR" and len(parts) >= 2:
                    tag_id = parts[1]
                    try:
                        # 内部穿透查询 EdgeOS 的 DataHub
                        resp = requests.get(f"{self.edge_url}/api/v1/data/latest/{tag_id}", timeout=0.5)
                        writer.write((resp.text + "\n").encode())
                    except Exception as e:
                        writer.write(f"ERR: EdgeOS unreachable: {e}\n".encode())

                elif cmd == "LIST":
                    writer.write((json.dumps({"devices": list(self.devices.keys())}) + "\n").encode())
                
                elif cmd == "STATUS" and len(parts) >= 2:
                    dev_name = parts[1]
                    device = self.devices.get(dev_name)
                    if device:
                        writer.write((json.dumps({"device": dev_name, "datastore": device.protocol.datastore}) + "\n").encode())
                
                await writer.drain()
            except Exception as e:
                logger.error(f"API Error: {e}")
                break
        writer.close()

    async def run(self):
        tasks = [bus.start() for bus in self.buses.values()]
        if self._interactive_server:
            # 使用 asyncio.Future 保持运行而不阻塞其它任务
            await asyncio.gather(*tasks)
        else:
            await asyncio.gather(*tasks)