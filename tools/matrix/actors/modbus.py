#!/usr/bin/env python3
import socket
import os
import time
import struct
import json
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
    def __init__(self, socket_path, slave_id, manifest_path=None):
        self.socket_path = socket_path
        self.slave_id = slave_id
        self.registers = {}
        self.tag_to_reg = {}
        if manifest_path and os.path.exists(manifest_path):
            self._load_mappings(manifest_path)
        self._apply_initial_state()

    def _load_mappings(self, path):
        try:
            with open(path, 'r') as f:
                data = json.load(f)
                for tag in data.get("tags", []):
                    for b in tag.get("bindings", []):
                        if "MODBUS" in b["protocol"].upper():
                            addr = b["attributes"].get("addr")
                            if addr is not None:
                                self.tag_to_reg[tag["id"]] = addr
                                self.registers[addr] = 0
        except: pass

    def _apply_initial_state(self):
        init_raw = os.environ.get("MATRIX_INIT_DATA")
        if init_raw:
            try:
                init_data = json.loads(init_raw)
                for tid_str, val in init_data.items():
                    tid = int(tid_str)
                    if tid in self.tag_to_reg:
                        reg_addr = self.tag_to_reg[tid]
                        self.registers[reg_addr] = int(float(val) * 10) if reg_addr == 0 else int(val)
            except: pass

    def run(self):
        print(f"📠 [Actor {self.slave_id}] Starting Client mode...")
        while True:
            try:
                conn = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                conn.connect(self.socket_path)
                print(f"🚀 [Actor {self.slave_id}] LINK ESTABLISHED. Ready for requests.")
                
                while True:
                    data = conn.recv(1024)
                    if not data: break
                    
                    # [埋点日志] 强制冲刷
                    print(f"  📥 [Actor {self.slave_id}] RX: {data.hex().upper()}", flush=True)
                    
                    if len(data) < 6 or data[0] != self.slave_id: continue
                    
                    func = data[1]
                    if func == 0x03:
                        addr = struct.unpack(">H", data[2:4])[0]
                        num = struct.unpack(">H", data[4:6])[0]
                        body = bytearray([self.slave_id, func, num * 2])
                        for i in range(num):
                            val = self.registers.get(addr + i, 0)
                            body.extend(struct.pack(">H", val))
                        crc_val = crc16(body)
                        resp = body + struct.pack("<H", crc_val)
                        conn.sendall(resp)
                        print(f"  📤 [Actor {self.slave_id}] TX: {resp.hex().upper()}", flush=True)
                    
                    # 波动模拟
                    if 0 in self.registers: self.registers[0] = 2200 + (int(time.time() * 10) % 30)
                    if 12 in self.registers: self.registers[12] = 1500 + (int(time.time() * 5) % 100)
            except Exception as e:
                # print(f"⚠️  Actor error: {e}")
                time.sleep(1)
            finally:
                try: conn.close()
                except: pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", required=True)
    parser.add_argument("--id", type=int, default=1)
    parser.add_argument("--manifest", help="Path to omnis_manifest.json")
    args = parser.parse_args()
    ModbusSlaveActor(args.socket, args.id, args.manifest).run()