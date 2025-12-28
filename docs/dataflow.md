
结合之前讨论的 总线仲裁 (Chapter 8)、调度器 (Chapter 5) 和 数据管道 (Chapter 4)，我将 《EdgeOS SDK 开发手册 - 第四章：全场景数据流向》 中的 4.4 节 进行全量补全。

这里不再是简单的 5 个场景，而是扩展为 9 大核心业务流，覆盖了从物理层到应用层，从毫秒级控制到文件级传输的所有路径。

4.4 全场景核心流程详解 (Complete Scenario Workflows)

我们将数据流向分为三类：采集与上行 (Upstream)、控制与下行 (Downstream)、系统与横向 (System & Horizontal)。

A. 采集与上行类 (Upstream)

场景一：周期抄读 (Periodic Polling)

适用：HEMS (Modbus), DCU (DLMS), EVSE (Meter)

特征：低优先级，不允许阻塞。

流程：

Scheduler：timerfd 触发，生成 ReadJob (Prio=Normal)。

Arbiter：Worker 线程获取总线锁。

Driver：执行物理读取 (read_registers)。

Parser：将原始字节转换为 PipeMsg (Scalar)。

DataHub：写入 ShmRing / RingBuffer。

Pipeline：主线程清洗数据（TinyExpr 计算）。

Storage：写入 SQLite WAL。

场景二：设备主动推送 (Device Active Push)

适用：HPLC (自动上报), BMS (CAN 广播), 报警器 (GPIO 中断)

特征：突发，非索要。

流程：

Arbiter：poll() 返回 POLLIN (非任务触发)。

Driver：读取完整报文。

Router：根据报文中的地址/ID，反查 DeviceMap 找到设备实例。

Injection：调用 pipeline_input(msg) 注入管道。

EventBus：如果数据判定为“告警”，立即通过 IPC 通知业务 APP。

场景三：高性能波形分析 (High-Perf Analysis)

适用：SG-EL2025 (电能质量/FFT), IEMS (高频录波)

特征：数据量大 (MB级)，零拷贝。

流程：

Hardware：FPGA/ADC 通过 DMA 将数据写入物理内存 (dma-buf)。

Driver：收到硬件中断，封装 PipeMsg (Ref模式)，仅携带 fd 和 offset。

Pipeline：

Stage 1: 识别 Ref 类型，不读取数据。

Stage 2: 调用 stage_fft_accel(fd)，该函数将 fd 传给 NPU/DSP 驱动。

Stage 3: DSP 计算完毕，返回结果结构体 (THD, 谐波)。

Merge：将分析结果 (Scalar) 重新注入 DataHub，原始波形数据根据策略丢弃或存盘。

场景四：数据上云与断点续传 (Cloud Export)

适用：全系产品

特征：流量整形，可靠传输。

流程：

Export Svc：订阅 DataHub。

Batching：攒够 4KB 或 100ms。

Send：调用 MQTT Publish。

Failover：

网络通：发送成功 -> 结束。

网络断：写入 SQLite export_queue。

Recovery：网络恢复 -> 启动低优先级线程 -> 批量读 SQLite -> 补发。

B. 控制与下行类 (Downstream)

场景五：主站即时点抄 (Master On-Demand Read)

适用：DCU (主站查表), HEMS (App 刷新)

策略：读写分离。

流程 A (查影子)：MQTT 请求 -> SDK 查 ShmRing 最新值 -> 立即返回 (1ms)。

流程 B (查透传)：

MQTT 请求 (含 cmd: transparent)。

Command Svc：生成 CmdMsg (Prio=High)。

Arbiter：暂停周期轮询，插队执行物理读取。

Reply：物理回执直接透传回 MQTT，不更新本地缓存。

场景六：算法/云端反向控制 (Control Loop)

适用：IEMS (削峰填谷), EVSE (远程启停), HEMS (家电控制)

特征：高优先级，必须执行。

流程：

Source：算法进程 (IPC) 或 云端 (MQTT) 下发指令 Switch=OFF。

Security：SDK 校验指令签名 (SE 芯片)。

State Check：检查 AdminState 是否为 LOCKED。

Arbiter：生成 CmdMsg (Prio=Emergency)，插入队列头部。

Preempt：Worker 线程完成当前字节传输后，立即执行该指令。

Audit：操作结果写入 SQLite audit_log。

场景七：从设备 OTA 升级 (Device Maintenance)

适用：DCU (升级电表), IEMS (升级 PCS)

特征：大数据流下行，独占总线。

流程：

Command Svc：收到 OTA_Start 指令。

Arbiter：调用 arbiter_set_exclusive(true)。

效果：暂停 Scheduler，清空/挂起所有周期性任务。

Transfer：循环接收 MQTT 数据块 -> 写入串口 -> 等待 ACK。

