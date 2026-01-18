import numpy as np
import mmap
import os
import logging

logger = logging.getLogger("Matrix.ADC")

class ADCSimulator:
    def __init__(self, sample_rate=4000, frame_size=400):
        self.sample_rate = sample_rate
        self.frame_size = frame_size
        self.fd = -1
        self.mmap_obj = None

    def create_memfd(self, name="v_adc"):
        self.fd = os.memfd_create(name)
        size = self.frame_size * 4 
        os.ftruncate(self.fd, size)
        self.mmap_obj = mmap.mmap(self.fd, size, flags=mmap.MAP_SHARED, prot=mmap.PROT_WRITE)
        logger.info(f"Created memfd {name} (size: {size} bytes) for ADC simulation")
        return self.fd

    def generate_sine(self, freq=50, amplitude=220.0, noise_std=0.0, harmonics=None):
        t = np.linspace(0, self.frame_size / self.sample_rate, self.frame_size, endpoint=False)
        signal = amplitude * np.sin(2 * np.pi * freq * t)
        if harmonics:
            for order, ratio in harmonics:
                signal += (amplitude * ratio) * np.sin(2 * np.pi * freq * order * t)
        if noise_std > 0:
            signal += np.random.normal(0, noise_std, self.frame_size)
            
        # 工业级：直接使用系统调用 pwrite 写入 memfd
        data_bytes = signal.astype(np.float32).tobytes()
        os.pwrite(self.fd, data_bytes, 0)
        return signal

    def write_val(self, val: float):
        data = np.full(self.frame_size, val, dtype=np.float32).tobytes()
        os.pwrite(self.fd, data, 0)

    def close(self):
        if self.mmap_obj:
            self.mmap_obj.close()
        if self.fd != -1:
            os.close(self.fd)
