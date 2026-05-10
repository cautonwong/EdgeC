# EdgeC SDK 工业化之路：后续开发路线图 (ROADMAP)

## 1. 数学与算法基座 (Math & Algos) - 持续整顿
- [ ] **LQR 求解器强化**：在 `control_algo.c` 中集成代数 Riccati 方程 (ARE) 的数值解法，利用 GSL 的 Schur 分解实现。
- [ ] **MPC 工业级补强**：引入轻量级二次规划 (QP) 求解器（如 OSQP 或 qpOASES），支持多约束条件下的最优调度。
- [ ] **2D FFT 完备化**：补齐 `dsp_fft2d_c2c` 的列处理逻辑（涉及矩阵转置优化），支持完整的雷达 Range-Doppler Map。
- [ ] **CFAR 目标检测**：在 `nexus_lab` 中实现恒虚警 (Constant False Alarm Rate) 算法，用于雷达点云的噪声过滤与目标提取。

## 2. 协议栈深耕 (Protocols) - 稳扎稳打
- [ ] **DLMS 证书与安全**：在 `dlms_security.c` 中落地 AES-GCM 128 加密流，支持电表的数据传输安全。
- [ ] **DLT698 递归解析器**：利用 `dlms_decoder` 的设计模式，重构 698 的 Data 结构解析，支持复杂的 Object 定义。
- [ ] **IEC104 1+1 冗余**：实现双链路冗余切换逻辑，满足电网对高可用性的硬性要求。
- [ ] **CANOpen 映射**：在 `driver_can.c` 基础上，封装标准的 CANOpen SDO/PDO 映射层，支持机器人电机控制。

## 3. 计算平面与业务落地 (Compute & Products) - 降维打击
- [ ] **`robot_ctrl` 实战**：基于 `edge_linalg` 编写 6 轴机械臂的逆运动学算子 (Inverse Kinematics)。
- [ ] **`smart_storage` 调度**：将 MPC 算子集成进微电网 Pipeline，实现峰谷平抑逻辑。
- [ ] **FOC 闭环控制**：在 `StreamEngine` 中编写一个 20kHz 频率的高速 FOC Stage，挑战 Linux 实时性能极限。

## 4. 平台与生态 (Ecosystem) - 跨界整合
- [ ] **Zephyr 深度适配**：将 `edge_linalg` 对接到 ARM CMSIS-DSP，验证在 STM32/Vango 芯片上的运行效率。
- [ ] **Docker 标准化部署**：更新 `Dockerfile.prod`，将 GSL/OpenBLAS 等数学依赖打包进镜像。
- [ ] **Web 看板增强**：在 UI 端增加“波形图”和“频谱图”组件，直观展示 FFT 和 Kalman 的运算结果。

---
*字典在手，工业无忧。EdgeC 核心已成，万物皆可计算。*
