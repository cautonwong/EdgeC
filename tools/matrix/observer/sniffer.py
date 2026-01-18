import time
import logging
import os
from datetime import datetime

logger = logging.getLogger("Matrix.Sniffer")

class TrafficSniffer:
    """
    总线嗅探器：实时捕捉 VBus 报文。
    """
    def __init__(self, log_dir: str):
        self.log_dir = log_dir
        if not os.path.exists(log_dir):
            os.makedirs(log_dir, exist_ok=True)
        
        # 文本 Hex 日志：人类可读
        self.text_log_path = os.path.join(log_dir, "vbus_traffic.log")
        self.log_f = open(self.text_log_path, "a")

    def on_traffic(self, direction: str, raw_data: bytes, metadata: dict):
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        bus_id = metadata.get("bus", 0)
        device = metadata.get("device", "Master")
        
        hex_data = raw_data.hex(' ').upper()
        
        # 增加简易协议识别 (Modbus 示例)
        proto_hint = ""
        if len(raw_data) >= 3:
            unit_id = raw_data[0]
            func_code = raw_data[1]
            if func_code in [3, 4, 6, 16]:
                proto_hint = f"[Modbus ID:{unit_id} FC:{func_code}]"

        log_line = f"[{timestamp}] [Bus {bus_id}] {direction:>2} {device:<15} | {hex_data}  {proto_hint}"
        
        with open(self.log_path, "a") as f:
            f.write(log_line + "\n")
        
        # 实时打印到控制台 (仅在调试模式)
        if direction == "RX":
            print(f"\033[94m{log_line}\033[0m") # 蓝色表示接收
        else:
            print(f"\033[92m{log_line}\033[0m") # 绿色表示发送

    def close(self):
        self.log_f.close()
