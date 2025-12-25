# EdgeOS Product Feature Matrix (v2.0)

本文档详细定义了 EdgeOS 平台支撑的五大核心产品的功能需求，并将其深度映射到当前架构中的核心**角色 (Roles)**：`Arbiter` (总线仲裁), `IO Executor` (驱动执行), `DataHub` (数据总线), `Algorithm` (计算引擎), `Storage` (持久化), `Northbound` (云端交互)。

## 1. 产品定义与画像 (Product Portraits)

| 产品 | 全称 | 核心定位 | 硬件画像 (典型) | 关键痛点 |
| :--- | :--- | :--- | :--- | :--- |
| **HEMS** | Home Energy Management | **连接**：家电互联、能耗可视化 | ARM A7, 256MB RAM, WiFi | 成本敏感，用户体验要求高 |
| **IEMS** | C&I Energy Management | **控制**：削峰填谷、需量管理 | ARM A55 (4-Core), 1GB RAM, 8xRS485 | **强实时性** (100ms闭环)，总线竞争激烈 |
| **SG-EL2025** | Smart Grid Fusion | **融合**：营配贯通、台区智能 | A55+M33 (异构), NPU, FPGA | **国网标准**，高频AC采样，国密安全 |
| **DCU** | Data Concentrator | **存储**：集抄、冻结数据 | ARM A9, 512MB RAM, HPLC | **海量存储** (10年数据)，协议转换复杂 |
| **EVSE** | EV Supply Equipment | **交易**：计费、车网互动(V2G) | ARM A7, SE芯片, 4G | **数据一致性** (涉及金钱)，断网保护 |

---

## 2. 功能需求与角色映射矩阵 (Feature-to-Role Mapping)

### 2.1 角色：Arbiter (总线仲裁器) & IO Executor (驱动执行)
**核心职责**：管理物理总线资源，执行具体的协议驱动。

| 特性需求 | HEMS | IEMS | SG-EL2025 | DCU | EVSE | 角色行为 (Implementation) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **总线并发控制** | Low | **Critical** | **Critical** | Medium | High | **Arbiter**: IEMS中PCS调节与BMS轮询冲突时，优先保障PCS指令 (Priority=Realtime)。 |
| **Modbus RTU** | Basic | **Master** (PCS/BMS) | **Master** (Sensors) | Master (Aux) | Master (Meter) | **IO Executor**: 需支持散乱寄存器的自动聚合读取 (Batch Read)。 |
| **DLMS/COSEM** | - | - | - | **Client** (Meters) | - | **IO Executor**: 需实现复杂的 OBIS 编码解析和 HLS 认证。 |
| **Q/GDW 376.2** | - | - | **Master** (HPLC) | **Master** (HPLC) | - | **IO Executor**: 对接 CCO 模块，处理电力线载波组网事件。 |
| **IEC 61850** | - | - | **MMS/GOOSE** | - | - | **IO Executor**: 需映射到 M33 核心或 FPGA 处理以保证 <3ms 延时。 |
| **CAN Bus** | - | **J1939/BMS** | - | - | **BMS/GB27930** | **IO Executor**: 处理 ISO 15765 多帧传输，EVSE 需处理充电握手时序。 |
| **即插即用** | WiFi Config | Static | **Udev/Hotplug** | Topology Scan | - | **Arbiter**: 监听 Udev 事件，动态生成 IO Job。 |

### 2.2 角色：DataHub (shm_ring 数据总线)
**核心职责**：无锁、零拷贝的数据分发与交换中心。

| 特性需求 | HEMS | IEMS | SG-EL2025 | DCU | EVSE | 角色行为 (Implementation) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **遥测 (Telemetry)**| 1s | **100ms** | **10ms** | 15min | 200ms | **DataHub**: IEMS 和 SG 场景下开启 `PIPEMSG_REF` 模式，传递指针而非大块数据。 |
| **波形数据 (AC)** | - | - | **4kHz** (Harmonics)| - | - | **DataHub**: SG-EL2025 专用通道，传递 `dma_buf` 句柄给 DSP 算法。 |
| **事件 (Events)** | Info | Alarm | **SOE** (1ms) | Alarm | Transaction | **DataHub**: SOE 事件需带高精度时间戳，优先级高于普通遥测。 |
| **跨核通信** | - | - | **AMP** (A-M Core) | - | - | **DataHub**: 映射到 RPMSG 共享内存区域，实现异构核数据交换。 |

