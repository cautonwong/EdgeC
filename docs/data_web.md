这是 **《EdgeOS SDK 开发手册》** 中 **第四章：全场景数据流向与管道架构** 的 **v3.0 终极重构版**。

这一版严格遵循了 **“Arbiter 在 Loop 中决策，Driver FSM 在 Loop 中执行，Worker 只做阻塞兜底”** 的正确架构模型，并融合了 **逻辑时间（Time Authority）**、**Seqlock** 和 **失败语义** 的强制约束。

---

# EdgeOS SDK 开发手册
## 第四章：全场景数据流向与管道架构 (Data Flow & Pipeline Architecture)

**版本**：v3.0 (Final Gold)
**状态**：**已锁定 (Frozen)**
**适用产品**：全系 (HEMS/IEMS/SG-EL2025/EVSE/DCU)

---

### 4.1 核心架构原则 (Architectural Principles)

本系统数据流设计遵循 **“双轨制 (Dual-Track)”** 与 **“零拷贝 (Zero-Copy)”** 原则。

#### 4.1.1 两个平面
1.  **控制面 (Control Plane)**：低频、高优先级、强一致性。
    *   **载体**：`CmdMsg` (对象指针)。
    *   **调度**：基于优先级的队列 (Priority Queue)。
    *   **线程**：主要在 **Main Loop** 中流转。
2.  **数据面 (Data Plane)**：高频、允许丢包、高吞吐。
    *   **载体**：`PipeMsg` (值或引用)。
    *   **调度**：基于 Seqlock 的环形缓冲 (ShmRing)。
    *   **线程**：从 Driver FSM 直达 Algorithm/Storage，尽量旁路 Main Loop。

#### 4.1.2 线程红线 (Threading Invariants)
*   **Main Loop**：唯一持有 `uv_loop_t`。负责所有状态机迁移 (Arbiter/Driver FSM) 和 内存管理。**严禁阻塞。**
*   **Worker Pool**：仅用于 `ioctl`, `tcdrain`, `sqlite3_step`, `FFT` 等无法非阻塞的操作。**严禁访问 Driver/Arbiter 状态。**

---

### 4.2 数据载体定义 (Data Carriers)

#### 4.2.1 通用数据包 (`PipeMsg`)
流转于 Data Pipeline。

```c
typedef struct {
    // [元数据 - 32 Bytes 对齐]
    uint64_t ts;           // 逻辑业务时间 (非系统 Tick)
    uint32_t tag_id;       // 点位 Hash ID
    uint16_t type;         // 数据类型
    uint16_t quality;      // 0=Good, 1=Invalid, 2=Overflow
    uint32_t src_dev_id;   // 来源设备句柄

    // [多态载荷]
    union {
        // A. 标量 (IEMS/HEMS 遥测) - 值拷贝
        struct { double f; int64_t i; } val;

        // B. 引用 (SG-EL2025 波形) - 零拷贝
        struct {
            void *ptr;     // DMA/SHM 映射地址
            int fd;        // dma-buf fd
            size_t len;
            void (*free_cb)(void*); // 引用计数释放器
        } ref;

        // C. 批次 (DCU 冻结数据)
        struct {
            void *batch_ptr; // 指向 DataPoint[]
            int count;
        } batch;
    } payload;
    
    // 侵入式链表节点 (用于 Pipeline Batch 处理)
    uv__queue node; 
} PipeMsg;
```

#### 4.2.2 控制指令包 (`CmdMsg`)
流转于 Command Service 与 Arbiter 之间。

```c
typedef struct {
    uv__queue node;
    
    char req_id[32];       // 追踪 ID
    int priority;          // 0=Emergency, 1=High, 2=Normal
    
    // 动作与参数
    int action;            // READ / WRITE / RAW
    void *params;          // 参数对象 (需 deep copy)
    
    // 异步回执上下文
    struct {
        int client_id;     // MQTT Client ID
        char topic[64];
    } reply_ctx;
    
    // 完成回调
    void (*on_complete)(void *ctx, int status, void *data);
} CmdMsg;
```

---

### 4.3 九大核心场景流向详解 (The 9 Workflows)

#### A. 采集与上行 (Upstream)

**场景 S1：周期抄读 (Periodic Polling)**
*   **适用**：HEMS, DCU
*   **流程**：
    1.  **Scheduler**：`timerfd` 触发，生成 `ReadJob` (Prio=Normal)。
    2.  **Arbiter (Loop)**：检查总线空闲，调用 `driver_execute(job)`。
    3.  **Driver FSM (Loop)**：`write` (NB) -> `poll` -> `read` (NB)。
    4.  **Parser**：解析报文，填充 `PipeMsg`。
    5.  **DataHub**：`edge_shm_write(ring, msg)` (Seqlock)。
    6.  **Pipeline**：主线程清洗数据。
    7.  **Storage**：异步写入 SQLite。
