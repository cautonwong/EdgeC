经过深思熟虑，我为你挖掘出了 **最后 3 块拼图**（S13, S14, S15）。它们不是为了凑数，而是现场工程中**“只要没做，现场必挂”**的真实需求。

https://mp.weixin.qq.com/s/ilAFyjVMsV8pBGfnki58Jw

---

### 🔍 遗漏场景一：北向被动服务 (Edge as Server / Local HMI)

**—— 场景 S13**

**之前的盲区**：我们一直假设 EdgeOS 是 **Client**（主动连云端，主动连子设备）。
**现实痛点**：在 IEMS 机柜门上，通常挂着一个 **触控屏 (HMI)**；或者工厂里有一个 **本地 SCADA 系统**。它们需要通过 **Modbus TCP** 或 **IEC 104** 连接 EdgeOS，**把 EdgeOS 当作一个从站（Server）来轮询数据**。

这和 **S5 (主站即时点抄)** 不同。S5 是云端发起的异步指令，而 S13 是本地的高频同步轮询。

- **特征**：高并发读取、只读影子数据、协议转换。
- **流向**：
  1.  **SCADA (TCP Client)** 连接 EdgeOS (TCP Server 端口 502)。
  2.  **协议栈 (Server Mode)**：解析 Modbus Read Holding Registers。
  3.  **查表**：将 Modbus 地址映射回内部 `TagID`。
  4.  **读取**：直接读取 `DataHub (ShmRing)` 中的最新快照（Shadow）。
  5.  **响应**：组包返回。
- **关键约束**：**严禁穿透**。本地屏幕刷新率很高（1秒1次），绝不能每次都去读物理串口，必须读内存缓存。

---

### 🔍 遗漏场景二：掉电保护与临终遗言 (Power-Off / Last Gasp)

**—— 场景 S14**

**之前的盲区**：我们讨论了 crash，讨论了断网，但没讨论 **“拔电”**。
**现实痛点**：设备突然断电。此时内存里的 `ShmRing` 和 `Export Queue` 还没来得及落盘，数据全丢。对于 EVSE（计费）和 SG-EL2025（停电事件），这是 P0 级事故。

- **硬件前提**：板载超级电容，能维持系统运行 100ms~500ms，并触发 **PFI (Power Fail Interrupt)** 中断。
- **流向 (极速抢救)**：
  1.  **Trigger**：GPIO 中断触发。
  2.  **Kernel**：发送 `SIGPWR` 信号或触发 `sysfs` 事件。
  3.  **L1 Main Loop**：
      - **停止喂狗**：防止复位。
      - **广播**：`CMD_EMERGENCY_SHUTDOWN`。
  4.  **T2 Persistence**：
      - 执行 `sqlite3_wal_checkpoint` (强制刷盘)。
      - 执行 `sync()` (文件系统同步)。
  5.  **T1 Driver** (可选)：
      - 向云端发送最后一包 MQTT："Device Power Down"。
- **关键约束**：所有操作必须在 **200ms** 内完成，这也是为什么数据库必须用 WAL 模式（刷盘快）的原因。

---

### 🔍 遗漏场景三：大文件非结构化传输 (Bulk File Transfer)

**—— 场景 S15**

**之前的盲区**：我们只传了 `PipeMsg`（小数据）和 `Waveform`（内存引用）。
**现实痛点**：运维需要拉取 **最近 7 天的系统日志 (syslog)**，或者 SG-EL2025 需要上传 **Comtrade 格式的故障录波文件**（几 MB）。这些数据不能走 MQTT（Payload 太小），也不能走 DataHub（太占内存）。

- **策略**：**带外传输 (Out-of-Band)**。
- **流向**：
  1.  **Cloud**：下发指令 `UploadFile("/var/log/sys.log", FTP/HTTP_URL)`。
  2.  **Worker**：启动一个独立的 `FileUploader` 线程。
  3.  **Action**：分片读取磁盘文件 -> HTTP POST / FTP Upload。
  4.  **Flow Control**：限制上传带宽（如 50KB/s），防止占满 4G 带宽影响 IEMS 实时控制。

---

### 🔍 场景 S16：冷启动配置自举 (Zero-Touch Provisioning)

**—— 设备的“第一声啼哭”**

