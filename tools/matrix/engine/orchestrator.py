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

    def get_device(self, name: str) -> Optional[BaseVDevice]:
        return self.devices.get(name)

    def build_from_world(self):
        """
        根据 world_config 自动构建拓扑 (Sprint 4 特性)
        """
        if not self.loader.world_config:
            logger.warning("No world_config found, building empty topology.")
            return

        topology = self.loader.world_config.get("topology", [])
        for bus_item in topology:
            bus_id = bus_item.get("bus_id", 0)
            transport_cfg = bus_item.get("transport", {"type": "uds"})
            
            if transport_cfg["type"] == "uds":
                path = transport_cfg.get("path", f"/tmp/vbus{bus_id}.sock")
                transport = UDSTransport(path)
            else:
                continue

            if bus_id not in self.buses:
                self.buses[bus_id] = VBus(bus_id, transport)
                self.buses[bus_id].add_observer(self.sniffer.on_traffic)

            for dev_item in bus_item.get("devices", []):
                proto_name = dev_item.get("protocol")
                count = dev_item.get("count", 1)
                start_addr = dev_item.get("start_address", 1)

                for i in range(count):
                    addr = start_addr + i
                    dev_name = f"Bus{bus_id}_{proto_name}_{addr}"
                    
                    # 关键：从 loader 提取该协议的所有绑定点
                    binds = {}
                    if proto_name == "ModbusRTU":
                        for tid, bind in self.loader.modbus_map.items():
                            # 只要协议名匹配，就挂载到虚拟设备上
                            binds[tid] = bind.get("attributes") or bind.get("Attributes") or bind

                    device = BaseVDevice(dev_name, addr)
                    if proto_name == "ModbusRTU":
                        device.protocol = ModbusRTUProtocol(addr, binds)
                    
                    self.buses[bus_id].attach_device(device)
                    self.devices[dev_name] = device
                    logger.info(f"Auto-Initialized Device: {dev_name} (Address: {addr}) with {len(binds)} tags.")

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