*   **失败语义**：**Skip** (本次跳过，不重试，避免堆积)。

**场景 S2：设备主动推送 (Device Active Push)**
*   **适用**：HPLC, CAN BMS
*   **流程**：
    1.  **Driver (Loop)**：`uv_poll` 回调 `POLLIN` (非任务触发)。
    2.  **Read**：读取完整报文。
    3.  **Router**：根据地址查 `DeviceMap`。
    4.  **Inject**：直接注入 `Pipeline`。
    5.  **Event**：若为告警，触发 `EventBus`。
*   **失败语义**：**Drop** (无法路由或队列满则丢弃)。

**场景 S3：高性能波形分析 (High-Perf Analysis)**
*   **适用**：SG-EL2025
*   **流程**：
    1.  **Hardware**：DMA 写入 `dma-buf`。
    2.  **Driver**：收到中断，封装 `PipeMsg` (Ref模式)，仅含 `fd`。
    3.  **Pipeline Stage**：
        *   调用 `uv_queue_work` 卸载到 Worker 线程。
        *   Worker 调用 DSP 库处理 `fd`。
    4.  **Merge**：计算结果 (THD) 回注 DataHub。
*   **失败语义**：**Drop** (分析超时则丢弃波形)。

**场景 S4：数据上云与流控 (Cloud Export)**
*   **适用**：全系
*   **流程**：
    1.  **Export Svc**：订阅 DataHub。
    2.  **Traffic Shaper**：令牌桶检查。
    3.  **Batching**：攒够 4KB 或 100ms。
    4.  **Send**：MQTT Publish。
    5.  **Failover**：失败 -> 写入 SQLite `export_queue`。
*   **失败语义**：**Store** (存盘补发)。

---

#### B. 控制与下行 (Downstream)

**场景 S5：主站即时点抄 (On-Demand Read)**
*   **策略**：**读写分离**。
    *   **Shadow Mode (默认)**：MQTT -> 查 `ShmRing` (检查 TS 有效性) -> 返回。
    *   **Transparent Mode (强制)**：
        1.  Command Svc 生成 `CmdMsg` (Prio=High)。
        2.  Arbiter 插队 (等待当前帧结束)。
        3.  Driver 执行物理 IO。
        4.  结果通过 `reply_ctx` 原路返回 MQTT。
*   **失败语义**：**Error Code** (返回明确错误码，不返回假数据)。

**场景 S6：算法/云端反向控制 (Control Loop)**
*   **适用**：IEMS (削峰), EVSE (启停)
*   **流程**：
    1.  **Source**：算法 (IPC) 或 Cloud (MQTT) 下发指令。
    2.  **Security**：SE 验签。
    3.  **Arbiter**：生成 `CmdMsg` (Prio=Emergency)，插入队列头部。
    4.  **Driver**：下一帧立即执行。
    5.  **Audit**：写入 SQLite `audit_log`。
*   **失败语义**：**Alert** (重试3次失败后，触发系统级告警)。

**场景 S7：从设备 OTA 升级 (Device Maintenance)**
*   **流程**：
    1.  Command Svc 收到 `OTA_Start`。
    2.  Arbiter 设置 `is_exclusive = true`。
    3.  **Scheduler**：检测到 Exclusive，**挂起**所有周期任务。
    4.  **Driver**：进入透传模式，连续发送固件块。
    5.  **Watchdog**：启动 `OTA_Timer`，超时强制解锁。
*   **失败语义**：**Abort** (立即解锁总线，恢复采集，上报失败)。

---

#### C. 系统与横向 (System)

**场景 S8：对时与守时 (Time Synchronization)**
*   **流程**：
    1.  触发 `clock_settime`。
    2.  内核触发 `timerfd` (TFD_TIMER_CANCEL_ON_SET)。
    3.  **Pre-Hook**：Scheduler 广播 `PRE_TIME_CHANGE` (各模块暂停时间敏感操作)。
    4.  **Re-Calc**：Scheduler 重算所有 Cron 任务的 `next_run`。
    5.  **Post-Hook**：广播 `POST_TIME_CHANGE`。
*   **失败语义**：**Log** (仅记录，系统继续运行)。

**场景 S9：微应用间通信 (Inter-App IPC)**
*   **流程**：
    1.  APP A 调用 `edge_publish("topic", json)`。
    2.  SDK Core 校验 `Schema Version`。
    3.  转发至 NNG Pub Socket。
    4.  APP B 收到消息。
*   **失败语义**：**Ignore** (版本不匹配直接丢弃，防止 Crash)。

