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

from collections import defaultdict

class ModbusSlaveActor:
    def __init__(self, socket_path, slave_id, dev_id, manifest_path=None):
        self.socket_path = socket_path
        self.slave_id = slave_id
        self.dev_id = dev_id
        self.registers = defaultdict(int)
        self.tag_to_reg = {}
        self.tag_name_to_id = {}
        self.tag_scalers = {}
        self.tag_lengths = {}
        self.ctrl_file = f"/tmp/matrix_ctrl_{dev_id}.tmp"
        print(f"  📠 [Actor {dev_id}] Control File Path: {os.path.abspath(self.ctrl_file)}")
        if manifest_path and os.path.exists(manifest_path):
            self._load_mappings(manifest_path)
        self._apply_initial_state()

    def _load_mappings(self, path):
        try:
            with open(path, 'r') as f:
                data = json.load(f)
                for tag in (data.get("tags") or []):
                    self.tag_name_to_id[tag["name"]] = tag["id"]
                    for b in (tag.get("bindings") or []):
                        if "MODBUS" in b["protocol"].upper():
                            attr = b["attributes"]
                            addr_raw = attr.get("addr")
                            if addr_raw is not None:
                                try:
                                    # 💡 [Omnis Fix] 支持 0x 前缀的字符串地址或纯数字
                                    addr = int(str(addr_raw), 0)
                                    self.tag_to_reg[tag["id"]] = addr
                                    self.tag_scalers[tag["id"]] = float(attr.get("scale") or attr.get("scaler") or 1.0)
                                    self.tag_lengths[tag["id"]] = int(attr.get("length") or 1)
                                    self.registers[addr] = 0
                                    if self.tag_lengths[tag["id"]] == 2:
                                        self.registers[addr + 1] = 0
                                except (ValueError, TypeError):
                                    print(f"  ⚠️ [Actor] Invalid Address format: {addr_raw} for Tag {tag['id']}")
            print(f"  📠 [Actor {self.dev_id}] Loaded Mappings (tag_to_reg): {self.tag_to_reg}")
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
            print(f"  🔥 [Actor] Control file DETECTED: {self.ctrl_file}")
            try:
                with open(self.ctrl_file, 'r') as f:
                    lines = f.readlines()
                print(f"  🔥 [Actor] Read lines: {lines}")
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
                            
                            if tag_id is not None:
                                tag_id = int(tag_id)
                            
                            if tag_id is not None and tag_id in self.tag_to_reg:
                                reg = self.tag_to_reg[tag_id]
                                scaler = self.tag_scalers.get(tag_id, 1.0)
                                raw_val = int(val / scaler)
                                
                                length = self.tag_lengths.get(tag_id, 1)
                                if length == 2:
                                    # 32位数据：高16位写 reg，低16位写 reg+1 (工业标准)
                                    self.registers[reg] = (raw_val >> 16) & 0xFFFF
                                    self.registers[reg + 1] = raw_val & 0xFFFF
                                    print(f"  🎯 [Actor] 32-bit Injection: {tag_key} -> {val} (Regs {reg}-{reg+1}: 0x{self.registers[reg]:04X}{self.registers[reg+1]:04X})")
                                else:
                                    self.registers[reg] = raw_val & 0xFFFF
                                    print(f"  🎯 [Actor] 16-bit Injection: {tag_key} -> {val} (Reg {reg}: {self.registers[reg]})")
                            else:
                                print(f"  ⚠️ [Actor] Tag {tag_id} ({type(tag_id)}) NOT in tag_to_reg keys: {list(self.tag_to_reg.keys())}")
                        except Exception as e:
                            print(f"  ⚠️ [Actor] Injection Parse Error: {e}")
            except Exception as e:
                print(f"  ⚠️ [Actor] External command process error: {e}")

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
                        
                        # [Nexus Fix] 粘包与分包循环处理 (Modbus RTU 读写标准请求帧为 8 字节)
                        offset = 0
                        while offset + 8 <= len(data):
                            frame = data[offset : offset + 8]
                            offset += 8
                            
                            if frame[0] != self.slave_id: 
                                print(f"  ⚠️ [Actor] Ignored frame targeting slave {frame[0]}, I am {self.slave_id}")
                                continue
                            
                            func = frame[1]
                            if func == 0x03:
                                addr = struct.unpack(">H", frame[2:4])[0]
                                num = struct.unpack(">H", frame[4:6])[0]
                                print(f"  📥 [Actor] Read Reg Request: addr={addr}, num={num}")
                                body = bytearray([self.slave_id, func, num * 2])
                                for i in range(num):
                                    val = self.registers.get(addr + i, 0)
                                    body.extend(struct.pack(">H", int(val)))
                                
                                resp = body + struct.pack("<H", crc16(body))
                                conn.sendall(resp)
                            elif func == 0x06:
                                addr = struct.unpack(">H", frame[2:4])[0]
                                val = struct.unpack(">H", frame[4:6])[0]
                                self.registers[addr] = val
                                print(f"  📝 [Actor] Write Reg {addr} = {val}")
                                # Echo request as response
                                resp = frame[:6] + struct.pack("<H", crc16(frame[:6]))
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