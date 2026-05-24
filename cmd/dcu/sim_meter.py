import socket
import os
import time
import struct
import signal

# --- 校验工具 (CRC16/CS) ---
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

# --- 协议解析与响应 ---
def handle_modbus(data):
    # 期望: [Addr] [FC] [RegH] [RegL] [LenH] [LenL] [CRCL] [CRCH]
    # 例如: 02 03 00 00 00 02 C4 38
    if len(data) < 8: return None
    slave_id = data[0]
    fc = data[1]
    if fc == 0x03: # Read Holding Registers
        addr = struct.unpack('>H', data[2:4])[0]
        count = struct.unpack('>H', data[4:6])[0]
        # 返回模拟数据: 220.5V -> 2205 (0x089D)
        # 我们随机波动一下
        import random
        val = 2200 + random.randint(-50, 50)
        pdu = bytes([slave_id, fc, count * 2]) + struct.pack('>H', val)
        if count > 1:
            pdu += struct.pack('>H', random.randint(100, 200)) # 第二个寄存器假数据
        return pdu + calc_modbus_crc(pdu)
    return None

def handle_dlt645(data):
    # 期望: 68 [Addr]*6 68 [Ctrl] [Len] [DI]*4 [CS] 16
    if len(data) < 12: return None
    if data[0] != 0x68: return None
    addr = data[1:7]
    ctrl = data[8]
    dlen = data[9]
    if ctrl == 0x11: # Read Data
        di = [x - 0x33 for x in data[10:14]]
        # 返回 220.5V -> 05 22 (BCD)
        # 加上 0x33 -> 38 55
        res_pdu = b'\x68' + addr + b'\x68\x91\x08'
        res_pdu += data[10:14] # 回显 DI
        res_pdu += bytes([0x33 + 0x05, 0x33 + 0x22, 0x33 + 0x00, 0x33 + 0x00]) # 0000220.5
        return res_pdu + calc_dlt_cs(res_pdu) + b'\x16'
    return None

def handle_dlt698(data):
    # 极简 DLT698 响应逻辑
    if len(data) < 10: return None
    if data[0] != 0x68: return None
    # 提取 SA
    sa_len = (data[3] & 0x0F) + 1
    sa = data[4:4+sa_len]
    # 提取 APDU
    apdu_start = 4 + sa_len + 1 + 2 # Header + SA + CA + HCS
    svc = data[apdu_start]
    if svc == 0x05: # GET-Request
        piid = data[apdu_start + 2]
        oad = data[apdu_start + 3:apdu_start + 7]
        # 返回 long-unsigned (18) = 2205 (0x089D)
        import random
        val = 2200 + random.randint(-10, 10)
        res_apdu = b'\x81\x01' + bytes([piid]) + oad + b'\x00' # Normal, PIID, OAD, Success
        res_apdu += b'\x12' + struct.pack('>H', val) # Tag 18 (long-unsigned), Value
        
        # 组装完整帧 (简化版，不严格计算长度位)
        frame = b'\x68\x00\x00\x03' + bytes([(0 << 6) | (sa_len - 1)]) + sa + b'\x10'
        # 计算 HCS/FCS 较复杂，此处使用占位符，真实驱动可能报错
        # 为保证闭环，我们还是尝试计算一下
        return None # 暂时不支持 698 的复杂校验闭环，除非脚本也实现 FCS16

def main():
    SOCK_PATH = "/tmp/rs485_1.sock"
    if os.path.exists(SOCK_PATH):
        os.remove(SOCK_PATH)
    
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(SOCK_PATH)
    server.listen(1)
    
    print(f"📡 [SIM] Meter Simulator listening on {SOCK_PATH}...")
    print("支持协议: ModbusRTU (Slave 02), DLT645 (Any Addr)")
    
    def cleanup(sig, frame):
        print("\n👋 [SIM] Simulator stopping...")
        server.close()
        if os.path.exists(SOCK_PATH):
            os.remove(SOCK_PATH)
        exit(0)
    
    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    while True:
        conn, addr = server.accept()
        print("🔗 [SIM] DCU Connected.")
        try:
            while True:
                data = conn.recv(1024)
                if not data: 
                    print("🔗 [SIM] DCU closed connection.")
                    break
                
                print(f"📥 [SIM] RX: {data.hex().upper()}")
                
                resp = None
                try:
                    if data[0] == 0x68:
                        resp = handle_dlt645(data)
                    elif data[0] in [0x01, 0x02, 0x03]: # Possible Modbus Addr
                        resp = handle_modbus(data)
                except Exception as e:
                    print(f"⚠️  [SIM] Parse error: {e}")
                
                if resp:
                    print(f"📤 [SIM] TX: {resp.hex().upper()}")
                    conn.send(resp)
                else:
                    print("❓ [SIM] Unknown or unhandled packet.")
        except Exception as e:
            print(f"❌ [SIM] Connection error: {e}")
        finally:
            conn.close()
            print("🔌 [SIM] DCU Disconnected.")

if __name__ == "__main__":
    main()
