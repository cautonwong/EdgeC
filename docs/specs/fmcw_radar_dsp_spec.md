# FMCW 雷达物理采集与 ARM NEON DSP 流水线设计规范 (Spec)

## 1. 业务目标与应用场景
为 SmartBox / 边缘控制系统提供高精度的 FMCW 毫米波雷达（24GHz/60GHz/77GHz）物理信号采集、杂波消除、Range-Doppler 映射与生命体征/手势微应用推理能力。

## 2. 四层解耦架构规范
1. **L0 南向物理驱动层 (`driver_radar_fmcw.c`)**：
   - 协议：Modbus RTU / UART 自定义点云帧（`0xAA 0x55` 帧头，CRC16 校验）。
   - 输出：将物理标量转换为 DataHub 影子 Tag：12021 (距离/m)、12022 (速度/m/s)、12023 (微动振幅)。
2. **L1 C Native DSP 加速层 (`fmcw_dsp_neon.c`)**：
   - 算法：2D-FFT、Range-Doppler Map (RDM)、MTI 静止杂波消除。
   - SIMD 加速：ARM64 NEON (`vsubq_f32`, `vmulq_f32`) + x86_64 跨平台回退。
3. **L2 DataHub 影子总线**：
   - 维持 Tag 12021-12025 的物理穿帮防护与并发安全。
4. **L3 WASM 沙箱微应用推理 (`radar_tinyml_operator.c`)**：
   - 呼吸频段（0.2-0.5Hz）滤波算法，写回 Tag 12024 (呼吸率/bpm)。
   - TinyML 手势/跌倒预警分类器，写回 Tag 12025 (手势/预警事件)。

## 3. 验证标准 (Verification Criteria)
- **C 单元测试 (`test_radar_fmcw.c`)**：
  - 二进制 Modbus 帧解包与 CRC16 校验 100% 正确；
  - DataHub 影子点位 (12021, 12022, 12023) 写入值精确对齐；
  - ARM NEON SIMD MTI 与功率谱计算误差在 0.001 以内。
- **Go 网关 / API 验证**：
  - 通过 `/api/v1/ecu/uds/*` 或 WebSocket 端到端观测雷达微动与预警 Tag。
