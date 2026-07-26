# EdgeC 40 项全景架构增强与持续交付实施计划 (40 Architecture Enhancement Plan)

> **版本**：v2.0  
> **日期**：2026-07-25  
> **执行团队**：Planner + Coders + Tester + Reviewer (SmartBox & EdgeC Core Squad)  
> **最高指挥**：Mr. WKT (老板)  
> **适用产品**：EdgeC / SmartBox-Alpha (智能康复盒子)  

---

## 一、 第一批 20 项架构增强 (Points 1 - 20)

### 阶段一：L0/L1 契约总线与零拷贝 IPC 增强
- **1.1 相对指针 SHM 引擎** (`modules/edge_core/include/shmring.h`): `shmring_abs_to_rel` 与 `shmring_rel_to_abs` 首地址无关偏移指针。 *(Status: IMPLEMENTED & TESTED)*
- **1.2 Dynamic Profile 运行期点表热加载** (`modules/edge_core/src/datahub.c`): `datahub_switch_schema` CAS 热切换。
- **1.3 SQLite 级联 WAL 与离线同步队列** (`modules/edge_core/src/sqlite_service.c`): `synced INT DEFAULT 0` 断网持久化与重放。 *(Status: IMPLEMENTED & TESTED)*
- **1.4 DeviceTwin 离线影子状态机** (`modules/edge_core/src/datahub.c`): `datahub_devicetwin_reconcile` 期望状态与实际报告状态对齐。 *(Status: IMPLEMENTED & TESTED)*

### 阶段二：L2 边缘算力与流 DAG 处理引擎
- **2.1 100Hz 拓扑滑动窗口算子** (`modules/edge_stream`): 20ms 卡尔曼滤波与姿态抖动抑制。
- **2.2 WAMR 沙箱资源看门狗** (`modules/edge_stream/src/wasm_runtime`): 10ms 调度超时打断与自愈。
- **2.3 二进制 Protobuf / TLV IPC 适配器** (`cmd/smart_box/main.c`): 替换高频 cJSON 解析。
- **2.4 Zephyr Driver Model 规范化解耦** (`modules/hal`): 统一 `struct device` 接口规范。

### 阶段三：L3 管理平面与组网/分发增强
- **3.1 mDNS 本地 P2P 去中心化组网** (`go/cmd/smart-box-gateway`): 无互联网局域网对决组网。
- **3.2 BLE Plugin Slot 动态驱动加载桶** (`go/internal/ble`): 扫码自动加载外设描述符。
- **3.3 内存 Chunk 日志积攒压缩** (`go/internal/logger`): 64KB 批次 Gzip 写盘，保护 Flash 寿命。
- **3.4 远程医生 Canvas 触控坐标注入** (`go/internal/prescription`): `/api/v1/remote/screen` WS 触控注入。

### 阶段四：L4 执行平面与硬件安全防护
- **4.1 Zephyr DeviceTree 编译期 Static Linting** (`scripts/dts_lint.py`): 引脚复用强校验。
- **4.2 eBPF / Socket Filter SOS 报文打标** (`modules/edge_security`): SOS 紧急帧 DSCP 高优先级打标。
- **4.3 MCUBoot 双 Slot 安全 OTA 与自动回滚** (`cmd/smart_box`): Trial-run 试运行校验与退回。
- **4.4 SPSC 无锁单生产者单消费者环形队列** (`cmd/smart_box/main.c`): GPIO IRQ 与 AXP209 采集无锁传递。

### 阶段五：边缘 AI 与自愈监控
- **5.1 MicroTVM / ONNX C 代码跌倒模型量化** (`modules/edge_algo`): 跌倒预估 C 算子推理 < 5ms。
- **5.2 MbedTLS 硬件 KeyStore 密钥绑定** (`modules/edge_security`): ATECC608A 加密芯片对接。
- **5.3 Chrome DevTools Protocol 远程 HMI 诊断** (`web/products/smart_box`): 4K 屏帧率与 Console 捕获。
- **5.4 Proactive Health Score Matrix 自愈健康状态机** (`modules/edge_core/src/arbiter.c`): 多维健康打分与子服务拉起。

---

## 二、 第二批 20 项架构增强 (Points 21 - 40)

