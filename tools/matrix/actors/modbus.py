#!/usr/bin/env python3
import socket
import os
import time
import struct
import argparse

def crc16(data: bytes):
    crc = 0xFFFF
    for pos in data:
        crc ^= pos
        for i in range(8):
            if (crc & 1) != 0:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return crc

class ModbusSlaveActor:
    def __init__(self, socket_path, slave_id=1):
        self.socket_path = socket_path
        self.slave_id = slave_id
        self.registers = {0: 2200, 1: 500} 

    def run(self):
        if os.path.exists(self.socket_path):
            try: os.unlink(self.socket_path)
            except: pass
            
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server.bind(self.socket_path)
        server.listen(5)
        
        print(f"📠 [Modbus Sniffer] Listening on {self.socket_path} (Slave ID: {self.slave_id})")

        while True:
            conn, _ = server.accept()
            # print("  [Modbus] New Connection Accepted.")
            try:
                while True:
                    data = conn.recv(1024)
                    if not data: break
                    
                    # --- 强制打印收到的数据 ---
                    print(f"  📥 RX ({len(data)} bytes): {data.hex().upper()}")
                    
                    if len(data) < 6: continue
                    if data[0] != self.slave_id:
                        print(f"  ⚠️  ID Mismatch: expected {self.slave_id}, got {data[0]}")
                        continue
                    
                    func = data[1]
                    if func == 0x03:
                        addr = struct.unpack(">H", data[2:4])[0]
                        num = struct.unpack(">H", data[4:6])[0]
                        print(f"  🔍 Request: Read Holding Registers at {addr}, count {num}")
                        
                        body = bytearray([self.slave_id, func, num * 2])
                        for i in range(num):
                            val = self.registers.get(addr + i, 0)
                            body.extend(struct.pack(">H", val))
                        
                        crc_val = crc16(body)
                        response = body + struct.pack("<H", crc_val)
                        conn.send(response)
                        print(f"  📤 TX: {response.hex().upper()}")
                    
                    # 动态跳变
                    self.registers[0] = 2200 + (int(time.time() * 5) % 20)
                    self.registers[1] = 500 + (int(time.time() * 5) % 50)
                    self._check_control_commands()
            except Exception as e:
                print(f"  ❌ Actor Error: {e}")
            finally:
                conn.close()

    def _check_control_commands(self):
        ctrl_file = f"/tmp/matrix_ctrl_slave_{self.slave_id}.tmp"
        if os.path.exists(ctrl_file):
            try:
                with open(ctrl_file, "r") as f:
                    cmd = f.read().strip()
                    if ":" in cmd:
                        key, val = cmd.split(":")
                        if "voltage" in key.lower(): self.registers[0] = int(float(val) * 10)
                os.remove(ctrl_file)
            except: pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", default="/tmp/vbus0.sock")
    parser.add_argument("--id", type=int, default=1)
    args = parser.parse_args()
    ModbusSlaveActor(args.socket, args.id).run()