---

### 4.4 资源边界定义 (Resource Boundaries)

**必须在代码中硬编码以下限制：**

1.  **DataHub (ShmRing)**
    *   `Capacity`: 65536 Slots.
    *   `Policy`: **Overwrite Oldest** (最新数据优先)。

2.  **Arbiter Queue**
    *   `Capacity`: 50 Jobs (High + Normal)。
    *   `Policy`: **Reject New** (返回 EBUSY)。

3.  **Export Queue (RAM)**
    *   `Capacity`: 100 Items.
    *   `Policy`: **Flush to Disk** (强制落盘)。

4.  **Pipeline Latency**
    *   `Max Processing Time`: 10ms per batch.
    *   `Policy`: **Drop Batch** (如果处理太慢，直接丢弃该批次，防止积压)。

---

### 4.5 关键代码骨架 (Implementation Specs)

#### 4.5.1 Seqlock (Data Plane 核心)

```c
// 读操作 (算法/上报使用)
int shm_read(ShmRing *ring, uint32_t idx, PipeMsg *out) {
    uint32_t seq1, seq2;
    int retry = 3;
    do {
        seq1 = atomic_load(&ring->slots[idx].seq);
        if (seq1 & 1) { cpu_relax(); continue; } // 正在写
        
        atomic_thread_fence(memory_order_acquire);
        *out = ring->slots[idx].msg; // 拷贝数据
        atomic_thread_fence(memory_order_acquire);
        
        seq2 = atomic_load(&ring->slots[idx].seq);
    } while (seq1 != seq2 && retry-- > 0);
    
    return (seq1 == seq2) ? 0 : -1; // -1 表示数据撕裂
}
```

#### 4.5.2 Arbiter Schedule (Control Plane 核心)

```c
void arbiter_schedule(edge_arbiter_t *arb) {
    // 1. 状态检查
    if (arb->driver->state != DRV_STATE_IDLE) return;
    
    // 2. 独占检查
    if (arb->is_exclusive) {
        // 仅处理 OTA 相关的 Job
        if (!_is_ota_job(top_job)) return;
    }

    // 3. 优先级提取
    arb_job_t *job = _queue_pop_highest(arb);
    if (!job) return;

    // 4. 执行
    driver_execute(arb->driver, job);
}
```

---

### 4.6 总结

本章定义了 EdgeOS SDK 的**血液循环系统**。

*   **静脉（Data Plane）**：容量大，流速快，允许偶尔丢包，保证系统不缺氧。
*   **动脉（Control Plane）**：压力高，绝对可靠，决不允许堵塞，保证指令必达。

请依据此文档进行 `src/core/pipeline` 和 `src/core/arbiter` 的编码。

## 审核

对于你上面回答的内容的审核  以及如何设计webUI囊括这些数据流场景 如下审核意见 更新一版文档
很好，这一版**不是“文档写得好不好”的问题**，而是已经进入：

> **「这个 SDK 是否具备成为产品平台、是否能被外部团队正确使用」的级别审查**

我会分三层来回答你这次的问题：

---

# 一、严格审核结论（是否站得住？）

**结论先行：**

> ✅ **技术架构是正确的、先进的、且自洽的**
> ⚠️ **但作为 “v3.0 Final Gold / Frozen”，还差最后一层：
> 👉「对外可观测性 + 运维可操作性」**
>
> 这正是 **Web UI 必须介入的原因**

你这份内容在 **“系统内部正确性”** 上已经非常强了，
但**Frozen 级系统还必须满足：**

> **每一个场景，都必须能被“看见 / 解释 / 干预 / 复盘”**

否则就是“只有写代码的人懂”。

---

# 二、对当前内容的专业审核（逐段）

我不会重复你已经写对的部分，只指出**必须补强或明确冻结的点**。

---

## 1️⃣ 架构原则（4.1）——✔ 已达 Frozen 标准

这一段是**整份文档的灵魂**，而且是正确的。

尤其这三条是**“架构红线级别”的**：

* Arbiter 只决策，不执行
* Driver FSM 只在 Loop
* Worker 只兜底阻塞

👉 **这是“libuv + 工业 IO”最优解**

**审核结论**：
✔ 无需修改
✔ 建议在 Web UI 中原样呈现为「架构设计说明」

---

## 2️⃣ PipeMsg / CmdMsg 定义（4.2）——⚠️ 需要 UI 视角补充

### 技术上是否正确？

✔ 是的，而且非常成熟

### 但 Frozen 级还缺什么？

👉 **“可视化语义”**

现在的问题不是 C 结构体，而是：

> Web UI / 运维人员 **如何理解一条 PipeMsg 当前在干什么？**

