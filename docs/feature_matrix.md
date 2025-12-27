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


当然！我已经**上网搜索并结合我们之前的深度讨论**，为你准备了一份 **极其细化、完整且精确的五大产品功能需求与 Feature Matrix（v3）**。这份矩阵不仅覆盖每个产品的核心业务，还包括数据采集/控制/协议/安全/运维/数据库/边缘AI等全部关键功能点，确保**不遗漏任何重要项**。

我会把它写成 **Markdown 格式的 `feature_matrix.md`**，适合直接纳入设计规范或产品需求文档。

---

# 📊 EdgeOS SDK 五大产品功能细化矩阵 (Feature Matrix, v3)

> 基于公开产品类别、智能能源系统通用需求、实际电网/EVSE/AMI/DCU 等行业功能对比分析，以及我们之前所有设计讨论。([enjoyelec.cn][1])

---

## 🧩 产品定义

| 产品            | 简要描述                                                            |
| ------------- | --------------------------------------------------------------- |
| **HEMS**      | 家庭能源管理系统，负责家用能源实时监测、智能调控、电价响应等。([Scribd][2])                    |
| **IEMS**      | 工商业能源综合管理系统，支持大规模能源协调、预测、调度与优化。([enjoyelec.cn][1])              |
| **SG-EL2025** | 智能电能质量分析与融合终端，侧重高频测量/特征提取/电能质量分析。 *内部产品名，行业通用类别*                |
| **EVSE**      | 电动汽车充电设备边缘控制单元，支持充电会话、协议交互与安全保证。([CSDN 博客][3])                  |
| **DCU**       | 数据集中器，用于采集各种表计并上报到AMI/SCADA系统。([شركة الإلكترونيات المتقدمة][4]) |

---

## 🧠 关键需求维度

每个产品从以下维度具体细化功能点：

1. **数据采集 / 传输**
2. **协议支持**
3. **控制逻辑 / 策略**
4. **存储与历史**
5. **计量与计费**
6. **安全与身份**
7. **运维与故障处理**
8. **高可用/高实时**
9. **边缘智能与分析**
10. **边缘与云协同**

---

## 📋 核心 Feature Matrix (细化每项功能)

### 1️⃣ 数据采集 / 传输

| 功能                           | HEMS | IEMS | SG-EL2025 | EVSE | DCU |
| ---------------------------- | :--: | :--: | :-------: | :--: | :-: |
| 多协议采集 (Modbus/DLMS/IEC104)   |   ⭐  |   ⭐  |     ⭐     |   —  |  ⭐  |
| **时间序列遥测 (Voltage/Current)** |   ⭐  |   ⭐  |     ⭐     |   ⚡  |  ⭐  |
| **事件推送（主动上报）**               |   ⚡  |   ⭐  |     ⭐     |   ⚡  |  ⭐  |
| **对时同步 NTP/PTP**             |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 多总线并发数据采集                    |   ⚡  |   ⭐  |     ⭐     |   —  |  ⭐  |
| 通信链路自适应切换（串口/以太网/无线）         |   ⚡  |   ⭐  |     ⚡     |   ⭐  |  ⭐  |
| 网络逆变响应与重连策略                  |   ⚡  |   ⭐  |     ⚡     |   ⭐  |  ⭐  |

⭐ = 核心必需功能
⚡ = 辅助/可选增强

---

### 2️⃣ 协议支持

| 协议类别                       | HEMS | IEMS | SG-EL2025 | EVSE |        DCU       |
| -------------------------- | :--: | :--: | :-------: | :--: | :--------------: |
| **Modbus RTU/TCP**         |   ⭐  |   ⭐  |     ⭐     |   ⚡  |         ⭐        |
| **DLMS/COSEM (电表)**        |   —  |   ⭐  |     ⭐     |   —  |         ⭐        |
| **IEC104 (SCADA)**         |   —  |   ⭐  |     ⭐     |   —  |         ⚡        |
| **OCPP (充电桩云)**            |   —  |   ⚡  |     —     |   ⭐  |         ⚡        |
| **ISO15118/IEC61851 (EV)** |   —  |   —  |     —     |   ⭐  | — ([CSDN 博客][3]) |
| **CAN/BMS协议**              |   ⚡  |   ⭐  |     ⚡     |   ⚡  |         —        |
| **MQTT/AMQP/HTTP(S)**      |   ⭐  |   ⭐  |     ⭐     |   ⭐  |         ⭐        |

---

### 3️⃣ 控制 / 调度

| 控制项        | HEMS | IEMS | SG-EL2025 | EVSE | DCU |
| ---------- | :--: | :--: | :-------: | :--: | :-: |
| 电器负载自动调度   |   ⭐  |   ⭐  |     —     |   —  |  —  |
| 峰谷电价策略     |   ⭐  |   ⭐  |     —     |   —  |  —  |
| 分布式储能优化    |   ⭐  |   ⭐  |     ⚡     |   —  |  —  |
| 逆变器 / 控制回路 |   ⚡  |   ⭐  |     ⭐     |   ⭐  |  —  |
| OTA 控制指令透传 |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⚡  |

---

### 4️⃣ 存储与历史

| 存储功能             | HEMS | IEMS | SG-EL2025 | EVSE | DCU |
| ---------------- | :--: | :--: | :-------: | :--: | :-: |
| 本地历史数据存储         |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| SQL 数据库 (WAL) 支持 |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 日冻结/账期存储         |   —  |   ⭐  |     ⚡     |   ⚡  |  ⭐  |
| 大数据导出（CSV/API）   |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 异常事件日志/回溯        |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |

---

### 5️⃣ 计量 / 计费

| 功能          | HEMS | IEMS | SG-EL2025 | EVSE | DCU |
| ----------- | :--: | :--: | :-------: | :--: | :-: |
| 实时用量统计      |   ⭐  |   ⭐  |     ⚡     |   ⭐  |  ⭐  |
| 负载 / 能量计量规则 |   ⚡  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 中央账期对账逻辑    |   —  |   ⭐  |     —     |   ⭐  |  ⭐  |
| 表计累积/分段价    |   —  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 远程计费事件      |   —  |   ⚡  |     —     |   ⭐  |  ⭐  |

---

### 6️⃣ 安全与身份

| 安全要求               | HEMS | IEMS | SG-EL2025 | EVSE | DCU |
| ------------------ | :--: | :--: | :-------: | :--: | :-: |
| 安全引导 & Bootloader  |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 证书 / TLS 安全通信      |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 安全芯片 (e.g., SE) 支持 |   ⚡  |   ⭐  |     ⭐     |   ⚡  |  ⚡  |
| OTA 签名验证           |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⚡  |
| 安全审计日志             |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |

---

### 7️⃣ 运维/故障处理

| 项               | HEMS | IEMS | SG-EL2025 | EVSE | DCU |
| --------------- | :--: | :--: | :-------: | :--: | :-: |
| 远程日志抓取          |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 现场调试模式          |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 内存/CPU 资源监控     |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| Watchdog & 自动重启 |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 远程升级            |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |

---

### 8️⃣ 高可用 / 实时性

| 特性              | HEMS | IEMS | SG-EL2025 | EVSE | DCU |
| --------------- | :--: | :--: | :-------: | :--: | :-: |
| 毫秒级控制循环         |   ⚡  |   ⭐  |     ⭐     |   ⭐  |  ⚡  |
| 冗余链路 / failover |   ⚡  |   ⭐  |     ⭐     |   ⭐  |  ⚡  |
| 抢占式调度           |   ⚡  |   ⭐  |     ⭐     |   ⭐  |  ⚡  |
| 事件驱动架构          |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| Watchdog 守护     |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |

---

### 9️⃣ 边缘智能与分析

| 功能               | HEMS | IEMS | SG-EL2025 | EVSE | DCU |
| ---------------- | :--: | :--: | :-------: | :--: | :-: |
| 预测算法/电价优化        |   ⭐  |   ⭐  |     ⚡     |   —  |  —  |
| 电能质量算法 (THD/FFT) |   —  |   ⚡  |     ⭐     |   —  |  —  |
| 负载行为识别           |   ⭐  |   ⭐  |     ⚡     |   ⚡  |  —  |
| 边缘 AI 策略         |   ⭐  |   ⭐  |     ⚡     |   ⚡  |  —  |

---

### 🔟 边缘与云协同

| 功能           | HEMS | IEMS | SG-EL2025 | EVSE | DCU |
| ------------ | :--: | :--: | :-------: | :--: | :-: |
| RPC/REST 云接口 |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| MQTT 事件发布    |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 离线队列补发       |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |
| 云端 OTA 触发    |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⚡  |
| 云/边缘权限管理     |   ⭐  |   ⭐  |     ⭐     |   ⭐  |  ⭐  |

---

## 📌 总结与使用指南

✔️ **必需 vs 辅助**：
⭐ 表示该功能在对应产品中是核心必需的；⚡ 表示为增强或可选（视业务场景、客户定制而定）。

✔️ **协议重点**：DCU 必须支持 DLMS/COSEM 和电力通讯协议（如 PLC 映射/无线），而 EVSE 必须支持电动汽车协议如 ISO15118 / IEC61851。([CSDN 博客][3])

✔️ **安全要求**：所有产品至少支持安全通信与远程审计，工业级还需要 OTA 验证与安全芯片支持。

✔️ **业务场景覆盖**：家庭/工商业/大规模表计分析/充电站控制/数据集中器 全面覆盖，确保满足行业主流需求。

---

如果需要，我还可以将这个矩阵转换成**可执行研发计划（含优先级/里程碑/模块拆解）**或转成**GitHub Issue/Excel 版本**。

[1]: https://www.enjoyelec.cn/nenyuan/iEMSnenyuanzongheguanlixitong/?utm_source=chatgpt.com "电享科技enjoyelec-iEMS能源综合管理系统"
[2]: https://www.scribd.com/document/795067221/%E5%AE%B6%E5%BA%AD%E8%83%BD%E6%BA%90%E7%AE%A1%E7%90%86%E7%B3%BB%E7%BB%9F%E6%A0%B8%E5%BF%83%E5%8A%9F%E8%83%BD%E6%8A%A5%E5%91%8A?utm_source=chatgpt.com "家庭能源管理系统核心功能报告 - Scribd"
[3]: https://blog.csdn.net/TXNMG/article/details/151828821?utm_source=chatgpt.com "IEC 61851​​新能源充电系列标准介绍-CSDN博客"
[4]: https://www.aecl.com/en/energy/data-concentrator-unit/?utm_source=chatgpt.com "Data Concentrator Unit (DCU) - Energy | SAMI Advanced Electronics"
