import socket
import os
import time
import struct
import signal

# --- 校验工具 (CRC16/CS/FCS16) ---
def calc_modbus_crc(data):
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

def calc_dlt_cs(data):
    return bytes([sum(data) & 0xFF])

DLT698_FCS_TAB = [
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
]

def calc_dlt698_fcs(data):
    fcs = 0xFFFF
    for b in data:
        fcs = (fcs >> 8) ^ DLT698_FCS_TAB[(fcs ^ b) & 0xFF]
    return struct.pack('<H', fcs ^ 0xFFFF)

# --- 协议解析与响应 ---
def handle_modbus(data):
    if len(data) < 8: return None
    slave_id = data[0]
    fc = data[1]
    if fc == 0x03:
        count = struct.unpack('>H', data[4:6])[0]
        import random
        val = 2200 + random.randint(-50, 50)
        pdu = bytes([slave_id, fc, count * 2]) + struct.pack('>H', val)
        if count > 1:
            pdu += struct.pack('>H', random.randint(100, 200))
        return pdu + calc_modbus_crc(pdu)
    return None

def handle_dlt645(data):
    if len(data) < 12: return None
    addr = data[1:7]
    ctrl = data[8]
    if ctrl == 0x11:
        res_pdu = b'\x68' + addr + b'\x68\x91\x08'
        res_pdu += data[10:14] # 回显 DI
        res_pdu += bytes([0x33 + 0x05, 0x33 + 0x22, 0x33 + 0x00, 0x33 + 0x00])
        return res_pdu + calc_dlt_cs(res_pdu) + b'\x16'
    return None

def handle_dlt698(data):
    if len(data) < 10: return None
    sa_len = (data[3] & 0x0F) + 1
    sa = data[4:4+sa_len]
    apdu_start = 1 + 2 + 1 + sa_len + 1 + 2
    if len(data) <= apdu_start: return None
    
    svc = data[apdu_start]
    if svc == 0x05: # GET-Request
        piid = data[apdu_start + 2]
        oad = data[apdu_start + 3:apdu_start + 7]
        import random
        val = 2200 + random.randint(-10, 10)
        # APDU: Response, Success, A-XDR long-unsigned (18)
        res_apdu = b'\x81\x01' + bytes([piid]) + oad + b'\x00' + b'\x12' + struct.pack('>H', val)
        body = b'\x03' + bytes([(0 << 6) | (sa_len - 1)]) + sa + b'\x10'
        hcs = calc_dlt698_fcs(body)
        payload = body + hcs + res_apdu
        fcs = calc_dlt698_fcs(payload)
        total_len = 1 + 2 + len(payload) + 2 + 1
        len_bytes = struct.pack('<H', (total_len - 1) & 0x3FFF)
        return b'\x68' + len_bytes + payload + fcs + b'\x16'
    return None

def main():
    SOCK_PATH = "/tmp/rs485_1.sock"
    if os.path.exists(SOCK_PATH): os.remove(SOCK_PATH)
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(SOCK_PATH)
    server.listen(1)
    print(f"📡 [SIM] Meter Simulator listening on {SOCK_PATH}...")
    print("Support: ModbusRTU, DLT645-2007, DLT698.45 (A-XDR Enabled)")
    
    signal.signal(signal.SIGINT, lambda s,f: (server.close(), os.remove(SOCK_PATH), exit(0)))

    while True:
        conn, _ = server.accept()
        print("🔗 [SIM] DCU Connected.")
        try:
            while True:
                data = conn.recv(1024)
                if not data: break
                print(f"📥 [SIM] RX: {data.hex().upper()}")
                
                # --- [Nexus] 破坏性压力测试：40% 概率模拟物理干扰/丢包 ---
                import random
                if random.random() < 0.4:
                    print("👻 [SIM] 模拟物理干扰：故意丢弃报文，保持沉默...")
                    continue

                resp = None
                if data[0] == 0x68:
                    if len(data) > 15 and (data[3] & 0xC0) == 0: resp = handle_dlt698(data)
                    else: resp = handle_dlt645(data)
                elif data[0] in [0x01, 0x02, 0x03]: resp = handle_modbus(data)
                
                if resp:
                    print(f"📤 [SIM] TX: {resp.hex().upper()}")
                    conn.send(resp)
        except Exception as e: print(f"❌ [SIM] Error: {e}")
        finally: conn.close()

if __name__ == "__main__": main()
