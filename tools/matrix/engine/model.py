import asyncio
import logging
from typing import Dict, Any, List, Optional, Callable
from .transport import BaseTransport

logging.basicConfig(level=logging.INFO)

logger = logging.getLogger("Matrix.Model")

class DataStore:
    """
    数字孪生核心存储：以 TagID 为 Key。
    支持值变更监听（Observer Pattern）和质量位（Quality Bits）。
    """
    def __init__(self):
        self._data: Dict[int, Any] = {}
        self._status: Dict[int, int] = {} # 质量位 (0=Good, 1=Bad, 2=Manual, etc.)
        self._observers: List[callable] = []

    def set(self, tag_id: int, value: Any, status: int = 0):
        old_value = self._data.get(tag_id)
        if old_value != value or self._status.get(tag_id) != status:
            self._data[tag_id] = value
            self._status[tag_id] = status
            for obs in self._observers:
                obs(tag_id, value, status)

    def get(self, tag_id: int, default=None) -> Any:
        return self._data.get(tag_id, default)

    def get_status(self, tag_id: int) -> int:
        return self._status.get(tag_id, 0)

    def add_observer(self, callback: callable):
        self._observers.append(callback)

class BaseVDevice:
    """
    虚拟设备基类。
    持有一个 DataStore 和对应的协议逻辑。
    """
    def __init__(self, name: str, address: Any):
        self.name = name
        self.address = address 
        self.datastore = DataStore()
        self.protocol = None 

    def inject(self, tag_name: str, value: Any, loader: 'SchemaLoader' = None):
        """语义化注入：直接设置业务值"""
        if loader and tag_name in loader.name_to_id:
            tag_id = loader.name_to_id[tag_name]
            self.datastore.set(tag_id, value)
            logger.info(f"[{self.name}] Injected {tag_name} ({tag_id}) -> {value}")
        else:
            # 如果没传 loader，尝试直接作为 TagID
            try:
                self.datastore.set(int(tag_name), value)
            except ValueError:
                logger.error(f"Unknown TagName: {tag_name}")

    async def receive_raw(self, raw_data: bytes) -> Optional[bytes]:
        """从总线接收原始报文并判定是否响应"""
        if not self.protocol:
            return None
        return await self.protocol.process(raw_data, self.datastore)

class VBus:
    """
    虚拟总线：模拟物理广播特性。
    """
    def __init__(self, bus_id: int, transport: BaseTransport):
        self.bus_id = bus_id
        self.devices: List[BaseVDevice] = []
        self.observers: List[Callable] = [] # 观察者列表
        self.transport = transport
        self.transport.on_received = self.broadcast

    def add_observer(self, callback: Callable[[str, bytes, Any], None]):
        """
        callback(direction, raw_data, metadata)
        """
        self.observers.append(callback)

    def attach_device(self, device: BaseVDevice):
        self.devices.append(device)
        logger.info(f"Attached device {device.name} to Bus {self.bus_id}")

    async def broadcast(self, raw_data: bytes) -> List[bytes]:
        """将数据广播给所有挂载设备，并收集响应"""
        # 记录下行流量 (EdgeC -> Matrix)
        for obs in self.observers:
            obs("TX", raw_data, {"bus": self.bus_id})

        responses = []
        for dev in self.devices:
            # Pass the correct datastore to the protocol's process method
            resp = await dev.protocol.process(raw_data, dev.datastore)
            if resp:
                # 记录上行流量 (Matrix -> EdgeC)
                for obs in self.observers:
                    obs("RX", resp, {"bus": self.bus_id, "device": dev.name})
                responses.append(resp)
        return responses

    async def start(self):
        await self.transport.start()
