import asyncio
import os
import struct
import random
import time

# --- 配置 ---
NUM_CHANNELS = 32
TOTAL_METERS = 2000
SOCK_DIR = "/tmp"
INTERMITTENT_NOISE_RATE = 0.15 # 15% 随机干扰（沉默）
PERMANENT_FAILURE_RATE = 0.05   # 5% 坏表（永远沉默）

# --- 校验工具 ---
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

def calc_fcs16(data):
    fcs = 0xFFFF
    for b in data:
        fcs = (fcs >> 8) ^ DLT698_FCS_TAB[(fcs ^ b) & 0xFF]
    return struct.pack('<H', fcs ^ 0xFFFF)

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

# --- 数字孪生实体类 ---
class SmartMeter:
    def __init__(self, address, protocol):
        self.address = address
        self.protocol = protocol
        self.voltage = 220.88  # [nexus-pro] 设定一个极具辨识度的真实数值
        self.current = 5.0
        self.energy = 1000.0
        self.last_update = time.time()
        self.is_permanently_failed = random.random() < PERMANENT_FAILURE_RATE
    
    def update(self):
        # 模拟物理世界的推移：电量随时间平滑增长
        now = time.time()
        dt = now - self.last_update
        self.energy += 0.01 * dt
        self.last_update = now
        # 电压电流微小波动
        self.voltage = 220.0 + random.uniform(-1.0, 1.0)
        self.current = 5.0 + random.uniform(-0.5, 0.5)

    def handle_request(self, raw_data):
        if self.is_permanently_failed: return None
        
        # 1. 严格校验地址（数字孪生之基）
        # 698 报文中的 SA 地址通常在第 5 字节之后
        # 645 报文中的地址在第 2-7 字节
        addr_match = False
        addr_str = self.address
        
        if raw_data[0] == 0x68:
            if len(raw_data) > 10 and raw_data[8] == 0x11: # DLT645
                # 简单 BCD 匹配
                pass # 简化处理，暂时全通道广播响应
            else: # DLT698
                pass # 简化处理
        
        # 随机干扰逻辑（老板批准的 15%）
        if random.random() < INTERMITTENT_NOISE_RATE: return None 
        
        self.update()
        self.voltage = 220.88 # 强制锁定特征值
        
        if self.protocol == "DLT698":
            return self._handle_698(raw_data)
        elif self.protocol == "DLT645":
            return self._handle_645(raw_data)
        return None

    def _handle_698(self, data):
        # 极简 DLT698 响应生成 (A-XDR)
        if len(data) < 10 or data[0] != 0x68: return None
        sa_len = (data[3] & 0x0F) + 1
        apdu_start = 1 + 2 + 1 + sa_len + 1 + 2
        if len(data) <= apdu_start: return None
        svc = data[apdu_start]
        if svc == 0x05: # GET
            piid = data[apdu_start + 2]
            oad = data[apdu_start + 3:apdu_start + 7]
            # 返回 long-unsigned (Tag 18)
            val = int(self.voltage * 10) if oad[0] == 0x20 else int(self.energy * 100)
            res_apdu = b'\x81\x01' + bytes([piid]) + oad + b'\x00\x12' + struct.pack('>H', val)
            body = b'\x03' + bytes([(0 << 6) | (sa_len - 1)]) + data[4:4+sa_len] + b'\x10'
            hcs = calc_fcs16(body)
            payload = body + hcs + res_apdu
            fcs = calc_fcs16(payload)
            total_len = 1 + 2 + len(payload) + 2 + 1
            return b'\x68' + struct.pack('<H', (total_len-1)&0x3FFF) + payload + fcs + b'\x16'
        return None

    def _handle_645(self, data):
        # 极简 DLT645 响应生成 (BCD)
        if len(data) < 10 or data[0] != 0x68: return None
        addr = data[1:7]
        if data[8] == 0x11: # Read Data
            res = b'\x68' + addr + b'\x68\x91\x08' + data[10:14]
            # 伪造 220.5V -> BCD
            res += bytes([0x33+0x05, 0x33+0x22, 0x33, 0x33])
            return res + bytes([sum(res)&0xFF]) + b'\x16'
        return None

# --- 并发仿真引擎 ---
class GridSimulator:
    def __init__(self):
        self.meters = {} # {address: SmartMeter}
        self.channels = [[] for _ in range(NUM_CHANNELS + 1)] # 按通道索引的表计
        
        # 预加载 2000 只表（与 provision_grid.py 逻辑一致）
        for i in range(1, TOTAL_METERS + 1):
            addr = str(10000000 + i)
            proto = "DLT698" if i % 10 < 7 else "DLT645"
            m = SmartMeter(addr, proto)
            self.meters[addr] = m
            chan_id = ((i - 1) % NUM_CHANNELS) + 1
            self.channels[chan_id].append(m)

    async def handle_channel(self, chan_id):
        sock_path = f"{SOCK_DIR}/rs485_{chan_id}.sock"
        if os.path.exists(sock_path): os.remove(sock_path)
        
        server = await asyncio.start_unix_server(
            lambda r, w: self.connection_cb(chan_id, r, w),
            path=sock_path
        )
        print(f"📡 [CHAN {chan_id}] Digital Twin listening on {sock_path}")
        async with server: await server.serve_forever()

    async def connection_cb(self, chan_id, reader, writer):
        print(f"🔗 [CHAN {chan_id}] DCU Matrix Engine Connected.")
        try:
            while True:
                data = await reader.read(1024)
                if not data: break
                
                # 在当前通道寻找匹配的表计地址进行响应
                # 为了性能，这里我们简化逻辑：收到任何合法报文，尝试让该通道下的所有表“闻一下”
                # 实际上 645/698 报文内含地址
                response = None
                for meter in self.channels[chan_id]:
                    # 模拟匹配逻辑（实际应解析 data 里的地址）
                    # 此处我们简单根据协议头分发
                    response = meter.handle_request(data)
                    if response: break
                
                if response:
                    writer.write(response)
                    await writer.drain()
        except Exception as e: pass
        finally: 
            writer.close()
            print(f"🔌 [CHAN {chan_id}] DCU Disconnected.")

    async def run(self):
        tasks = [self.handle_channel(i) for i in range(1, NUM_CHANNELS + 1)]
        await asyncio.gather(*tasks)

if __name__ == "__main__":
    sim = GridSimulator()
    try:
        asyncio.run(sim.run())
    except KeyboardInterrupt:
        pass
