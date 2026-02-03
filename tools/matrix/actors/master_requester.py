#!/usr/bin/env python3
import socket
import os
import time
import struct
import json
import argparse
import requests
import sys

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
    def __init__(self, socket_path, slave_id, dev_id, manifest_path, server_url):
        self.socket_path = socket_path
        self.slave_id = slave_id
        self.dev_id = dev_id
        self.server_url = server_url
        self.tag_mappings = {}
        self.tag_scalers = {}
        self._load_mappings(manifest_path)

    def _load_mappings(self, path):
        try:
            with open(path, 'r') as f:
                data = json.load(f)
                for tag in data.get("tags", []):
                    tag_id = tag["id"]
                    for b in tag.get("bindings", []):
                        if "MODBUS" in b["protocol"].upper():
                            attr = b["attributes"]
                            addr = attr.get("addr")
                            if addr is not None:
                                self.tag_mappings[tag_id] = addr
                                self.tag_scalers[tag_id] = float(attr.get("scaler", 1.0))
        except Exception as e:
            sys.stderr.write(f"[Requester] Load Mappings Fail: {e}\n")

    def _request_tag(self, conn, tag_id):
        if tag_id not in self.tag_mappings: return
        addr = self.tag_mappings[tag_id]
        req = struct.pack(">BBHH", self.slave_id, 0x03, addr, 1)
        req += struct.pack("<H", crc16(req))
        
        try:
            conn.sendall(req)
            start_time = time.time()
            rx_buf = b''
            while time.time() - start_time < 1.0:
                chunk = conn.recv(1024)
                if not chunk: break
                rx_buf += chunk
                if len(rx_buf) >= 7:
                    for i in range(len(rx_buf) - 6):
                        if rx_buf[i] == self.slave_id and rx_buf[i+1] == 0x03 and rx_buf[i+2] == 2:
                            msg = rx_buf[i:i+7]
                            if crc16(msg[:-2]) == struct.unpack("<H", msg[-2:])[0]:
                                val = struct.unpack(">H", msg[3:5])[0] * self.tag_scalers.get(tag_id, 1.0)
                                self._report_to_server(tag_id, val)
                                return
        except Exception as e:
            sys.stderr.write(f"[Requester] IO Error: {e}\n")

    def _report_to_server(self, tag_id, value):
        url = f"{self.server_url.rstrip('/')}/observer/record"
        try:
            requests.post(url, json={"dev_id": self.dev_id, "tag": str(tag_id), "value": value}, timeout=1.0)
        except Exception as e:
            sys.stderr.write(f"[Requester] Post Fail: {e}\n")

    def run(self):
        while True:
            try:
                conn = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                conn.connect(self.socket_path)
                conn.settimeout(1.0)
                while True:
                    for tid in list(self.tag_mappings.keys()):
                        self._request_tag(conn, tid)
                        time.sleep(0.5)
            except:
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
