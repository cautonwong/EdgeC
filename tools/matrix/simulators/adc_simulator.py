#!/usr/bin/env python3
import os
import numpy as np
import struct
import socket
import time
import signal
import json
import threading
from typing import List, Dict

class WaveComponent:
    """可配置的波形组件"""
    def __init__(self, name: str):
        self.name = name
        self.enabled = True

    def generate(self, t_vec: np.ndarray) -> np.ndarray:
        raise NotImplementedError

    def update_params(self, params: dict):
        for k, v in params.items():
            if hasattr(self, k):
                setattr(self, k, float(v))
                print(f"  [ADC-SIM] Updated {self.name}.{k} = {v}")

class SineSource(WaveComponent):
    def __init__(self, name="base_sine", freq=50.0, amp=311.0, phase=0.0):
        super().__init__(name)
        self.freq = freq
        self.amp = amp
        self.phase = phase
    def generate(self, t_vec):
        return self.amp * np.sin(2 * np.pi * self.freq * t_vec + self.phase)

class HarmonicDisturb(WaveComponent):
    def __init__(self, name="harmonic", order=3, ratio=0.0):
        super().__init__(name)
        self.order = order
        self.ratio = ratio
        self.base_freq = 50.0
    def generate(self, t_vec):
        if self.ratio <= 0: return np.zeros_like(t_vec)
        return (311.0 * self.ratio) * np.sin(2 * np.pi * (self.base_freq * self.order) * t_vec)

class NoiseSource(WaveComponent):
    def __init__(self, name="noise", level=0.0):
        super().__init__(name)
        self.level = level
    def generate(self, t_vec):
        if self.level <= 0: return np.zeros_like(t_vec)
        return np.random.normal(0, self.level, len(t_vec))

class AdcSimulator:
    """
    [Omnis] Sprint 5: Ion - 工业级 ADC 仿真引擎
    """
    def __init__(self, sample_rate=100000, block_size=1000):
        self.sample_rate = sample_rate
        self.block_size = block_size
        self.num_buffers = 8
        self.buffer_fds = []
        self.buffers = []
        self.components: Dict[str, WaveComponent] = {}
        self.current_t = 0.0
        self.running = True
        
        self._setup_memfds()
        
        # 默认初始化组件
        self.add_component(SineSource("grid"))
        self.add_component(HarmonicDisturb("h3", order=3))
        self.add_component(HarmonicDisturb("h5", order=5))
        self.add_component(NoiseSource("white_noise"))

    def _setup_memfds(self):
        import ctypes
        libc = ctypes.CDLL("libc.so.6")
        MFD_ALLOW_SEALING = 0x0002
        import mmap
        
        for i in range(self.num_buffers):
            name = f"adc_v_{i}".encode()
            fd = libc.memfd_create(name, MFD_ALLOW_SEALING)
            os.ftruncate(fd, self.block_size * 4)
            self.buffer_fds.append(fd)
            buf = mmap.mmap(fd, self.block_size * 4, mmap.MAP_SHARED, mmap.PROT_WRITE)
            self.buffers.append(buf)
        print(f"✅ ADC Simulator: {self.num_buffers} buffers ready (BlockSize: {self.block_size})")

    def add_component(self, comp: WaveComponent):
        self.components[comp.name] = comp

    def update_block(self, buf_idx):
        t_start = self.current_t
        t_end = t_start + (self.block_size / self.sample_rate)
        t_vec = np.linspace(t_start, t_end, self.block_size, endpoint=False)
        
        out = np.zeros(self.block_size, dtype=np.float32)
        for comp in self.components.values():
            if comp.enabled:
                out += comp.generate(t_vec).astype(np.float32)
        
        self.buffers[buf_idx].seek(0)
        self.buffers[buf_idx].write(out.tobytes())
        self.current_t = t_end

    def _command_listener(self):
        """监听控制指令以动态修改波形参数"""
        ctrl_path = "/tmp/matrix_adc_ctrl.sock"
        if os.path.exists(ctrl_path): os.remove(ctrl_path)
        
        with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as s:
            s.bind(ctrl_path)
            print(f"🎮 ADC Command Listener ready at {ctrl_path}")
            while self.running:
                try:
                    data, _ = s.recvfrom(1024)
                    cmd = json.loads(data.decode())
                    # e.g. {"target": "grid", "params": {"freq": 49.5}}
                    target = cmd.get("target")
                    if target in self.components:
                        self.components[target].update_params(cmd.get("params", {}))
                except: pass

    def run(self, socket_path="/tmp/v_adc.sock"):
        threading.Thread(target=self._command_listener, daemon=True).start()
        
        if os.path.exists(socket_path): os.remove(socket_path)
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as srv:
            srv.bind(socket_path)
            srv.listen(1)
            print(f"📡 ADC Stream Server ready at {socket_path}")
            
            while self.running:
                conn, _ = srv.accept()
                print("🔗 EdgeOS Handshake: Transferring Buffer Metadata...")
                try:
                    # 1. 握手：发送所有 FD 和基本信息
                    # 先发个头信息告知 BlockSize 和 BufferCount
                    conn.sendall(struct.pack("<II", self.block_size, self.num_buffers))
                    
                    # 2. 循环发送数据通知
                    buf_idx = 0
                    while self.running:
                        self.update_block(buf_idx)
                        
                        # 发送 TYPE_REF 消息 (64 bytes)
                        # [TS(8), TagID(4), Type(2), State(2), ... Payload(FD)]
                        # 注意：真正的 PipeMsg 发送需要对齐 C 结构体
                        ts_ns = int(time.time() * 1e9)
                        # Tag 2001: ADC_RAW_DATA
                        msg_header = struct.pack("<QIHHI", ts_ns, 2001, 2, 1, 0)
                        padding = b'\x00' * (64 - len(msg_header) - 4) # 预留 4 字节给 FD 占位
                        
                        import array
                        fds = array.array("i", [self.buffer_fds[buf_idx]])
                        conn.sendmsg([msg_header + padding + struct.pack("<i", buf_idx)], 
                                     [(socket.SOL_SOCKET, socket.SCM_RIGHTS, fds)])
                        
                        # 控制流速
                        time.sleep(self.block_size / self.sample_rate)
                        buf_idx = (buf_idx + 1) % self.num_buffers
                except Exception as e:
                    print(f"❌ Stream disconnected: {e}")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", default="/tmp/v_adc.sock")
    parser.add_argument("--rate", type=int, default=100000)
    args = parser.parse_args()
    
    sim = AdcSimulator(sample_rate=args.rate, block_size=1000)
    sim.run(args.socket)
