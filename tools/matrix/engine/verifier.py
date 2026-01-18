import logging
from typing import Dict, List, Any
import numpy as np

logger = logging.getLogger("Matrix.Verifier")

class AlgorithmVerifier:
    """
    算法精度校验基类：负责将 C 侧结果与 Python 理论真值进行比对。
    """
    def __init__(self, name: str, tolerance: float = 0.01):
        self.name = name
        self.tolerance = tolerance
        self.history: List[Dict] = []

    def verify(self, c_value: float, py_ref: float) -> bool:
        error = abs(c_value - py_ref)
        relative_error = error / abs(py_ref) if py_ref != 0 else error
        
        passed = relative_error <= self.tolerance
        
        result = {
            "c_val": c_value,
            "py_ref": py_ref,
            "error": error,
            "rel_error": relative_error,
            "passed": passed
        }
        self.history.append(result)
        
        if not passed:
            logger.error(f"❌ [{self.name}] Accuracy Fail! C: {c_value:.4f}, Py: {py_ref:.4f}, Error: {relative_error:.2%}")
        else:
            logger.info(f"✅ [{self.name}] Accuracy Pass. Error: {relative_error:.4%}")
            
        return passed

class RMSVerifier(AlgorithmVerifier):
    """
    专门针对有效值 (RMS) 算法的校验器。
    """
    def __init__(self, tolerance: float = 0.005): # 默认 0.5% 精度要求
        super().__init__("RMS_Calculator", tolerance)

    def calculate_reference(self, raw_buffer: np.ndarray) -> float:
        """Python 侧的高精度有效值实现"""
        return float(np.sqrt(np.mean(np.square(raw_buffer))))
