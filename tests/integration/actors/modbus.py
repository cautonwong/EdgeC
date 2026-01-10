import socket
import os
import time
import struct
import sys
import threading
import json
from tests.integration.engine.parser import ContractParser

class ModbusMimic:
    def __init__(self, product="iems"):
        # 根据环境变量或默认值确定产品和 UDS 路径
        self.product = os.environ.get("OMNIS_PRODUCT", product)
        gen_yaml = f"build/products/{self.product}/generated/contracts.yaml"
        self.parser = ContractParser(gen_yaml)
        
        # 演习总线 UDS
        self.vbus_path = "/tmp/vbus0.sock"
        
        # 控制服务器 UDS
        self.ctrl_path = f"/tmp/mimic_modbus_{os.getpid()}.ctrl"
        
        self.registers = {}
        self.tag_to_reg = {}
        
        for name, tag in self.parser.get_all_tags().items():
            # 兼容嵌套的 bindings 结构
            bindings = tag.get('bindings', [])
            for b in bindings:
                if b.get('protocol') == 'MODBUS':
                    attr = b.get('attributes', {})
                    reg_addr = attr.get('addr')
                    if reg_addr is not None:
                        self.registers[reg_addr] = 230.0 if 'VOLTAGE' in name else 0.0
                        self.tag_to_reg[name] = reg_addr
                        print(f"🎬 [Mimic] Mapping Tag {name} to Register {reg_addr}")
        
        sys.stdout.flush()

    def _run_control_server(self):
        """后台控制线程：接收来自导演部的指令"""
        if os.path.exists(self.ctrl_path): os.remove(self.ctrl_path)
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server.bind(self.ctrl_path)
        server.listen(1)
        print(f"🎮 [Mimic] Control neural link active at {self.ctrl_path}")
        sys.stdout.flush()

        while True:
            conn, _ = server.accept()
            try:
                data = conn.recv(1024)
                if not data: continue
                cmd = json.loads(data.decode())
                # 指令格式: {"action": "set", "tag": "VOLTAGE_PHASE_A", "value": 275.0}
                if cmd.get("action") == "set":
                    tag_name = cmd.get("tag")
                    value = cmd.get("value")
                    if tag_name in self.tag_to_reg:
                        reg = self.tag_to_reg[tag_name]
                        self.registers[reg] = float(value)
                        print(f"⚡ [Mimic] Set {tag_name} (Reg {reg}) -> {value}")
                        sys.stdout.flush()
            except Exception as e:
                print(f"❌ [Mimic] Control error: {e}")
            finally:
                conn.close()

    def calculate_crc(self, data):
        crc = 0xFFFF
        for pos in data:
            crc ^= pos
            for i in range(8):
                if (crc & 1) != 0:
                    crc >>= 1
                    crc ^= 0xA001
                else:
                    crc >>= 1
        return struct.pack('<H', crc)

    def run(self):
        # 启动控制神经
        threading.Thread(target=self._run_control_server, daemon=True).start()

        print(f"🔄 [Mimic] Waiting for EdgeC bus at {self.vbus_path}...")
        sys.stdout.flush()
        
        while not os.path.exists(self.vbus_path):
            time.sleep(0.5)

        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            client.connect(self.vbus_path)
            print(f"✅ [Mimic] Connected to EdgeC Bus")
            sys.stdout.flush()
            
            while True:
                data = client.recv(1024)
                if not data: break
                
                print(f"📡 [Mimic] RAW RECV ({len(data)} bytes): {data.hex().upper()}")
                sys.stdout.flush()
                
                # 极简 Modbus 抢答逻辑 (仅支持 03 读寄存器)
                if len(data) >= 8 and data[1] == 0x03:
                    slave_id = data[0]
                    start_reg = struct.unpack('>H', data[2:4])[0]
                    reg_count = struct.unpack('>H', data[4:6])[0]
                    
                    byte_count = reg_count * 2
                    payload = bytearray([slave_id, 0x03, byte_count])
                    for i in range(reg_count):
                        # 获取实时值并应用缩放 (0.1V -> *10)
                        val = int(self.registers.get(start_reg + i, 0) * 10)
                        payload.extend(struct.pack('>H', val))
                    
                    payload.extend(self.calculate_crc(payload))
                    print(f"📡 [Mimic] SENDING RESP: {payload.hex().upper()}")
                    sys.stdout.flush()
                    client.send(payload)
        except Exception as e:
            print(f"❌ [Mimic] Bus error: {e}")
        finally:
            print("🛑 [Mimic] Exiting.")
            sys.stdout.flush()
            client.close()
            if os.path.exists(self.ctrl_path): os.remove(self.ctrl_path)

if __name__ == "__main__":
    mimic = ModbusMimic()
    mimic.run()