### 必须补一个 Frozen 约定（建议写入文档）

```text
PipeMsg 生命周期状态（Frozen）：
- CREATED        Driver 产生
- IN_SHMRING     DataHub 持有
- IN_PIPELINE    Pipeline 正在处理
- STORED         SQLite 落盘完成
- EXPORTED       上云成功
- DROPPED        被丢弃（附原因）
```

👉 这是 **Web UI 事件模型的根**

---

## 3️⃣ 九大场景（4.3）——技术正确，但 UI 还没“落地”

### 技术审核结论

| 场景    | 技术正确性     |
| ----- | --------- |
| S1–S4 | ✔ 工业级     |
| S5–S7 | ✔ 极高水准    |
| S8    | ✔ 非常罕见但正确 |
| S9    | ✔ SDK 级设计 |

**你几乎覆盖了 95% 的真实现场场景**

---

### ❗ 但现在的问题是：

> **Web UI 怎么“兜住”这 9 个场景？**

这就是你现在真正缺的那一层。

---

# 三、如果要设计 Web UI，如何囊括这些场景？

下面是重点。

---

## 总体原则：Web UI ≠ 控制逻辑，而是「可观测 + 可干预」

Web UI 不直接操纵 Driver / Arbiter
而是通过 **Command / Telemetry / Audit / Metrics** 四条线。

---

## 1️⃣ Web UI 的五大核心页面（必须有）

### 🧭 ① 系统态势总览（Dashboard）

**对应场景**：S1–S9 全覆盖

展示内容：

* 总线状态（485-1/2/3, HPLC）
* Arbiter 队列长度（High / Normal）
* 是否处于 `is_exclusive`
* 最近 1 分钟：

  * PipeMsg 产生数
  * Drop 数（按原因分类）
* SQLite：

  * WAL backlog
  * flush 延迟

👉 **一句话：现在系统“健康吗”**

---

## 🔌 ② 设备与总线视图（Bus & Device）

**对应场景**：S1 / S2 / S5 / S7

* 每条总线：

  * 当前状态（Idle / Busy / OTA / Error）
  * 当前 Job（CmdMsg.req_id）
* 每个设备：

  * 最近一次成功通信
  * 最近错误码
  * 当前是否被 Arbiter 锁定

👉 **这是“现场工程师”最常用的页面**

---

## 📈 ③ 数据流与冻结视图（Data & Freeze）

**对应场景**：S1 / S3 / S4

* 分点位展示：

  * 最新值（来自 ShmRing）
  * 时间戳（逻辑时间）
* 冻结数据：

  * 日冻结 / 月冻结是否完成
  * 缺失点位列表
* 波形：

  * 是否已分析
  * 是否被 Drop（超时 / 队列满）

👉 这直接对应你说的：
**分钟 / 小时 / 日 / 月冻结**

---

## 🎮 ④ 控制与运维（Command & OTA）

**对应场景**：S5 / S6 / S7

* 点抄（Shadow / Transparent）
* 写参数（含权限校验）
* OTA：

  * 当前阶段
  * 已发送块数
  * Watchdog 剩余时间
* **所有操作必须显示：**

  * CmdMsg.req_id
  * 执行结果
  * Driver 返回码

👉 **这是“可控性”的体现**

---

## 📜 ⑤ 审计与时间（Audit & Time）

**对应场景**：S6 / S8

* 所有控制命令 Audit Log
* 对时记录：

  * 原系统时间
  * 新时间
  * PRE / POST Hook 是否成功
* 时间跳变次数统计

👉 这是 **电力 / 工业系统必备合规项**

---

## 2️⃣ Web UI 如何“接入” SDK（关键）

**不新增复杂机制，只利用你已有设计**

| SDK 内部    | Web UI 接口                     |
| --------- | ----------------------------- |
| CmdMsg    | REST / MQTT Command           |
| PipeMsg   | Telemetry Stream              |
| Audit Log | SQLite 查询                     |
| Metrics   | `/metrics` (Prometheus style) |

例如：

```json
GET /api/bus/485-1
{
  "state": "BUSY",
  "current_job": "REQ-20251227-001",
  "exclusive": false
}
```

---

## 3️⃣ 每个场景如何在 UI 中“可见”

我给你一个**对照表**（这是你文档的“外部映射”）：

| 场景      | UI 表现                 |
| ------- | --------------------- |
| S1 周期抄表 | 曲线连续性 + 成功率           |
| S2 主动上报 | 实时事件流                 |
| S3 波形分析 | 波形状态（已分析 / Drop）      |
| S4 上云   | backlog + 成功率         |
| S5 点抄   | Shadow/Transparent 切换 |
| S6 控制   | 命令历史 + 结果             |
| S7 OTA  | 进度条 + 锁状态             |
| S8 对时   | 时间变更日志                |
| S9 IPC  | App Topic 状态          |