- **痛点**：新设备上电处于“零配置”状态，无法直接进入生产逻辑。
- **流向**：开启临时配置入口 (ZTP Mode) -> 从云端/U盘拉取契约 -> 物理层 Self-Check -> 动态加载 Pipeline。
- **意义**：解决现场大规模部署的运维成本问题。

### 🔍 场景 S17：运行资源隔离与逻辑熔断 (Circuit Breaker)

**—— 防止“一颗老鼠屎坏了一锅粥”**

- **痛点**：某个用户自定义 Stage（如复杂的 JS 算法）导致 CPU 满载或死循环。
- **流向**：Monitor 监控 Stage 执行耗时 -> 触发阈值熔断 -> 卸载异常 Stage -> 维持控制平面 (Arbiter) 正常运行。
- **意义**：提供工业级的软件容错能力。

### 🔍 场景 S18：指令优先级冲突仲裁 (Priority Arbitration)

**—— 谁才是真正的“大脑”**

- **痛点**：多个北向来源（UI、云端、本地策略）同时下发矛盾指令。
- **流向**：Arbiter 读取指令优先级 -> 依据“安全第一”原则过滤指令 -> 指令执行确认。
- **意义**：确保系统在任何状态下具有唯一、确定的控制意志。

### 🔍 场景 S19：多链路冗余与自动切换 (Link Redundancy)

**—— 永不掉线的实时控制**

- **痛点**：现场以太网损坏，系统失去云端控制。
- **流向**：检测 Primary 链路失效 -> 秒级切换至 4G/5G 备用路径 -> 自动触发流量配额策略（仅传输 P0 级控制报文）。
- **意义**：确证高可靠控制场景下的通信连续性。

### 🔍 场景 S20：安全审计与指令验签 (Security & Audit)

**—— 拒绝“特洛伊木马”**

- **痛点**：北向指令被篡改或非法注入。
- **流向**：指令包携带 Hmac/签名 -> Northbound 模块执行物理校验 -> 合法指令进入 Arbiter -> 审计日志记录操作者身份。
- **意义**：保障电力、储能等国家关键基础设施的物理安全。

---

### 🔄 更新后的全景图 (The Complete 20 Scenarios)

加上这最后 5 块拼图，EdgeOS 的业务版图已达成 **100% 逻辑闭环**。

| 类别     | 场景 ID | 场景名称       | 核心特征         | 涉及组件               |
| :------- | :------ | :------------- | :--------------- | :--------------------- |
| **上行** | S1      | 周期抄读       | 轮询、解析       | Scheduler, Arbiter     |
|          | S2      | 主动推送       | 突发、路由       | Driver, EventBus       |
|          | S3      | 波形分析       | 大数据、DMA      | dma-buf, DSP, Pipeline |
|          | S4      | 数据上云       | 缓存、续传       | Export, SQLite         |
|          | S15     | 文件上传       | 带外、低速       | Worker, HTTP Client    |
| **下行** | S5      | 指令透传       | 插队、无状态     | Arbiter(High)          |
|          | S6      | 反向控制       | 紧急、强一致     | Arbiter(Emergency)     |
|          | S7      | 从设备 OTA     | 总线独占         | Arbiter(Exclusive)     |
|          | S12     | 远程隧道       | 长连接、管道     | TCP Bridge, Arbiter    |
|          | **S18** | **冲突仲裁**   | **优先级、安全** | **Arbiter, Security**  |
| **横向** | S8      | 配置热更       | 动态、无重启     | ConfigMgr, Inotify     |
|          | S9      | APP 通信       | 进程间           | IPC (NNG)              |
|          | S10     | 对时守时       | 抗跳变           | TimerFD                |
|          | S13     | 本地服务       | 被动、影子       | Modbus/104 Server      |
|          | **S16** | **冷启动自举** | **自组织、契约** | **ZTP Mgr, Compiler**  |
| **异常** | S11     | 自动发现       | 扫描、指纹       | Udev, Driver Scan      |
|          | S14     | 掉电保护       | 抢救、刷盘       | Signal, Sync           |
|          | **S17** | **逻辑熔断**   | **隔离、限流**   | **Stream Watchdog**    |
|          | **S19** | **链路冗余**   | **切换、降级**   | **NetManager**         |
|          | **S20** | **指令验签**   | **审计、加密**   | **Security Engine**    |

---
