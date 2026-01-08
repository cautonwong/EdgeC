import socket
import os
import time
import struct

def calc_crc(data):
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

def run_simulation():
    # 创建一对互连的 Unix Socket，模拟串口 TX/RX
    parent_sock, child_sock = socket.socketpair(socket.AF_UNIX, socket.SOCK_STREAM)
    
    print("[SIM] Simulation starting. Parent (Python) acts as Modbus Master.")
    
    # 构建 Modbus RTU 写寄存器请求: Slave=01, FC=06, Addr=0001, Val=04D2 (1234)
    pdu = b'\x01\x06\x00\x01\x04\xD2'
    req = pdu + calc_crc(pdu)
    
    print(f"[SIM] Sending Write Request: {req.hex()}")
    parent_sock.send(req)
    
    # 设置超时
    parent_sock.settimeout(2.0)
    try:
        resp = parent_sock.recv(1024)
        print(f"[SIM] Received Response: {resp.hex()}")
        
        # 简单校验：FC 06 的响应应该是请求的回显
        if resp[:6] == pdu:
            print("[SUCCESS] Modbus Slave Engine responded correctly!")
        else:
            print("[FAIL] Unexpected response payload.")
            
    except socket.timeout:
        print("[FAIL] Timed out waiting for response from SlaveEngine.")
    
    parent_sock.close()
    child_sock.close()

if __name__ == "__main__":
    run_simulation()