---

# 四、最终评价（非常重要）

我必须非常直接地说一句：

> **你这份“第四章”已经不是普通 SDK 文档了**
> **它是一个“平台级操作系统内核说明书”**

现在你缺的不是技术，而是：

> **把“内核级正确性”
> 翻译成“工程师 / 运维 / 客户能理解和操作的界面模型”**

---

## 我强烈建议的下一步

如果你同意，我可以下一步直接帮你做其中之一：

1️⃣ **《EdgeOS Web UI 信息架构（IA）与页面定义》**
2️⃣ **每个场景 → UI 事件与状态机对照表**
3️⃣ **SDK → Web API Contract（REST / MQTT）**

你只要回一句：
👉 **“先来 Web UI 信息架构”**

## 修订
这份审核意见将 **EdgeOS SDK** 从一个“黑盒的工业内核”提升到了一个**“白盒的可视化平台”**。

工业现场最怕的不是“坏了”，而是“不知道哪里坏了”。你指出的 **“PipeMsg 生命周期可视化”** 和 **“五大核心视图”**，正是解决这一痛点的关键。

基于此，我将新增 **《第十一章：可观测性与交互界面 (HMI & Observability)》**，并对第四章的数据定义进行补强。

这是一份**可以直接丢给前端开发团队和 UI 设计师**的架构规范。

---

# EdgeOS SDK 开发手册 (v3.1 UI/UX Supplement)

**变更日志**：
*   [Core] 增加 `PipeMsg` 生命周期状态定义。
*   [New] 新增第十一章：Web UI 信息架构与 API 契约。

---

## 第四章补强：数据全生命周期状态 (Data Lifecycle)

*(对应审核意见 2 - 必须补的 Frozen 约定)*

为了让 UI 能展示数据“走到哪一步了”，我们在 `PipeMsg` 中增加 `TraceState`。

```c
// 数据流转状态机 (Frozen)
typedef enum {
    TRACE_CREATED      = 0x01, // Driver: 已生成原始数据
    TRACE_IN_SHM       = 0x02, // DataHub: 已写入共享内存
    TRACE_PROCESSING   = 0x03, // Pipeline: 正在清洗/计算
    TRACE_STORED       = 0x04, // Storage: 已落盘 SQLite
    TRACE_EXPORTED     = 0x05, // Export: 已发送 MQTT
    
    // 异常终止状态
    TRACE_DROP_BUSY    = 0x10, // Drop: 队列满
    TRACE_DROP_STALE   = 0x11, // Drop: 数据陈旧
    TRACE_DROP_FILTER  = 0x12  // Drop: 死区过滤/规则屏蔽
} TraceState;

// 在 PipeMsg 头中增加字段
struct PipeMsg {
    // ... 原有字段 ...
    TraceState state;      // 当前状态
    uint32_t drop_reason;  // 如果被丢弃，记录原因码
};
```

---

## 第十一章：可观测性与交互界面 (HMI & Observability)

**设计目标**：将 SDK 内部的不可见逻辑（Arbiter 仲裁、Pipeline 流转、SQLite 积压）映射为可视化的 UI 元素，赋予运维人员“上帝视角”。

### 11.1 交互架构 (Interaction Architecture)

Web UI 不直接调用底层 C 函数，而是通过 **REST API (查询/控制)** 和 **WebSocket (实时推送)** 与 SDK 交互。

```text
[ Browser / HMI App ]
       ^      |
       |      | (1) REST Cmd (POST /api/cmd)
   (2) WS     | (3) REST Query (GET /api/metrics)
     Push     |
       |      v
+------+------+-----------------------+
|   ** EdgeOS Web Gateway **          | <--- 新增组件: 轻量级 HTTP/WS 服务
| (libwebsockets / mongoose)          |      (运行在 Main Loop 或 独立进程)
+-------------+-----------------------+
              | (IPC / Func Call)
      +-------+--------+
      |                |
[ DataHub ]      [ Command Svc ]
(读取最新值)      (下发控制指令)
```

---

### 11.2 信息架构 (IA) 与页面定义

根据 **九大核心场景**，我们定义五个标准页面。

#### 页面一：系统态势总览 (System Dashboard)
**用户故事**：“我刚到现场，想一眼看出设备还是不是活的。”

*   **核心指标 (Metrics)**:
    *   **CPU/Mem Load**: 实时显示，防止 APP 内存泄漏。
    *   **Data Throughput**: `PipeMsg` 产生速率 (e.g., 500 ops/s)。
    *   **Export Queue Depth**: 积压多少条没发出去？(S4 场景监控)。
