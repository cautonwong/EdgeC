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
        print(f"  [Requester] Loading mappings from {path}")
        try:
            with open(path, 'r') as f:
                data = json.load(f)
                for tag in data.get("tags", []):
                    # 记录名字到 ID 的映射，方便调试
                    tag_id = tag["id"]
                    for b in tag.get("bindings", []):
                        if "MODBUS" in b["protocol"].upper():
                            attr = b["attributes"]
                            addr = attr.get("addr")
                            if addr is not None:
                                self.tag_mappings[tag_id] = addr
                                self.tag_scalers[tag_id] = float(attr.get("scaler", 1.0))
                                print(f"    - Mapped Tag {tag_id} ({tag['name']}) -> Addr {addr}, Scale {self.tag_scalers[tag_id]}")
        except Exception as e:
            print(f"  [Requester] Error loading mappings from {path}: {e}")
            if "tags" not in str(e): # 如果是 JSON 解析错误
                print(f"  [Requester] Hint: Manifest must be JSON, not YAML.")

    def _request_tag(self, conn, tag_id):
        if tag_id not in self.tag_mappings: return
        addr = self.tag_mappings[tag_id]
        
        # [Nexus Fix] 移除危险的非阻塞 drain，改为设置超时直接发送
        conn.setblocking(True)
        conn.settimeout(1.0)

        # 构造 Modbus 03 请求
        req = struct.pack(">BBHH", self.slave_id, 0x03, addr, 1)
        req += struct.pack("<H", crc16(req))
        
        try:
            conn.sendall(req)
            # print(f"  📤 [Requester] Sent request for Tag {tag_id} (Addr {addr})")
            
            start_recv = time.time()
            rx_buf = b''
            while time.time() - start_recv < 1.0: # [Nexus Fix] 缩短超时，加快轮询效率
                try:
                    chunk = conn.recv(1024)
                    if not chunk: break
                    print(f"  📥 [Requester] Received {len(chunk)} bytes: {chunk.hex()}")
                    rx_buf += chunk
                except socket.timeout:
                    continue
                
                # 寻找匹配的响应：ID(1) + FC(1) + ByteCount(1) + Data(2) + CRC(2) = 7 bytes
                while len(rx_buf) >= 7:
                    if rx_buf[0] == self.slave_id and rx_buf[1] == 0x03:
                        byte_count = rx_buf[2]
                        expected_len = 3 + byte_count + 2
                        if len(rx_buf) >= expected_len:
                            payload = rx_buf[3:3+byte_count]
                            try:
                                if len(payload) >= 2:
                                    raw_val = struct.unpack(">H", payload[:2])[0]
                                    actual_val = raw_val * self.tag_scalers.get(tag_id, 1.0)
                                    
                                    self.latest_data[tag_id] = actual_val
                                    # print(f"  👁️  [Requester] Observed Tag {tag_id} = {actual_val}")
                                    self._report_to_server(tag_id, actual_val)
                                    return # 成功，退出该 Tag 处理
                            except Exception as pe:
                                print(f"  ⚠️ [Requester] Parse Error: {pe}")
                            rx_buf = rx_buf[expected_len:]
                        else: break # 数据不足
                    else:
                        rx_buf = rx_buf[1:] # 错位
                
        except Exception as e:
            print(f"  ⚠️ [Requester] Error during request Tag {tag_id}: {e}")

    def _report_to_server(self, tag_id, value):
        try:
            # [Nexus Fix] 确保 tag_id 以字符串形式上报，对齐 server.py 的 key 逻辑
            payload = {"dev_id": self.dev_id, "tag": str(tag_id), "value": value, "role": "supervisor"}
            resp = requests.post(f"{self.server_url}/observer/record", json=payload, timeout=1.0)
            if resp.status_code == 200:
                print(f"  ✅ [Requester] Reported Tag {tag_id} to server.")
            else:
                print(f"  ❌ [Requester] Failed to report Tag {tag_id}: HTTP {resp.status_code}")
        except Exception as e:
            print(f"  ⚠️ [Requester] Report Error: {e}")

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
                
                loop_count = 0
                while True:
                    loop_count += 1
                    if loop_count % 10 == 0:
                        print(f"  💓 [Requester] Polling Loop Active (Iteration {loop_count})")
                    
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