Finish：收到 OTA_End，调用 arbiter_set_exclusive(false)。

Resume：Scheduler 恢复周期性采集。

C. 系统与横向类 (System & Horizontal)

场景八：对时与守时 (Time Synchronization)

适用：全系产品 (时间准确性是电力的命门)

流程：

Trigger：NTP 同步或主站下发对时帧。

Action：调用 clock_settime 修改系统时间。

Signal：Linux 内核触发 TFD_TIMER_CANCEL_ON_SET。

Scheduler：

timerfd 可读事件触发。

Re-calc：遍历 Min-Heap，基于新时间重算所有任务的 next_run。

Broadcast：SDK 发布 SYS_EVENT_TIME_CHANGED 消息，通知所有驱动更新时间戳缓存。

场景九：微应用间通信 (Inter-App IPC)

适用：SG-EL2025 (APP 协同), IEMS (策略与网关协同)

特征：横向数据流，不经过云端。

流程：

APP A (如光伏预测)：计算出未来功率，调用 edge_app_publish("pv/prediction", json)。

SDK Core：将消息转发到 ZeroMQ / NNG Pub Socket。

APP B (如有序充电)：收到订阅消息。

Action：APP B 根据预测值调整充电桩功率限制，调用 edge_app_set_value(...)。

4.5 场景与产品矩阵 (Scenario Matrix)

为了让开发团队明确重点，我们将场景映射到产品。

场景 ID	场景名称	HEMS	IEMS	SG-EL2025	EVSE	DCU	关键技术组件
S1	周期抄读	★	★	★	★	★	Scheduler, Arbiter
S2	设备主动推送	★	☆	★	☆	★	Poll, Protocol Router
S3	高性能波形分析			★			dma-buf, DSP Driver
S4	数据上云	★	★	★	★	★	MQTT, SQLite Queue
S5	主站即时点抄	☆	★	★	☆	★	Shadow Cache
S6	反向控制	★	★	★	★	★	Priority Queue, SE
S7	从设备 OTA		★	★		★	Exclusive Lock
S8	对时处理	☆	★	★	★	★	TimerFD
S9	APP 间通信		★	★			IPC Bus (NNG)

(★ = 核心功能, ☆ = 辅助功能)

4.6 总结

通过这 9 个场景的补全，我们不仅仅定义了数据怎么流，更定义了：

冲突时谁让路（优先级策略）。

忙碌时怎么排队（背压策略）。

断网时怎么缓存（持久化策略）。

跨进程怎么协作（IPC 策略）。

这才是完整的、无死角的数据流架构。

下面我按**正式技术审核（Architecture / Design Review）**的标准来评这份内容，而不是泛泛“点评”。结论先行，然后逐段给**肯定点、风险点、必须补充点**。

---

# 一、总体审核结论（非常关键）

**结论一句话：**

> 这是一份**已经达到“可以作为 EdgeOS SDK 核心章节定稿候选”的内容**，
> 技术闭环度 ≥ 90%，属于**工业级系统少见的“全路径显式化设计”**。

在我见过的同类文档中，这一章有三个非常突出的优点：

1. **不是画图，而是“执行路径级别”的描述**
2. **主动覆盖 Corner Case（冲突 / 抢占 / 断网 / 掉电 / 对时）**
3. **优先级、仲裁、缓存、恢复逻辑全部显性化**

这已经**超过普通架构文档**，接近**运行时规范（Runtime Spec）**。

---

# 二、核心肯定（为什么这章是“对的”）

## 1️⃣ 场景拆分方式是“正确的工业视角”

你不是按“模块”拆，而是按：

* **业务意图**
* **实时性等级**
* **数据体量**
* **是否可阻塞**

来拆 9 个场景。

这是**工业控制系统唯一合理的拆法**。

尤其这三类划分非常专业：

* Upstream（采集）
* Downstream（控制）
* System & Horizontal（系统横切）

👉 这意味着你在**设计调度与仲裁之前，就已经想清楚“谁该让谁”**。

---

## 2️⃣ Arbiter / Scheduler 的角色被“用对了”

这是我非常认可的一点。

你没有让 Scheduler 背锅一切，而是：

* **Scheduler**：时间维度（when）
* **Arbiter**：资源维度（who first / who exclusive）

在以下场景中用得非常到位：

* S5 即时点抄（插队但不污染缓存）
* S6 反向控制（Emergency 优先级）
* S7 OTA（总线独占）

👉 这避免了 90% 系统里常见的：

> “调度器越来越复杂，最后没人敢改”

---

## 3️⃣ 对“零拷贝”和“引用语义”的理解是正确的

S3 高性能波形分析这一段，是**专业级**的：

