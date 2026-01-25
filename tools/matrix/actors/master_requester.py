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
        
        # 构造 Modbus 03 请求: [ID, Func, Addr_H, Addr_L, Num_H, Num_L, CRC_L, CRC_H]
        req = struct.pack(">BBHH", self.slave_id, 0x03, addr, 1)
        req += struct.pack("<H", crc16(req))
        
        try:
            conn.sendall(req)
            resp = conn.recv(1024)
            if len(resp) >= 7 and resp[0] == self.slave_id and resp[1] == 0x03:
                byte_count = resp[2]
                raw_val = struct.unpack(">H", resp[3:3+byte_count])[0]
                actual_val = raw_val * self.tag_scalers.get(tag_id, 1.0)
                self.latest_data[tag_id] = actual_val
                
                # 同步回 Matrix Server 供观察
                self._report_to_server(tag_id, actual_val)
                # print(f"  [Requester] Polled Tag {tag_id} -> {actual_val}")
        except Exception as e:
            pass

    def _report_to_server(self, tag_id, value):
        try:
            payload = {"dev_id": self.dev_id, "tag": tag_id, "value": value, "role": "supervisor"}
            # 注意：此处 endpoint 需与 server.py 配合
            requests.post(f"{self.server_url}/observer/record", json=payload, timeout=0.1)
        except: pass

    def run(self):
        print(f"🕵️  [Requester {self.dev_id}] Starting Supervisor Loop at {self.socket_path}...")
        while True:
            try:
                with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as conn:
                    conn.connect(self.socket_path)
                    conn.settimeout(1.0)
                    while True:
                        for tid in self.tag_mappings.keys():
                            self._request_tag(conn, tid)
                            time.sleep(0.2) # 控制主站轮询频率
            except Exception as e:
                time.sleep(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", required=True)
    parser.add_argument("--id", type=int, default=1)
    parser.add_argument("--dev_id", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--server", default="http://localhost:50051/api/v1")
    args = parser.parse_args()
    
    MasterRequesterActor(args.socket, args.id, args.dev_id, args.manifest, args.server).run()
