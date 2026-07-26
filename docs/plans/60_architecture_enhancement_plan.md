# EdgeC 60 项全景架构增强与业务场景落地实施计划 (60 Architecture & Business Enhancement Plan)

> **版本**：v3.0  
> **日期**：2026-07-25  
> **执行团队**：Planner + Coders + Tester + Reviewer (SmartBox & EdgeC Core Squad)  
> **最高指挥**：Mr. WKT (老板)  
> **适用产品**：EdgeC / SmartBox-Alpha (智能康复盒子)  

---

## 一、 架构增强 (Points 1 - 40)

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

### 阶段六：边缘实时总线与微服务 IPC (Points 21 - 24)
- **6.1 Eclipse Zenoh-Pico 零损耗 Pub/Sub/Query 数据路由** (`modules/edge_core/src/datahub.c`): `datahub_query_snapshot` 瞬态快照 API。 *(Status: IMPLEMENTED & TESTED)*
- **6.2 PX4 uORB 无锁多订阅者消息代理** (`cmd/smart_box/main.c`): 姿态/心率事件多算子并发订阅。
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

## 二、 康复业务场景与医疗标准 (Points 41 - 60)

### 阶段十一：肌电与姿态康复业务场景 (Points 41 - 44)
- **11.1 OpenBCI sEMG 表面肌电生物反馈与游戏控制** (`modules/edge_algo`): 带通滤波 + RMS 肌电疲劳与收缩力控制。
- **11.2 MediaPipe 关节活动度 (ROM) 自动测定与轨迹比对** (`modules/edge_stream`): 3D 向量夹角计算与 ROM 阻尼介入。
- **11.3 ROS2 Control 跌倒防碰撞主动阻尼防护 (E-Stop)** (`cmd/smart_box/main.c`): 失稳瞬间 2ms 内切断外骨骼动力。
- **11.4 HL7 FHIR 国际医疗标准电子病历映射** (`go/internal/prescription`): 生成标准 Observation & CarePlan JSON。

### 阶段十二：数字孪生与实时消息增强 (Points 45 - 48)
- **12.1 Eclipse Ditto 物理实体孪生与多端镜像** (`go/cmd/smart-box-gateway`): WebSocket `/api/v1/twin` 一键拉取实体全貌。
- **12.2 MQTT-SN 超低功耗无线节点离线 QoS 2** (`modules/edge_protocol`): Topic ID 代替字符串 Topic 降低广播开销。
- **12.3 PLC4X 通用工业 PLC 协议适配框架** (`modules/edge_io`): 一键连接 Siemens S7 / 工业气动康复设备。
- **12.4 Project Matter / Thread 智能家居健康组网** (`go/cmd/smart-box-gateway`): 报警时室内智能灯光联动变红。

### 阶段十三：功耗控制、防磨损与性能监控 (Points 49 - 52)
- **13.1 Zephyr DVFS 动态电压与频率调节** (`cmd/smart_box/main.c`): 闲置自动降频至 24MHz（功耗降低 85%）。
- **13.2 Flash Wear-Leveling 闪存擦写均衡与页保护** (`modules/edge_core/src/sqlite_service.c`): NVS 环形擦写，保障 10 年不损坏。
- **13.3 Telegraf Tag Metric Bucket 测点聚合降维** (`modules/edge_core/src/datahub.c`): 1 分钟/5 分钟 min/max/avg/p95 聚合。
- **13.4 eBPF USDT 静态追踪埋点** (`modules/edge_core/src/shmring.c`): NOP 埋点零开销诊断微秒级延迟。

### 阶段十四：边缘安全、音视频与人机交互 (Points 53 - 56)
- **14.1 WebRTC Embedded Native 实时视频对讲** (`go/cmd/smart-box-gateway`): 远端医生 1080P 零延迟视频看护。
- **14.2 Whisper.cpp 离线极简语音识别** (`modules/edge_stream/src/wasm_runtime`): 脱机“开始训练/救命/暂停”命令词识别。
- **14.3 OpenCV ArUco 标记跟踪与肢体动作矫正** (`modules/edge_algo`): 低成本贴纸标签 3D 姿态角追踪。
- **14.4 SPI Flash Dual-Bank A/B 升级与硬件看门狗** (`cmd/smart_box/main.c`): `/dev/watchdog` 定时喂狗与死锁硬件重启。

### 阶段十五：边缘微服务与部署管理 (Points 57 - 60)
- **15.1 K3s Light 微服务镜像 Patch 增量分发** (`go/cmd/smart-box-gateway`): Wasm 算子仅传输 Diff Patch 文件。
- **15.2 OpenTofu / Terraform 边缘基础设施批量配置** (`go/cmd/smart-box-gateway`): 声明式 YAML 一键配置 100 台 SmartBox。
- **15.3 SQLite R*Tree 空间索引与位置邻近告警** (`modules/edge_core/src/sqlite_service.c`): 检索家具碰撞区域与防跌倒边界。
- **15.4 NATS JetStream 持久化边缘消息队列** (`go/cmd/smart-box-gateway`): 游标重放与 Exactly-Once 消息投递。

---

## 三、 质量验证与持续交付规范

1. **Workflow 标准规程**：依照 [team_workflow_pipeline.md](file:///workspaces/edgeos/EdgeC/docs/plans/team_workflow_pipeline.md) 执行 Planner 锁定 -> Coders 编码 -> Tester 仿真 -> Reviewer 审查。
2. **点表一致性校验**：`./scripts/check_tag_consistency.sh` 始终保持 `PASSED`。
3. **零 Mock 物理原则**：基于真实内核驱动、I2C/SPI 总线、DataHub 影子矩阵。