* PipeMsg 区分 Scalar / Ref
* fd + offset 传递所有权
* Pipeline Stage 明确 **不解引用原始数据**
* Merge 阶段才回归 Scalar 世界

这是**dma-buf 正确用法**，不是“把大块内存塞进消息”。

---

## 4️⃣ 对“断网不是异常”的态度是工业级的

S4 数据上云这一段，设计是**成熟的**：

* 正常路径：MQTT
* 异常路径：SQLite export_queue
* 恢复路径：低优先级补发

没有“网络失败就报错”，而是**状态机式退化运行**。

---

# 三、必须指出的风险点（不是吹毛求疵）

下面是**架构委员会一定会追问的地方**。

---

## ⚠️ 风险 1：Pipeline / Storage 线程模型未显式说明

在多个场景中存在潜在歧义：

> “Pipeline：主线程清洗数据”

问题是：

* **哪个主线程？**
* 是 libuv loop？
* 是独立 pipeline thread？
* 高负载下是否会阻塞 Scheduler？

### 建议补一句“硬规则”

例如在 4.4 总体说明中明确：

> Pipeline 的 CPU 密集 Stage（FFT / AI）**不得运行在 Scheduler / IO 线程**，
> 必须通过 Work Queue / Thread Pool 执行。

否则实现阶段会被“偷懒式 inline 处理”破坏。

---

## ⚠️ 风险 2：Exclusive Lock 的饥饿风险（S7 OTA）

S7 的设计方向是对的，但需要**防止极端情况**：

* OTA 长时间失败
* 总线长期被锁
* 系统失去基本可观测性

### 建议补充一个“逃生阀”

例如：

* Exclusive 模式下：

  * 仍允许 **心跳 / Admin 命令**
  * 允许 `arbiter_force_release()`（带权限）

否则 OTA 出错 = 设备“假死”。

---

## ⚠️ 风险 3：时间跳变广播的“级联效应”

S8 对时处理逻辑是**非常正确的**，但少了一条安全约束：

* 所有驱动都会收到 `SYS_EVENT_TIME_CHANGED`
* 如果驱动在回调里：

  * 重算定时器
  * 写日志
  * 写数据库

👉 有 **风暴风险**。

### 建议补充：

* 时间变更事件：

  * 分两级：

    * `PRE_TIME_CHANGE`
    * `POST_TIME_CHANGE`
* 且回调必须：

  * 非阻塞
  * 禁止 IO

---

## ⚠️ 风险 4：Shadow Cache 的一致性边界（S5）

S5 的“查影子 / 查透传”策略非常好，但要防一个经典坑：

* 影子值是否带时间戳？
* 是否有 TTL？
* 是否允许返回“陈旧值”？

### 建议明确一句：

> Shadow Cache 返回的数据必须携带 timestamp 与 freshness 标记，
> 上层决定是否接受。

否则现场会出现：

> “数是对的，但时间不对”

---

## ⚠️ 风险 5：APP 间 IPC 的 ABI 稳定性

S9 里：

```text
edge_app_publish("pv/prediction", json)
```

这在 V1 可以，但**长期风险极大**：

* JSON Schema 演进
* APP 版本不一致
* 隐式耦合

### 建议补充：

* IPC Topic 必须：

  * 有 version
  * 有 schema hash
* SDK 层做校验，不符合直接拒收

---

# 四、强烈建议补充的 3 个“收官级”内容

这三点加上去，这一章就是**不可挑剔级别**。

---

## ✅ 补充 1：每个场景的“不可做清单”

例如：

* S6 反向控制：

  * ❌ 不得 malloc
  * ❌ 不得阻塞 IO
* S3 波形分析：

  * ❌ 不得 memcpy 原始数据
* S7 OTA：

  * ❌ 不得写入业务 SQLite

这是**防止实现偏航的最有效手段**。

---

## ✅ 补充 2：失败语义（Failure Semantics）

每个场景至少回答一个问题：

> **失败后，系统的“正确行为”是什么？**

例如：

* S1 抄读失败：跳过，不补抄
* S6 控制失败：重试 / 回滚 / 上报
* S4 补发失败：延迟，不阻塞主路径

这能极大减少现场争议。

---

## ✅ 补充 3：资源上限声明（Backpressure）

你已经隐含了背压思想，但可以显式化：

* RingBuffer 满了怎么办？
* export_queue 最大多大？
* 超限后丢什么？

**工业系统一定要敢丢数据，但要丢“对的那部分”。**

---

# 五、最终审核结论（是否通过）

**结论：**

* ✅ **技术方向正确**
* ✅ **场景覆盖全面**
* ✅ **工业现实感极强**
* ⚠️ **需要补充少量“硬约束声明”与“失败语义”**

👉 我的审核意见是：

> **通过（Pass with Minor Revisions）**

这是一个**可以锁版本号的章节**，不是草稿。

---
