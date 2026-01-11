import socket
import os
import time
import sys
import threading
import json
import asyncio
from engine.parser import ContractParser

# 引入专家库
from pymodbus.datastore import ModbusDeviceContext, ModbusServerContext
from pymodbus.datastore import ModbusSequentialDataBlock

class ModbusOracleActor:
    """
    基于 Pymodbus 專家库的 Modbus Mimic Actor.
    支持標準協議行为，并提供 UDS 控制接口。
    """
    def __init__(self):
        product = os.environ.get("OMNIS_PRODUCT", "iems")
        gen_yaml = os.environ.get("OMNIS_CONTRACT_PATH")
        if not gen_yaml:
            gen_yaml = f"build/products/{product}/generated/contracts.yaml"
        
        self.parser = ContractParser(gen_yaml)
        self.vbus_path = os.environ.get("OMNIS_VBUS_PATH", "/tmp/vbus0.sock")
        self.ctrl_path = os.environ.get("OMNIS_ACTOR_CTRL", f"/tmp/mimic_modbus_{os.getpid()}.ctrl")
        
        # 1. 初始化 Pymodbus 数据存储 (寄存器空间)
        # 我们使用 0-65535 的顺序数据块
        self.store = ModbusDeviceContext(
            di=ModbusSequentialDataBlock(0, [0]*100),
            co=ModbusSequentialDataBlock(0, [0]*100),
            hr=ModbusSequentialDataBlock(0, [0]*1000), # Holding Registers
            ir=ModbusSequentialDataBlock(0, [0]*100)
        )
        self.context = ModbusServerContext(devices=self.store, single=True)
        
        self.tag_to_reg = {}
        self._setup_mappings()

    def _setup_mappings(self):
        for name, tag in self.parser.get_all_tags().items():
            bindings = tag.get('bindings', [])
            for b in bindings:
                if b.get('protocol') == 'MODBUS':
                    attr = b.get('attributes', {})
                    reg_addr = attr.get('addr')
                    if reg_addr is not None:
                        self.tag_to_reg[name] = reg_addr
                        # 设置初始值 (比如电压 230V)
                        val = 2300 if 'VOLTAGE' in name else 0
                        self.store.setValues(3, reg_addr, [int(val)])
                        print(f"🎬 [OracleActor] Mapping Tag {name} -> Reg {reg_addr} (Initial: {val})")
        sys.stdout.flush()

    async def _run_control_server(self):
        """处理来自 Director 的控制指令 (如注入数据)"""
        if os.path.exists(self.ctrl_path): os.remove(self.ctrl_path)
        server = await asyncio.start_unix_server(self._handle_control, self.ctrl_path)
        print(f"🎮 [OracleActor] Control link active at {self.ctrl_path}")
        async with server:
            await server.serve_forever()

    async def _handle_control(self, reader, writer):
        data = await reader.read(1024)
        if data:
            try:
                cmd = json.loads(data.decode())
                if cmd.get("action") == "set":
                    tag_name = cmd.get("tag")
                    value = float(cmd.get("value"))
                    if tag_name in self.tag_to_reg:
                        reg = self.tag_to_reg[tag_name]
                        # 应用简单的 0.1 缩放 (演习约定)
                        raw_val = int(value * 10) if 'VOLTAGE' in tag_name else int(value)
                        self.store.setValues(3, reg, [raw_val])
                        print(f"⚡ [OracleActor] Injected {tag_name} (Reg {reg}) -> {value} (Raw: {raw_val})")
            except Exception as e:
                print(f"❌ [OracleActor] Control Error: {e}")
        writer.close()
        await writer.wait_closed()

    async def _run_vbus_bridge(self):
        """连接 C 端的 vbus 并通过 Pymodbus 处理报文"""
        while not os.path.exists(self.vbus_path):
            await asyncio.sleep(0.5)

        print(f"✅ [OracleActor] Attempting to connect to EdgeC Bus: {self.vbus_path}")
        while True:
            try:
                reader, writer = await asyncio.open_unix_connection(self.vbus_path)
                print(f"✅ [OracleActor] Connected to EdgeC Bus")
                
                while True:
                    # 读取 Modbus RTU 报文
                    data = await reader.read(1024)
                    if not data: break
                    
                    # 使用 Pymodbus 的 FDU 处理器处理原始数据
                    # 为简化，我们直接手动调用 context 进行响应生成，
                    # 这样我们可以精确模拟 RTU Over Socket 的行为
                    response = self._process_modbus_request(data)
                    if response:
                        writer.write(response)
                        await writer.drain()
            except Exception as e:
                print(f"❌ [OracleActor] Bus Link Error: {e}")
                await asyncio.sleep(1)

    def _process_modbus_request(self, data):
        """
        极简 Modbus RTU 处理器。
        在真实场景中，我们会使用 pymodbus.factory.Decoder。
        """
        if len(data) < 4: return None
        slave_id = data[0]
        func_code = data[1]
        
        # Pymodbus 内部處理 (3=HR, 4=IR, 1=CO, 2=DI)
        try:
            if func_code == 0x03:
                addr = (data[2] << 8) | data[3]
                count = (data[4] << 8) | data[5]
                values = self.store.getValues(3, addr, count)
                
                payload = bytearray([slave_id, 0x03, len(values)*2])
                for v in values:
                    payload.extend([(v >> 8) & 0xFF, v & 0xFF])
                return payload + self._calc_crc(payload)
            
            elif func_code == 0x06:
                addr = (data[2] << 8) | data[3]
                value = (data[4] << 8) | data[5]
                self.store.setValues(3, addr, [value])
                print(f"📥 [OracleActor] Remote WRITE: Reg {addr} -> {value}")
                return data # Echo back
                
        except Exception as e:
            print(f"❌ [OracleActor] Protocol Error: {e}")
        return None

    def _calc_crc(self, data):
        crc = 0xFFFF
        for pos in data:
            crc ^= pos
            for i in range(8):
                if (crc & 1) != 0:
                    crc >>= 1
                    crc ^= 0xA001
                else:
                    crc >>= 1
        import struct
        return struct.pack('<H', crc)

    async def start(self):
        await asyncio.gather(
            self._run_control_server(),
            self._run_vbus_bridge()
        )

if __name__ == "__main__":
    actor = ModbusOracleActor()
    try:
        asyncio.run(actor.start())
    except KeyboardInterrupt:
        pass
