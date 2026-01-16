import logging
import struct
from typing import Optional
from ..engine.model import DataStore

logger = logging.getLogger("Matrix.Modbus")

class ModbusRTUProtocol:
    def __init__(self, slave_id: int, bindings: dict):
        self.slave_id = slave_id
        self.bindings = bindings
        self._tag_to_addr = {}
        self._addr_to_tag = {}
        self._setup_mappings()

    def _setup_mappings(self):
        for tag_id, bind in self.bindings.items():
            # 兼容嵌套结构
            attrs = bind.get("Attributes") or bind.get("attributes") or bind
            addr = attrs.get("Addr") or attrs.get("addr")
            if addr is not None:
                if isinstance(addr, str) and addr.startswith("0x"):
                    addr = int(addr, 16)
                elif isinstance(addr, str):
                    addr = int(addr)
                
                scaler = attrs.get("Scaler") or attrs.get("scaler") or attrs.get("Scale") or attrs.get("scale") or 1.0
                
                self._tag_to_addr[tag_id] = {"addr": addr, "scaler": scaler}
                self._addr_to_tag[addr] = {"tag_id": tag_id, "scaler": scaler}

        logger.info(f"Modbus Mappings for Slave {self.slave_id}:")
        logger.info(f"  Bindings: {self.bindings}")
        logger.info(f"  Tag to Addr: {self._tag_to_addr}")
        logger.info(f"  Addr to Tag: {self._addr_to_tag}")

    def _compute_crc(self, data: bytes) -> bytes:
        crc = 0xFFFF
        for pos in data:
            crc ^= pos
            for _ in range(8):
                if (crc & 1) != 0:
                    crc >>= 1
                    crc ^= 0xA001
                else:
                    crc >>= 1
        return struct.pack('<H', crc)

    async def process(self, raw_data: bytes, datastore: DataStore) -> Optional[bytes]:
        if not raw_data: return None
        
        # 物理层：站号过滤
        unit_id = raw_data[0]
        if unit_id != self.slave_id and unit_id != 0:
            return None

        # 仅处理 Read Holding Registers (03)
        if len(raw_data) >= 8 and raw_data[1] == 0x03:
            start_addr = struct.unpack('>H', raw_data[2:4])[0]
            count = struct.unpack('>H', raw_data[4:6])[0]
            
            logger.info(f"Datastore content: {datastore._data}")
            payload = bytearray()
            for i in range(count):
                addr = start_addr + i
                map_entry = self._addr_to_tag.get(addr) # Get the entire map entry
                
                val = 0
                scaler = 1.0
                tid = None
                
                if map_entry:
                    tid = map_entry["tag_id"]
                    scaler = map_entry["scaler"]
                    val = datastore.get(tid, 0)
                
                logger.info(f"Modbus Process: tid={tid}, val={val}, scaler={scaler}, val/scaler={val/scaler if scaler else 'NaN'}")
                raw_val = int(round(val / scaler))
                logger.info(f"Modbus Process: raw_val={raw_val}")
                payload.extend(struct.pack('>H', raw_val & 0xFFFF))
            
            res = bytearray([self.slave_id, 0x03, len(payload)])
            res.extend(payload)
            res.extend(self._compute_crc(res))
            logger.info(f"📤 [Matrix] Device {self.slave_id} responding: {res.hex(' ')}")
            return bytes(res)
            
        return None
