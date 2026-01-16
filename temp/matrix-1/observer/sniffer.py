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

    def on_traffic(self, direction: str, data: bytes, meta: dict):
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        bus_id = meta.get("bus", 0)
        dev_info = f" | {meta['device']}" if "device" in meta else ""
        
        # 语义化初步解析 (如果是 Modbus)
        summary = ""
        if len(data) >= 3:
            if data[1] == 0x03: summary = "[ReadHR]"
            elif data[1] == 0x10: summary = "[WriteMult]"
            
        log_line = f"[{timestamp}] [Bus {bus_id}] {direction} | {data.hex(' ').upper()} {dev_info} {summary}\n"
        
        self.log_f.write(log_line)
        self.log_f.flush()
        os.fsync(self.log_f.fileno()) # 强制系统级刷盘
        
        # 如果是 TX (EdgeC发出的)，用青色打印到控制台；如果是 RX (Matrix回复的)，用绿色
        color = "\033[96m" if direction == "TX" else "\033[92m"
        print(f"{color}{log_line.strip()}\033[0m")

    def close(self):
        self.log_f.close()