### 阶段六：边缘实时总线与微服务 IPC (Points 21 - 24)
- **6.1 Eclipse Zenoh-Pico 零损耗 Pub/Sub/Query 数据路由** (`modules/edge_core/src/datahub.c`): 增加 Query 快照拉取通道。
- **6.2 PX4 uORB 无锁多订阅者异步消息代理** (`cmd/smart_box/main.c`): 姿态/心率事件多算子并发订阅。
- **6.3 OpenWrt ubus/blobmsg 二进制 IPC 协议** (`modules/edge_io`): TLV 嵌套结构替代 JSON 序列化。
- **6.4 Open62541 OPC UA PubSub TSN 实时以太网发布** (`cmd/smart_box`): `SO_PRIORITY` 独占确定性时隙。

### 阶段七：边缘 AI 与 WASI-NN 神经算子 (Points 25 - 28)
- **7.1 WasmEdge WASI-NN 标准硬件加速接口** (`modules/edge_stream/src/wasm_runtime`): 规范 Wasm 算子调用 NPU ABI。
- **7.2 OpenVINO NPU 零拷贝缓冲区** (`modules/edge_stream`): 摄像头 DMA-BUF 直映射入张量内存。
- **7.3 TinyMaix 极简 MCU 级 CNN 推理库** (`modules/edge_algo`): 低算力芯片单 C 文件 8-bit 量化引擎。
- **7.4 TensorRT Dynamic Shape 动态张量形状调度** (`modules/edge_algo`): 多路 BLE 外设自适应变维输入。

### 阶段八：传感器子系统与物理驱动 (Points 29 - 32)
- **8.1 Linux IIO Subsystem 硬件触发环形缓冲区** (`cmd/smart_box/main.c`): IMU 中断替代 Timer 轮询（CPU < 0.1%）。
- **8.2 PX4 EKF2 24-State 扩展卡尔曼融合引擎** (`modules/edge_algo`): 患者重心轨迹与平稳度评估。
- **8.3 libmodbus 零分配 Sliding Window CRC 池** (`modules/edge_io`): 115200 串口满帧率不丢帧解析。
- **8.4 Android HAL (hardware/interfaces) 风格服务注册** (`modules/hal`): 摄像头/蓝牙/串口驱动插件化注册。

### 阶段九：边缘安全、日志与死信重试 (Points 33 - 36)
- **9.1 Vector Dead-Letter Queue (DLQ) 失败补偿** (`go/internal/prescription`): 云端同步 DLQ 隔离与指数退避。
- **9.2 OpenWrt ubox logd 内存环日志与 dmesg 快照** (`modules/edge_core/src/edge_log.c`): 512KB 崩溃前日志 Dump。
- **9.3 Linux Landlock / SECCOMP-BPF 内核沙盒** (`modules/edge_security`): 第三方 HMI 进程 syscall 强隔离。
- **9.4 WireGuard Noise Protocol 极简 P2P 隧道** (`go/cmd/smart-box-gateway`): 远端医生无感直连局域网盒子。

### 阶段十：可观测性、数字孪生与仿真 (Points 37 - 40)
- **10.1 ROS2 Rosbag2 二进制全速归档与回放** (`cmd/smart_box`): `datahub_record` / `datahub_replay` 康复过程复现。
- **10.2 ThingsBoard Telemetry 状态变化死区判定 (Deadband)** (`modules/edge_core/src/datahub.c`): 过滤无意义微小波动写盘。
- **10.3 SigNoz / eBPF 无侵入链路追踪 (TraceID)** (`go/cmd/smart-box-gateway`): 端到端传感器到 4K 屏渲染延时打标。
- **10.4 NuttX System Power Manager 分级低功耗休眠** (`cmd/smart_box/main.c`): 闲置 10 分钟屏幕休眠、扫码 50ms 秒级唤醒。

---

## 验收与质量闭环

1. **Workflow 标准流程**：Planner 契约锁定 -> Coders 代码移植 -> Tester 自动化测试与 Omnis 物理仿真 -> Reviewer 点表一致性校验。
2. **零抽象与零 Mock 铁律**：所有实现必须基于 Linux 物理驱动、真实数据结构与可用编译依赖。
3. **点表一致性命令**：`./scripts/check_tag_consistency.sh` 始终保持 `PASSED`。