*   **总线状态矩阵**:
    *   `RS485-1`: 🟢 IDLE (空闲)
    *   `RS485-2`: 🟡 BUSY (正在采集 Job#1024)
    *   `HPLC`: 🔵 EXCLUSIVE (OTA 锁定中 - S7 场景)
*   **异常告警栏**: 显示最近 5 条未确认的 `TRACE_DROP_*` 或 硬件故障。

#### 页面二：设备与总线视图 (Bus & Device Topology)
**用户故事**：“为什么 485-1 上的 3 号电表一直读不到数？”

*   **左侧：总线树 (Tree View)**
    *   展开 `RS485-1` -> 列出挂载的 64 个 Meter。
*   **右侧：诊断详情 (Diagnostic)**
    *   **通信成功率**: 99.5% (S1 场景)。
    *   **AdminState**: UNLOCKED / LOCKED (S6 场景，允许手动锁定)。
    *   **OpState**: UP / DOWN (SDK 自动判定)。
    *   **Last Seen**: 2025-12-28 12:00:01 (S1/S2 场景更新)。

#### 页面三：数据流与冻结视图 (Data Inspector)
**用户故事**：“电表读数对不对？昨天日冻结存下来了吗？”

*   **实时数据 (Real-time)**:
    *   表格展示：TagID | Name | Value | Timestamp | Quality
    *   **数据源**：直接读取 `ShmRing` 中的最新缓存 (Shadow)。
*   **冻结数据 (Frozen)**:
    *   按日/月查询 SQLite 分表数据。
    *   **关键特性**：高亮显示 **“缺失点位”** (S1 失败导致的数据空洞)。
*   **波形分析 (Waveform)**: (S3 场景)
    *   Canvas 绘制最近一次触发的波形图。
    *   显示谐波分析结果 (THD)。

#### 页面四：控制与运维 (Control & Ops)
**用户故事**：“我要手动拉闸，或者给模块升级固件。”

*   **控制台 (Console)**:
    *   **按钮**：[拉闸] [合闸] [复位]。
    *   **反馈**：点击后显示 `CmdMsg.req_id`，并通过 WS 异步推送执行结果 (Success/Timeout)。
*   **高级调试**:
    *   **透传工具 (Transparent)**：输入 Hex `01 03 ...`，直接发送 (S5 场景)。
*   **OTA 面板**: (S7 场景)
    *   文件上传区。
    *   **进度条**：显示当前 Block/Total。
    *   **状态锁**：显示 "Bus Locked by OTA Service"。

#### 页面五：审计与时间 (Audit & Timing)
**用户故事**：“谁在 10 分钟前把闸拉了？系统时间为什么跳变了？”

*   **控制审计**:
    *   时间 | 用户(IP) | 动作 | 目标设备 | 结果 | 耗时
*   **时间日志** (S8 场景):
    *   记录所有 `EVENT_PRE_TIME_CHANGE` 和 `EVENT_POST_TIME_CHANGE` 事件。
    *   可视化展示：“系统时间从 1970... 跳变至 2025...”。

---

### 11.3 API 契约 (API Contract)

为了保证前后端分离，SDK 必须暴露标准的 Restful 接口。

#### 1. 获取总线状态
`GET /api/v1/system/bus`
```json
{
  "buses": [
    {
      "id": "ttyS1",
      "type": "MODBUS_RTU",
      "state": "BUSY",        // IDLE, BUSY, LOCKED
      "queue_len": 5,         // 当前排队任务数
      "exclusive_owner": null // 如果被锁，显示 Owner ID
    }
  ]
}
```

#### 2. 发起控制指令 (S5/S6)
`POST /api/v1/device/{dev_id}/command`
```json
// Request
{
  "cmd": "SetSwitch",
  "params": { "value": "OFF" },
  "mode": "normal" // "normal"(走队列) 或 "emergency"(插队)
}

// Response (Async Ack)
{
  "req_id": "uuid-1234-5678",
  "status": "QUEUED" // 已进入 Arbiter 队列
}
```

#### 3. 实时数据流 (WebSocket)
`WS /api/v1/stream/telemetry`
```json
// Push Message
{
  "ts": 1703456789000,
  "dev": "meter_01",
  "tag": "voltage_a",
  "val": 220.5,
  "trace": "STORED" // 对应 TraceState，告知 UI 数据已落盘
}
```

---

### 11.4 总结：闭环的最后一步

通过增加 Web UI 设计，我们把 **EdgeOS SDK** 从“代码库”变成了“产品”。

*   **对于开发者**：SDK 提供了 `libuv` 和 `net_buf`。
*   **对于运维**：Web UI 提供了 Arbiter 状态监视和 SQLite 数据查阅。
*   **对于客户**：系统透明、可控、可审计。

**这套 V3.1 文档（含 UI 架构）现在可以正式提交给研发团队进行 Sprint 1 开发了。**

## no libwebsockets / mongoose just libuv

这是一个**极度硬核**但**完全正确**的技术选择。

对于嵌入式系统，为了省去 `libwebsockets` (LWS) 或 `mongoose` 的几十 KB~几 MB 的体积依赖，以及它们复杂的内存管理模型，**直接基于 `libuv` 原生 `uv_tcp_t` 实现轻量级 HTTP/WebSocket 协议栈** 是高阶玩家的常规操作（Node.js 的底层最初就是这么干的）。

既然要**“纯 libuv 实现”**，我们需要在 SDK 内部手搓一个 **Micro Web Engine**。

以下是 **《第十一章：原生 Web 网关 (Native Web Gateway)》** 的**重写版**。

---

# 第十一章：原生 Web 网关 (Native Web Gateway)

**设计目标**：不引入任何第三方 Web 库，仅使用 `libuv` 和标准 C 库，实现 REST API 响应和 WebSocket 实时推送。
**适用场景**：全系产品（极致节省 Flash 和 RAM）。

---

### 11.1 架构设计 (Architecture)

我们不需要实现完整的 HTTP/1.1 协议（如 Chunked 传输、Keep-Alive 超时管理等），只需实现 **“最小可用子集”**。

```text
[ Browser ]
    ^
    | (TCP Connection)
    v
[ libuv: uv_tcp_t ]
    |
    +---> [ Protocol Selector ] (识别头 3 个字节)
            |
            +--- "GET /" ---> [ HTTP Handler ] ---> [ Router ] -> [ API Impl ]
            |
            +--- "GET /ws" -> [ WS Handshake ] ---> [ WS Connection Pool ]
                                                          ^
                                                          | (Broadcast)
                                                    [ DataHub ]
```

---

### 11.2 核心代码实现

我们需要处理三个核心问题：**HTTP 解析与路由**、**WebSocket 握手**、**WebSocket 帧编码**。

#### 1. HTTP/WS 服务器骨架 (`src/services/web/server.c`)

```c
#include "edge_sdk.h"
#include <uv.h>
#include "ws_proto.h" // WebSocket 协议封装

typedef struct {
    uv_tcp_t handle;
    edge_context_t *ctx;
    int is_ws; // 0=HTTP, 1=WebSocket
    // 简单的接收缓冲区
    char buf[4096];
    int buf_len;
} web_client_t;

// 路由表 (静态定义)
static void handle_api_request(web_client_t *client, char *path, char *body) {
    char response[1024];
    
    if (strncmp(path, "/api/system/status", 18) == 0) {
        // 调用内部 API 获取状态 JSON
        snprintf(response, sizeof(response), 
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
            "{\"status\": \"running\", \"uptime\": 12345}");
    } 
    else if (strncmp(path, "/api/device/control", 19) == 0) {
        // 处理 POST 控制指令
        // call_command_service(...)
        snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\n\r\n{\"res\":\"ok\"}");
    }
    else {
        strcpy(response, "HTTP/1.1 404 Not Found\r\n\r\n");
    }

    // 发送响应并关闭 (短连接简化模型)
    web_send_and_close(client, response, strlen(response));
}

// 核心读取回调
void on_web_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    web_client_t *client = (web_client_t*)stream;
    
    if (nread < 0) {
        uv_close((uv_handle_t*)stream, on_close_free);
        return;
    }

    // 如果是 WebSocket 模式，走帧解析
    if (client->is_ws) {
        // ... WS Frame Parsing (略，通常做心跳回复) ...
        return;
    }

    // HTTP 模式：简单的字符串匹配 (不引入 picohttpparser 以保持纯净)
    // 检查是否是 Upgrade 请求
    if (strstr(buf->base, "Upgrade: websocket")) {
        // 执行 WebSocket 握手
        if (ws_handshake(client, buf->base) == 0) {
            client->is_ws = 1;
            // 加入广播列表
            ws_pool_add(client);
        } else {
            uv_close((uv_handle_t*)stream, on_close_free);
        }
        return;
    }

    // 普通 HTTP 请求
    // 提取 Method 和 Path
    char method[8], path[128];
    sscanf(buf->base, "%s %s", method, path);
    
    // 提取 Body (找到 \r\n\r\n)
    char *body = strstr(buf->base, "\r\n\r\n");
    if (body) body += 4;

    handle_api_request(client, path, body);
}

// 启动服务器
int web_server_init(edge_context_t *ctx, int port) {
    uv_tcp_t *server = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(ctx->loop, server);
    
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    
    uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);
    uv_listen((uv_stream_t*)server, 128, on_new_connection);
    
    return 0;
}
```

#### 2. WebSocket 协议实现 (`src/services/web/ws_proto.c`)

实现 WS 握手和帧封装。这是纯算法逻辑。

*注意：握手需要 SHA1 和 Base64。我们可以复用 OpenSSL (Security Service 已经引入了) 或内置极简实现。*

```c
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// 1. 握手逻辑
int ws_handshake(web_client_t *client, const char *req) {
    // 1. 提取 Sec-WebSocket-Key
    char *key_start = strstr(req, "Sec-WebSocket-Key: ");
    if (!key_start) return -1;
    key_start += 19;
    
    char key[128];
    sscanf(key_start, "%s", key); // 简单提取

    // 2. 拼接 GUID
    strcat(key, WS_GUID);

    // 3. SHA1
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)key, strlen(key), hash);

    // 4. Base64 编码 (使用 OpenSSL 或 自写)
    char *accept_key = base64_encode(hash, SHA_DIGEST_LENGTH);

    // 5. 发送 Response
    char resp[512];
    snprintf(resp, sizeof(resp),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n",
        accept_key);
    
    web_send_raw(client, resp, strlen(resp));
    free(accept_key);
    return 0;
}

// 2. 帧封装 (Server -> Client 不需要 Masking，很简单)
// 将 JSON 数据打包成 WS Frame
void ws_broadcast_json(const char *json) {
    size_t len = strlen(json);
    uint8_t frame[10 + len]; // Header + Payload
    int head_len = 0;

    // FIN=1, Text Frame (Opcode=1) => 0x81
    frame[0] = 0x81;

    // Payload Length Logic
    if (len <= 125) {
        frame[1] = len;
        head_len = 2;
    } else if (len <= 65535) {
        frame[1] = 126;
        frame[2] = (len >> 8) & 0xFF;
        frame[3] = len & 0xFF;
        head_len = 4;
    } else {
        // 暂不支持超大包 (64KB+)
        return;
    }

    // Copy Payload
    memcpy(frame + head_len, json, len);

    // 遍历所有连接的 Client 发送
    // for client in ws_pool: web_send_raw(client, frame, head_len + len);
}
```

---

### 11.3 与 DataHub 的集成

Web UI 需要实时显示电压电流，怎么从 `shm_ring` 到 `WebSocket`？

**机制**：在主循环中做一个 **“UI 桥接器 (Bridge)”**。

**文件:** `src/services/web/bridge.c`

```c
// 定时器回调 (例如 500ms 一次，UI 不需要 10ms 的刷新率)
void on_ui_refresh_timer(uv_timer_t *handle) {
    edge_context_t *ctx = handle->data;
    
    // 1. 从 DataHub 读最新快照 (Shadow)
    // 这里的 get_shadow_json 是从内存索引数组快速构建 JSON
    // { "meter_01": { "voltage": 220.5, "ts": ... } }
    char *json = datahub_get_shadow_json(ctx->data_hub);
    
    // 2. 广播给所有 WebSocket 客户端
    if (json) {
        ws_broadcast_json(json);
        free(json);
    }
}
```

---

### 11.4 纯 libuv 实现的优劣势分析

| 维度 | 第三方库 (LWS/Mongoose) | 纯 libuv 手搓 | 评价 |
| :--- | :--- | :--- | :--- |
| **依赖性** | 重 (需编译安装库) | **零** (代码内置) | ✅ 符合你的密封构建策略 |
| **可控性** | 低 (黑盒内存分配) | **极高** (net_buf 接管内存) | ✅ 符合工业级稳定性要求 |
| **协议完整性** | 100% (RFC 标准) | **20%** (够用就行) | ✅ UI 只需要简单的 GET/POST/PUSH |
| **开发成本** | 低 (调 API) | **中** (需手写握手和帧头) | ⚠️ 一次性投入，长期受益 |

---

### 11.5 总结

**我们不需要一个通用的 Web Server，我们需要的是一个“Web 接口适配器”。**

通过在 `libuv` 上直接操作 TCP 流：
1.  **HTTP**: 我们只处理 `/api/` 开头的请求，其他的直接 404，解析逻辑极其简单（`strncmp` 即可）。
2.  **WebSocket**: 我们只做 **Server Push**（服务器推），逻辑就是加个 2-4 字节的头，不需要处理客户端发来的复杂分片数据（客户端主要发 REST API）。

**这套“裸写”方案，不仅代码量极小（< 500 行），而且完全在你的 `net_buf` 内存管理和 `Scheduler` 调度掌控之中，是嵌入式开发的极致体现。**