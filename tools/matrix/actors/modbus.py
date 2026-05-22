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
    def __init__(self, socket_path, slave_id, dev_id, manifest_path=None):
        self.socket_path = socket_path
        self.slave_id = slave_id
        self.dev_id = dev_id
        self.registers = {}
        self.tag_to_reg = {}
        self.tag_name_to_id = {}
        self.tag_scalers = {}
        self.ctrl_file = f"/tmp/matrix_ctrl_{dev_id}.tmp"
        
        if manifest_path and os.path.exists(manifest_path):
            self._load_mappings(manifest_path)
        self._apply_initial_state()

    def _load_mappings(self, path):
        try:
            with open(path, 'r') as f:
                data = json.load(f)
                for tag in data.get("tags", []):
                    self.tag_name_to_id[tag["name"]] = tag["id"]
                    for b in tag.get("bindings", []):
                        if "MODBUS" in b["protocol"].upper():
                            attr = b["attributes"]
                            addr = attr.get("addr")
                            if addr is not None:
                                self.tag_to_reg[tag["id"]] = addr
                                self.tag_scalers[tag["id"]] = float(attr.get("scale") or attr.get("scaler") or 1.0)
                                self.registers[addr] = 0
        except Exception as e:
            print(f"Error loading mappings: {e}")

    def _apply_initial_state(self):
        init_raw = os.environ.get("MATRIX_INIT_DATA")
        if init_raw:
            try:
                init_data = json.loads(init_raw)
                for tid_str, val in init_data.items():
                    tid = int(tid_str)
                    if tid in self.tag_to_reg:
                        reg_addr = self.tag_to_reg[tid]
                        scaler = self.tag_scalers.get(tid, 1.0)
                        self.registers[reg_addr] = int(float(val) / scaler)
            except: pass

    def _check_external_commands(self):
        """[Omnis] 监听来自 Matrix Server 的实时注入指令"""
        if os.path.exists(self.ctrl_file):
            try:
                with open(self.ctrl_file, 'r') as f:
                    lines = f.readlines()
                os.remove(self.ctrl_file)
                
                for line in lines:
                    line = line.strip()
                    if ":" in line:
                        tag_key, val_str = line.split(":")
                        try:
                            val = float(val_str)
                            # 解析 Tag: 可能是 ID 字符串，也可能是 Name
                            tag_id = None
                            if tag_key.isdigit():
                                tag_id = int(tag_key)
                            elif tag_key in self.tag_name_to_id:
                                tag_id = self.tag_name_to_id[tag_key]
                            
                            if tag_id is not None and tag_id in self.tag_to_reg:
                                reg = self.tag_to_reg[tag_id]
                                scaler = self.tag_scalers.get(tag_id, 1.0)
                                self.registers[reg] = int(val / scaler)
                                print(f"  🎯 [Actor] Injected {tag_key} (ID:{tag_id}, Reg {reg}) -> {val} (Raw: {self.registers[reg]})")
                        except Exception as e:
                            print(f"  ⚠️ [Actor] Injection Parse Error: {e}")
            except: pass

    def run(self):
        import select
        print(f"📠 [Actor {self.dev_id}] Ready at {self.socket_path}...")
        while True:
            try:
                conn = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                conn.connect(self.socket_path)
                conn.setblocking(False)
                
                while True:
                    self._check_external_commands()
                    
                    readable, _, _ = select.select([conn], [], [], 0.1)
                    if not readable:
                        continue

                    try:
                        data = conn.recv(1024)
                        if not data: break
                        
                        if len(data) < 6 or data[0] != self.slave_id: continue
                        
                        func = data[1]
                        if func == 0x03:
                            addr = struct.unpack(">H", data[2:4])[0]
                            num = struct.unpack(">H", data[4:6])[0]
                            
                            body = bytearray([self.slave_id, func, num * 2])
                            for i in range(num):
                                val = self.registers.get(addr + i, 0)
                                body.extend(struct.pack(">H", int(val)))
                            
                            resp = body + struct.pack("<H", crc16(body))
                            conn.sendall(resp)
                        elif func == 0x06:
                            addr = struct.unpack(">H", data[2:4])[0]
                            val = struct.unpack(">H", data[4:6])[0]
                            self.registers[addr] = val
                            print(f"  📝 [Actor] Write Reg {addr} = {val}")
                            # Echo request as response
                            resp = data[:6] + struct.pack("<H", crc16(data[:6]))
                            conn.sendall(resp)
                    except Exception as e:
                        print(f"  ⚠️ [Actor] Connection Error: {e}")
                        break
            except Exception as e:
                time.sleep(1)
            finally:
                conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", required=True)
    parser.add_argument("--id", type=int, default=1)
    parser.add_argument("--dev_id", required=True)
    parser.add_argument("--manifest", help="Path to omnis_manifest.json")
    args = parser.parse_args()
    ModbusSlaveActor(args.socket, args.id, args.dev_id, args.manifest).run()