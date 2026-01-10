import socket
import os
import time
import struct
import sys
from ..engine.parser import ContractParser

class ModbusMimic:
    def __init__(self, product="iems"):
        gen_yaml = f"build/products/{product}/generated/contracts.yaml"
        self.parser = ContractParser(gen_yaml)
        self.uds_path = "/tmp/vbus0.sock"
        self.registers = {}
        for name, tag in self.parser.get_all_tags().items():
            binding = tag.get('binding')
            if binding and binding.get('protocol') == 'ModbusRTU':
                reg_addr = binding.get('reg_addr')
                if reg_addr is not None:
                    self.registers[reg_addr] = 230.0 if 'VOLTAGE' in name else 12.3
                    print(f"🎬 [Mimic] Mapping Tag {name} to Register {reg_addr}")
        sys.stdout.flush()

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
        print(f"🔄 [Mimic] Starting loop, waiting for {self.uds_path}...")
        sys.stdout.flush()
        
        while not os.path.exists(self.uds_path):
            time.sleep(0.5)

        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            client.connect(self.uds_path)
            print(f"✅ [Mimic] Connected to EdgeC Bus at {self.uds_path}")
            sys.stdout.flush()
            
            while True:
                data = client.recv(1024)
                if not data: break
                
                print(f"📡 [Mimic] RAW RECV: {data.hex().upper()}")
                sys.stdout.flush()
                
                if len(data) >= 8 and data[1] == 0x03:
                    slave_id = data[0]
                    start_reg = struct.unpack('>H', data[2:4])[0]
                    reg_count = struct.unpack('>H', data[4:6])[0]
                    
                    print(f"📡 [Mimic] Req: Read {reg_count} regs from {start_reg}")
                    sys.stdout.flush()
                    
                    byte_count = reg_count * 2
                    payload = bytearray([slave_id, 0x03, byte_count])
                    for i in range(reg_count):
                        val = int(self.registers.get(start_reg + i, 0) * 10)
                        payload.extend(struct.pack('>H', val))
                    
                    payload.extend(self.calculate_crc(payload))
                    client.send(payload)
        except Exception as e:
            print(f"❌ [Mimic] Error: {e}")
        finally:
            print("🛑 [Mimic] Exiting.")
            sys.stdout.flush()
            client.close()

if __name__ == "__main__":
    mimic = ModbusMimic("iems")
    mimic.run()