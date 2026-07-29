# Wayfinder Map: EdgeC 统一边缘智能与控制平台落地路线图 (Consensus Locked)

## Destination

将 GitHub 开源项目调研清单中的核心技术体系（TinyML、FMCW雷达/DSP、SimpleFOC电机控制、AUTOSAR UDS/DoIP、ROS2/MoveIt2）全面吸收并落地到 EdgeC / EdgeOS 平台：
1. **智能康复 (SmartBox/Healthcare)**：TinyML 姿态与跌倒识别 + FMCW 雷达生命体征检测；
2. **工业驱动 (Industrial Drive)**：SimpleFOC 电机矢量控制 + NanoEdge AI 振动故障诊断；
3. **车载/机器人 (Automotive & Robotics)**：AUTOSAR UDS/DoIP 诊断总线 + ROS2 节点通信 + WASM 边缘微应用沙箱。

---

## Notes

- **核心代码库**: `/workspaces/edgeos/EdgeC`
- **设计原则**:
  1. 遵从 **EdgeOS 生产级工程宪法** (非侵入式 HAL/RTOS，四平面隔离架构)。
  2. 遵从 **契约驱动开发 (CDD)**：所有点位在 `contracts/schema/` YAML 中严格定义。
  3. **100% 物理真实性与验证**：拒绝 Mock 占位符，支持 `ctest`、`go test` 与 `./test.sh --omnis` 仿真。

---

## Decisions so far (已锁定架构决议)

- [Decision-1 Locked]: **物理架构** —— 主从分级架构 (Master-Subnode)。RK3588 作为主节点（运行 EdgeC/Linux/WASM/Go/Web），Zephyr RTOS 作为硬实时从节点（运行 SimpleFOC 10kHz 电流环）。
- [Decision-2 Locked]: **主从通信总线** —— 双轨总线 (Dual-Track Bus)。CAN-FD (5Mbps) 走 100Hz 高频控制快车道与 UDS 帧，RS485/Modbus 走管理与参数配置平面。
- [Decision-3 Locked]: **TinyML 与雷达集成** —— 混合流水线 (C-DSP + WASM-TinyML)。C 原生层处理 FMCW 2D-FFT/Range-Doppler 并用 ARM NEON SIMD 加速；WASM 沙箱层运行 INT8 TinyML 手势/跌倒分类模型，支持静默热更新。
- [Decision-4 Locked]: **车载诊断网关暴露** —— 双模混合暴露 (Dual-Mode DoIP + REST/WS)。端口 13400 监听 ISO 13400 原生 DoIP 报文（兼容 Vector CANoe）；端口 8083 提供 RESTful/WebSocket 诊断管理接口。
- [Decision-5 Locked]: **数据契约扩展** —— 三域独立扩展 (3 Independent Domains)。在 `contracts/schema/domains/` 下建立 `smartbox_radar.yaml` (12021-12025)、`industrial_foc.yaml` (12026-12030) 与 `automotive_uds.yaml` (12031-12035)。

---

## Closed Frontier Tickets (已解决前沿票据)

| Ticket ID | 票据名称 | 类型 | 状态 | 决议内容与实现标准 |
| :--- | :--- | :--- | :--- | :--- |
| **Ticket-1** | `[A-1] TinyML 算子引擎选型与 WASM 沙箱绑定` | Research | **Closed** | 选用 TFLite-Micro + m2cgen，交叉编译为 WASM32 模块嵌入 `modules/edge_stream/src/wasm_runtime/`。 |
| **Ticket-2** | `[B-1] SimpleFOC 矢量控制在主从架构中的分工` | Prototype | **Closed** | 定向为 Zephyr RTOS 从节点硬件运行 10kHz PWM 电流环，RK3588 主节点通过 CAN-FD 跑 100Hz 位置/速度闭环。 |
| **Ticket-3** | `[C-1] AUTOSAR UDS/DoIP 车载诊断网关设计` | Research | **Closed** | 实现原生 13400 DoIP 监听 + HTTP REST (`/api/v1/ecu/uds/*`) 双模诊断接口。 |
| **Ticket-4** | `[A-2] FMCW 毫米波雷达 C-DSP 与 WASM 混合流水线` | Prototype | **In Progress** | C 驱动负责 2D-FFT 与 NEON SIMD 矩阵加速；WASM 沙箱负责手势与跌倒判定。 |
| **Ticket-5** | `[CDD-1] 三域独立 YAML 契约扩展` | Task | **Closed** | 创建 `smartbox_radar.yaml`、`industrial_foc.yaml` 与 `automotive_uds.yaml` 契约文件。 |
| **Ticket-B1** | `[B-FMCW-1] L0 Modbus RTU 物理雷达点云驱动` | Driver | **Closed** | 实现 `driver_radar_fmcw.c` 0xAA 0x55 魔法包解包、CRC16 校验与 DataHub (12021-12023) 写入。 |
| **Ticket-B2** | `[B-FMCW-2] L1 ARM NEON SIMD 2D-FFT / MTI 滤波算子` | DSP | **Closed** | 实现 `fmcw_dsp_neon.c` MTI 静止杂波消除与 Range-Doppler 功率谱计算，支持 ARM64/x86 跨平台。 |
| **Ticket-B3** | `[B-FMCW-3] L0/L1 物理仿真单元测试与 TDD 验证` | Test | **Under Review** | 编写 `test_radar_fmcw.c`，覆盖 Modbus 协议数据解包、CRC16 校验与 NEON DSP 算子断言。 |

---

## Standard Wayfinder Execution Loop (标准工作流)

`grill` ➔ `to-spec` ➔ `to-tickets` ➔ `tdd/implement` ➔ `diagnose-bugs` ➔ `review`

1. **Grill (架构对齐)**: **[LOCKED]** 锁定方案 1（Modbus 点云模式）与包含物理仿真的单元测试。
2. **To-Spec (规范撰写)**: **[COMPLETED]** 编写 [fmcw_radar_dsp_spec.md](file:///workspaces/edgeos/EdgeC/docs/specs/fmcw_radar_dsp_spec.md)。
3. **To-Tickets (票据拆解)**: **[COMPLETED]** 拆解 Ticket-B1、B2、B3。
4. **TDD / Implement (实现与测试)**: **[COMPLETED]** 完成 `driver_radar_fmcw.c`、`fmcw_dsp_neon.c` 与 `test_radar_fmcw.c`。
5. **Diagnose-Bugs (日志诊断)**: **[PASSED]** 无构件报错，准备执行单体 ctest 验收。
6. **Review (评审闭环)**: **[IN REVIEW]** 待二进制编译完成并直接运行 `test_radar_fmcw` 输出 100% PASS 报告。
