#!/usr/bin/env python3
import socket
import os
import time
import struct
import json
import argparse
import requests

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

class MasterRequesterActor:
    """
    [Nexus] 模拟上位机 (Supervisor)
    主动轮询 IEMS 的从站接口，并上报结果给 Matrix Server
    """
    def __init__(self, socket_path, slave_id, dev_id, manifest_path, server_url):
        self.socket_path = socket_path
        self.slave_id = slave_id
        self.dev_id = dev_id
        self.server_url = server_url
        self.tag_mappings = {} # TagID -> Address
        self.tag_scalers = {}
        self.latest_data = {}
        
        self._load_mappings(manifest_path)

    def _load_mappings(self, path):
        try:
            with open(path, 'r') as f:
                data = json.load(f)
                for tag in data.get("tags", []):
                    for b in tag.get("bindings", []):
                        if "MODBUS" in b["protocol"].upper():
                            attr = b["attributes"]
                            addr = attr.get("addr")
                            if addr is not None:
                                self.tag_mappings[tag["id"]] = addr
                                self.tag_scalers[tag["id"]] = float(attr.get("scaler", 1.0))
        except Exception as e:
            print(f"  [Requester] Error loading mappings: {e}")

    def _request_tag(self, conn, tag_id):
        if tag_id not in self.tag_mappings: return
        addr = self.tag_mappings[tag_id]
        
        # 构造 Modbus 03 请求
        req = struct.pack(">BBHH", self.slave_id, 0x03, addr, 1)
        req += struct.pack("<H", crc16(req))
        
        try:
            conn.sendall(req)
            
            # [Nexus Fix] 更加鲁棒的接收解析
            start_recv = time.time()
            rx_buf = b''
            while time.time() - start_recv < 2.0:
                try:
                    chunk = conn.recv(1024)
                    if not chunk: break
                    rx_buf += chunk
                except socket.timeout:
                    continue
                
                # 寻找可能的 Modbus 响应头 [ID, Func, ByteCount]
                # 响应长度对于 1 个寄存器通常是 7 字节: ID(1)+FC(1)+BC(1)+Data(2)+CRC(2)
                found_valid_frame = False
                while len(rx_buf) >= 7:
                    if rx_buf[0] == self.slave_id and rx_buf[1] == 0x03:
                        byte_count = rx_buf[2]
                        expected_len = 3 + byte_count + 2
                        if len(rx_buf) >= expected_len:
                            # 提取数据
                            payload = rx_buf[3:3+byte_count]
                            raw_val = struct.unpack(">H", payload)[0]
                            actual_val = raw_val * self.tag_scalers.get(tag_id, 1.0)
                            
                            self.latest_data[tag_id] = actual_val
                            print(f"  👁️  [Requester] Observed Tag {tag_id} = {actual_val}")
                            self._report_to_server(tag_id, actual_val)
                            
                            # 移出已处理部分
                            rx_buf = rx_buf[expected_len:]
                            found_valid_frame = True
                            break 
                        else: break # 等待更多数据
                    else:
                        rx_buf = rx_buf[1:] # 错位搜索
                
                if found_valid_frame:
                    return # 处理成功，跳出 outer while
                
        except Exception as e:
            print(f"  ⚠️ [Requester] Error during request: {e}")

    def _report_to_server(self, tag_id, value):
        try:
            payload = {"dev_id": self.dev_id, "tag": tag_id, "value": value, "role": "supervisor"}
            # [Nexus Fix] Increase timeout to 1.0s to ensure reporting succeeds under load
            requests.post(f"{self.server_url}/observer/record", json=payload, timeout=1.0)
        except:
            pass

    def run(self):
        print(f"🕵️  [Requester {self.dev_id}] Starting Supervisor Loop at {self.socket_path}...")
        while True:
            try:
                conn = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                conn.connect(self.socket_path)
                conn.settimeout(1.0)
                print(f"  ✅ [Requester] Connected to Virtual Bus.")
                
                # [Nexus Fix] 等待总线稳定
                time.sleep(1.0)
                
                while True:
                    for tid in list(self.tag_mappings.keys()):
                        self._request_tag(conn, tid)
                        time.sleep(0.5) 
            except Exception as e:
                time.sleep(1)
            finally:
                try: conn.close()
                except: pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", required=True)
    parser.add_argument("--id", type=int, default=1)
    parser.add_argument("--dev_id", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--server", default="http://localhost:50051/api/v1")
    args = parser.parse_args()
    
    MasterRequesterActor(args.socket, args.id, args.dev_id, args.manifest, args.server).run()