### 2.3 角色：Algorithm (计算聚合引擎)
**核心职责**：数据清洗、业务逻辑计算、策略执行。

| 特性需求 | HEMS | IEMS | SG-EL2025 | DCU | EVSE | 角色行为 (Implementation) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **基础计算** | Sum/Avg | SOC/SOH | RMS/Power | Aggregation | Energy/Cost | **Algorithm**: 监听 DataHub，触发 `TinyExpr` 表达式计算。 |
| **高级策略** | Scene | **Peak-Shaving** | **Line Loss** | - | Load Balance | **Algorithm**: IEMS 需运行 Lua/C 策略脚本，根据分时电价控制充放电。 |
| **边缘 AI** | - | - | **Load ID** (NILM) | - | - | **Algorithm**: 调用 NPU 接口进行非侵入式负荷辨识。 |
| **死区/过滤** | Yes | Yes | Yes | - | - | **Algorithm**: 实现迟滞比较器，减少无效数据上报。 |

### 2.4 角色：Storage (SQLite 服务)
**核心职责**：数据持久化、历史查询、掉电保护。

| 特性需求 | HEMS | IEMS | SG-EL2025 | DCU | EVSE | 角色行为 (Implementation) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **时序存储** | 7天 | 3个月 | 1年 | **10年+** | 6个月 | **Storage**: DCU 场景需分表存储 (按月/年)，启用 `WAL` 模式。 |
| **冻结数据** | - | - | Curve | **Freeze (Day/Mon)**| - | **Storage**: 专门的表结构设计，支持补冻、重冻逻辑。 |
| **事务完整性** | Low | High | High | Medium | **Critical** | **Storage**: EVSE 计费记录必须 `fsync`，确保掉电不丢金额。 |
| **写入频率** | Low | High | Very High | Burst (00:00) | Medium | **Storage**: 使用 `uv_queue_work` 批处理写入，DCU 需处理零点写入风暴。 |

### 2.5 角色：Northbound (北向导出引擎)
**核心职责**：云端连接、协议转换、断点续传。

| 特性需求 | HEMS | IEMS | SG-EL2025 | DCU | EVSE | 角色行为 (Implementation) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **协议栈** | MQTT | MQTT + **104** | MQTT + **CoAP** | **376.1** | **OCPP** | **Northbound**: 插件化架构，支持多协议栈并发运行。 |
| **断点续传** | RAM | **Disk (SQLite)**| Disk | Disk | Disk | **Northbound**: 网络中断时数据写入 Storage 缓存表，恢复后 FIFO 上送。 |
| **多主站** | No | Yes (Cloud+Grid)| Yes | Yes (Marketing)| Yes (Gov+Op) | **Northbound**: 支持多通道 (Channels)，数据分流。 |
| **安全链路** | TLS | TLS/VPN | **国密 SSL** | 国密 | TLS | **Northbound**: 集成 SE 芯片驱动，使用硬件证书建立连接。 |

---

## 3. 安全体系 (Security Context)

| 维度 | 需求详情 | 涉及角色 |
| :--- | :--- | :--- |
| **数据加密** | 敏感数据 (电费、控制指令) 必须使用 **SM4** 加密存储。 | Storage, IO Executor |
| **传输加密** | 上行通信必须通过 **SM2/SM3** 双向认证。 | Northbound |
| **启动信任** | 必须实现 U-Boot 到 Kernel 到 Rootfs 的 **Secure Boot** 链。 | OS / BSP |
| **应用鉴权** | 动态加载的 `.so` 驱动或算法必须有官方 **数字签名**。 | Driver Manager, Algorithm |

## 4. 关键技术指标 (KPIs)

*   **IEMS 响应时间**: 从 BMS 告警到 PCS 停机 < **100ms**。
*   **SG-EL2025 授时精度**: 对时误差 < **1us** (依靠 FPGA/PTP)。
*   **DCU 抄表成功率**: 日冻结数据采集成功率 > **99.9%**。
*   **EVSE 计费误差**: 充电影响金额误差 < **0.01 元**。
*   **系统寿命**: Flash 写入寿命优化，支持 **10年** 运行 (磨损均衡 + 减少写入)。