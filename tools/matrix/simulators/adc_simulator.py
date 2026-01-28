#!/usr/bin/env python3
import os
import numpy as np
import struct
import socket
import time
import signal
import json
import threading
import array
from typing import List, Dict, Optional

# ==============================================================================
# [Omnis] Sprint 5: Ion - 工业级 ADC 仿真引擎 (Compatibility Version)
# ==============================================================================

class WaveComponent:
    def __init__(self, name: str):
        self.name = name
        self.enabled = True

    def generate(self, t_vec: np.ndarray) -> np.ndarray:
        raise NotImplementedError

    def update_params(self, params: dict):
        for k, v in params.items():
            if hasattr(self, k):
                curr = getattr(self, k)
                if isinstance(curr, float): val = float(v)
                elif isinstance(curr, int): val = int(v)
                else: val = v
                setattr(self, k, val)
                print(f"  [ADC-SIM] Updated {self.name}.{k} = {val}")

class SineSource(WaveComponent):
    def __init__(self, name="grid", freq=50.0, amp=311.12):
        super().__init__(name)
        self.freq = freq
        self.amp = amp
    def generate(self, t_vec):
        return self.amp * np.sin(2 * np.pi * self.freq * t_vec)

class HarmonicDisturb(WaveComponent):
    def __init__(self, name, order, ratio=0.0):
        super().__init__(name)
        self.order = order
        self.ratio = ratio
        self.base_freq = 50.0
        self.base_amp = 311.12
    def generate(self, t_vec):
        if self.ratio <= 0: return np.zeros_like(t_vec)
        h_amp = self.base_amp * self.ratio
        return h_amp * np.sin(2 * np.pi * (self.base_freq * self.order) * t_vec)

class NoiseSource(WaveComponent):
    def __init__(self, name="noise", level=0.0):
        super().__init__(name)
        self.level = level
    def generate(self, t_vec):
        if self.level <= 0: return np.zeros_like(t_vec)
        return np.random.normal(0, self.level, len(t_vec))

class PipeMsgPacker:
    HEADER_FORMAT = "<QIHHI I B 7s" 
    PAYLOAD_REF_FORMAT = "<Q i Q Q"
    
    @staticmethod
    def pack_ref(tag_id: int, fd: int, length: int, ts_ns: int):
        header = struct.pack(PipeMsgPacker.HEADER_FORMAT,
                             ts_ns, tag_id, 2, 1, 0, 0, 1, b'\x00'*7)
        payload = struct.pack(PipeMsgPacker.PAYLOAD_REF_FORMAT, 0, fd, length, 0)
        return header + payload

class AdcSimulator:
    def __init__(self, sample_rate=4000, block_size=400):
        self.sample_rate = sample_rate
        self.block_size = block_size
        self.num_buffers = 8
        self.buffer_fds = []
        self.buffers = []
        self.components: Dict[str, WaveComponent] = {}
        self.current_t = 0.0
        self.running = True
        
        self._setup_memfds()
        
        # 兼容旧测试的组件命名
        self.add_component(SineSource("grid"))
        self.add_component(HarmonicDisturb("h3", order=3))
        self.add_component(HarmonicDisturb("h5", order=5))
        self.add_component(NoiseSource("noise"))

    def _setup_memfds(self):
        import ctypes
        libc = ctypes.CDLL("libc.so.6")
        MFD_ALLOW_SEALING = 0x0002
        import mmap
        
        self.buffer_size = self.block_size * 4 # 1 Channel
        
        for i in range(self.num_buffers):
            name = f"adc_v_{i}".encode()
            fd = libc.memfd_create(name, MFD_ALLOW_SEALING)
            os.ftruncate(fd, self.buffer_size)
            self.buffer_fds.append(fd)
            buf = mmap.mmap(fd, self.buffer_size, mmap.MAP_SHARED, mmap.PROT_WRITE)
            self.buffers.append(buf)
        print(f"✅ ADC Simulator Ready: {self.num_buffers} buffers (Size: {self.block_size} floats)")

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
        ctrl_path = "/tmp/matrix_adc_ctrl.sock"
        if os.path.exists(ctrl_path): os.remove(ctrl_path)
        
        with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as s:
            s.bind(ctrl_path)
            while self.running:
                try:
                    data, _ = s.recvfrom(2048)
                    cmd = json.loads(data.decode())
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
                try:
                    conn, _ = srv.accept()
                    # 简化握手
                    conn.sendall(struct.pack("<II", self.block_size, self.num_buffers))
                    
                    buf_idx = 0
                    while self.running:
                        start_tick = time.perf_counter()
                        self.update_block(buf_idx)
                        
                        ts_ns = int(time.time() * 1e9)
                        msg = PipeMsgPacker.pack_ref(2001, self.buffer_fds[buf_idx], self.buffer_size, ts_ns)
                        
                        fds = array.array("i", [self.buffer_fds[buf_idx]])
                        conn.sendmsg([msg], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, fds)])
                        
                        elapsed = time.perf_counter() - start_tick
                        sleep_time = (self.block_size / self.sample_rate) - elapsed
                        if sleep_time > 0:
                            time.sleep(sleep_time)
                        
                        buf_idx = (buf_idx + 1) % self.num_buffers
                except Exception as e:
                    print(f"❌ Stream disconnected: {e}")
                    if not self.running: break

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", default="/tmp/v_adc.sock")
    parser.add_argument("--rate", type=int, default=4000)
    parser.add_argument("--block", type=int, default=400)
    args = parser.parse_args()
    
    sim = AdcSimulator(sample_rate=args.rate, block_size=args.block)
    sim.run(args.socket)
