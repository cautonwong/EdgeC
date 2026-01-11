import socket
import os
import time
import struct
import sys
import threading
import json
from engine.parser import ContractParser

class ModbusMimic:
    def __init__(self):
        product = os.environ.get("OMNIS_PRODUCT", "iems")
        gen_yaml = os.environ.get("OMNIS_CONTRACT_PATH")
        if not gen_yaml:
            gen_yaml = f"build/products/{product}/generated/contracts.yaml"
        
        self.parser = ContractParser(gen_yaml)
        self.vbus_path = "/tmp/vbus0.sock"
        
        # [关键修复] 优先使用环境变量指定的控制路径
        self.ctrl_path = os.environ.get("OMNIS_ACTOR_CTRL", f"/tmp/mimic_modbus_{os.getpid()}.ctrl")
        
        self.registers = {}
        self.tag_to_reg = {}
        
        for name, tag in self.parser.get_all_tags().items():
            bindings = tag.get('bindings', [])
            for b in bindings:
                if b.get('protocol') == 'MODBUS':
                    attr = b.get('attributes', {})
                    reg_addr = attr.get('addr')
                    if reg_addr is not None:
                        # 默认值
                        self.registers[reg_addr] = 230.0 if 'VOLTAGE' in name else 0.0
                        self.tag_to_reg[name] = reg_addr
                        print(f"🎬 [Mimic] Mapping Tag {name} to Register {reg_addr}")
        
        sys.stdout.flush()

    def _run_control_server(self):
        if os.path.exists(self.ctrl_path): os.remove(self.ctrl_path)
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server.bind(self.ctrl_path)
        server.listen(1)
        print(f"🎮 [Mimic] Control link active at {self.ctrl_path}")
        sys.stdout.flush()

        while True:
            conn, _ = server.accept()
            try:
                data = conn.recv(1024)
                if not data: continue
                cmd = json.loads(data.decode())
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
        threading.Thread(target=self._run_control_server, daemon=True).start()

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
                
                if len(data) >= 8:
                    fc = data[1]
                    slave_id = data[0]
                    if fc == 0x03:
                        start_reg = struct.unpack('>H', data[2:4])[0]
                        reg_count = struct.unpack('>H', data[4:6])[0]
                        byte_count = reg_count * 2
                        payload = bytearray([slave_id, 0x03, byte_count])
                        for i in range(reg_count):
                            # 应用缩放 0.1
                            val = int(self.registers.get(start_reg + i, 0) * 10)
                            payload.extend(struct.pack('>H', val))
                        payload.extend(self.calculate_crc(payload))
                        client.send(payload)
                    elif fc == 0x06:
                        reg_addr = struct.unpack('>H', data[2:4])[0]
                        val = struct.unpack('>H', data[4:6])[0]
                        # 演习特殊处理：如果是 0x00FF (PROJECT_HUMIDITY)，直接存储
                        # 注意：此处不考虑缩放，演习中直接写 1
                        self.registers[reg_addr] = float(val)
                        print(f"⚡ [Mimic] Received WRITE: Reg {reg_addr} -> {val}")
                        # FC 06 响应是请求的镜像
                        client.send(data[:8])
        except Exception as e:
            print(f"❌ [Mimic] Bus error: {e}")
        finally:
            client.close()
            if os.path.exists(self.ctrl_path): os.remove(self.ctrl_path)

if __name__ == "__main__":
    mimic = ModbusMimic()
    mimic.run()