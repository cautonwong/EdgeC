import numpy as np
import mmap
import os
import logging
from typing import Dict, List, Optional

logger = logging.getLogger("Matrix.SignalFactory")

class SignalFactory:
    """
    高频信号工况工厂：利用 NumPy 模拟复杂的电网物理特性。
    支持：多频谐波、电压暂态（Sags/Swells）、频率漂移、非平稳噪声。
    """
    def __init__(self, sample_rate: int = 4000, frame_size: int = 400):
        self.sample_rate = sample_rate
        self.frame_size = frame_size
        self.fd = -1
        self.mmap_obj = None
        self.t_base = np.linspace(0, frame_size / sample_rate, frame_size, endpoint=False)
        self._current_time_offset = 0.0

    def create_memfd(self, name: str = "v_adc_shm"):
        """创建匿名内存文件，用于零拷贝跨进程共享"""
        self.fd = os.memfd_create(name)
        size = self.frame_size * 4  # float32
        os.ftruncate(self.fd, size)
        # MAP_SHARED 确保 Python 的写入对 C 进程可见
        self.mmap_obj = mmap.mmap(self.fd, size, flags=mmap.MAP_SHARED, prot=mmap.PROT_WRITE)
        logger.info(f"🚀 SignalFactory: Created memfd '{name}' ({size} bytes)")
        return self.fd

    def compose_wave(self, 
                     freq: float = 50.0, 
                     amp: float = 220.0, 
                     harmonics: Optional[Dict[int, float]] = None,
                     transient: Optional[Dict] = None,
                     noise_std: float = 0.0) -> np.ndarray:
        """
        合成一组复杂的工况数据。
        :param freq: 基波频率 (Hz)
        :param amp: 基波幅值 (V)
        :param harmonics: 谐波字典 {阶数: 幅值比例}, 如 {3: 0.05} 表示5%的三次谐波
        :param transient: 暂态配置, 如 {'type': 'sag', 'factor': 0.7} 
        :param noise_std: 噪声标准差
        """
        t = self.t_base + self._current_time_offset
        
        # 1. 生成基波
        actual_amp = amp
        if transient:
            if transient.get('type') == 'sag':
                actual_amp *= transient.get('factor', 0.8)
            elif transient.get('type') == 'swell':
                actual_amp *= transient.get('factor', 1.2)
        
        signal = actual_amp * np.sqrt(2) * np.sin(2 * np.pi * freq * t)

        # 2. 叠加谐波
        if harmonics:
            for order, ratio in harmonics.items():
                signal += (actual_amp * ratio * np.sqrt(2)) * np.sin(2 * np.pi * (freq * order) * t)

        # 3. 注入白噪声
        if noise_std > 0:
            signal += np.random.normal(0, noise_std, self.frame_size)

        # 4. 写入共享内存 (原子性写入)
        data_bytes = signal.astype(np.float32).tobytes()
        os.pwrite(self.fd, data_bytes, 0)
        
        # 更新时间偏移，确保波形连续
        self._current_time_offset += (self.frame_size / self.sample_rate)
        return signal

    def get_ground_truth_rms(self, signal: np.ndarray) -> float:
        """计算该帧的理论真值 RMS (用于比对)"""
        return float(np.sqrt(np.mean(np.square(signal))))

    def close(self):
        if self.mmap_obj: self.mmap_obj.close()
        if self.fd != -1: os.close(self.fd)