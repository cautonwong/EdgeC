好，进入 **Phase 3（高质量）**，这一步我们不写代码，**先把 EdgeOS Web UI 的“信息架构（IA）+ 页面定义”一次性定型**，让它**严格映射你前面已经冻结的系统设计**（调度 / Arbiter / DataHub / SQLite / 多总线 / 五产品矩阵）。

下面内容是 **可以直接作为 `docs/web_ui_ia.md` 的规范文档**。

---

# EdgeOS Web UI

## Information Architecture (IA) & Page Definition v1.0 (Frozen)

> 适用对象：
>
> * 集中器 / 采集器 / 边缘网关 / IEMS / 主站代理
> * 本地 Web（嵌入式 httpd）
> * 运维 / 调试 / 工程交付 / 远程诊断

---

## 一、UI 设计原则（非常重要）

### 1️⃣ UI 不是“控制系统”，而是 **观察 + 配置 + 触发**

* **不做实时控制**
* **不参与 Arbiter 决策**
* 只通过：

  * 写配置（SQLite）
  * 触发任务（Scheduler）
  * 查看状态（shm_ring / runtime state）

### 2️⃣ UI 必须 1:1 映射系统真实组件

| UI 概念 | 实际系统组件           |
| ----- | ---------------- |
| 任务    | Scheduler Job    |
| 总线    | Bus Driver       |
| 设备    | Logical Meter    |
| 通道    | Arbiter Channel  |
| 数据    | DataHub / SQLite |
| 事件    | Event Ring       |
| 算法    | Algorithm Worker |

> **UI 不允许引入“虚假抽象”**

---

## 二、整体 IA（一级导航）

```
Dashboard
├── Overview
├── System Health
├── Live Events

Devices
├── Device List
├── Device Detail
│   ├── Basic Info
│   ├── Communication
│   ├── Data
│   ├── Events
│   ├── Control (透传/升级)
│   └── History

Buses & Channels
├── Bus Status
├── Arbiter Channels
├── Priority & Preemption

Scheduling & Time
├── Time Source
├── Job List
├── Calendar Jobs
├── Freeze Jobs
└── Execution History

Data & Storage
├── DataHub Status
├── SQLite Tables
├── Data Query
├── Export

Algorithms
├── Algorithm List
├── Runtime Status
├── Aggregation Results

System
├── Configuration
├── Network
├── Firmware
├── Logs
└── Diagnostics
```

---

## 三、Dashboard

### 3.1 Overview（系统总览）

**目的**：5 秒内判断系统是否“健康”

**展示内容**

* 设备总数 / 在线数
* 总线状态（485 / HPLC / Modbus）
* 当前活跃任务数
* 最近一次成功抄表时间
* 当前系统时间 & 时间源

**数据来源**

* shm_ring: system_state
* scheduler_state
* arbiter_state

---

### 3.2 System Health

* CPU / 内存
* libuv loop 延迟
* Arbiter 阻塞统计
* DataHub ring 使用率
* SQLite WAL backlog

> 对应 **你前面讨论的“是否需要独立 SQLite writer 线程”**

---

### 3.3 Live Events

* 电表主动上报
* 总线异常
* 抄表失败
* 时间跳变
* 抢占发生记录

**本质**

* shm_ring(Event)
* 只读订阅

---

## 四、Devices（核心）

### 4.1 Device List

| 字段        | 说明            |
| --------- | ------------- |
| Device ID | 逻辑设备          |
| Bus       | 485-1 / HPLC  |
| Address   | 表地址           |
| Online    | 心跳状态          |
| Last Read | 最近成功通信        |
| Model     | DLMS / Modbus |

---

### 4.2 Device Detail

#### 4.2.1 Basic Info

* 设备类型
* 协议（DLMS / Modbus）
* 固件版本

#### 4.2.2 Communication

* 绑定 Bus
* 波特率 / 信道
* 最近通信统计

> **对应 IO Executor**

---

#### 4.2.3 Data

* 当前电量
* 日冻结 / 月冻结
* 分钟曲线（只读）

**数据来源**

* SQLite（历史）
* DataHub（实时）

---

#### 4.2.4 Events

* 设备事件时间线
* 掉线 / 上电 / 报警

---

#### 4.2.5 Control（高风险）

⚠️ **必须显示 Arbiter 占用状态**

* 透传命令
* OTA 升级
* 参数读写

> **触发 Arbiter 高优先级通道**

---

#### 4.2.6 History

* 抄表记录
* 错误统计

---

## 五、Buses & Arbiter

### 5.1 Bus Status

| Bus | 类型 | 状态 | 负载 | 错误率 |
| --- | -- | -- | -- | --- |

---

### 5.2 Arbiter Channels

**展示**

* 当前占用者
* 优先级
* 是否可抢占
* 等待队列

**这是你设计里非常关键的一页**

---

### 5.3 Priority & Preemption

* 优先级策略表
* 抢占规则（OTA > 主站透传 > 周期抄表）

---

## 六、Scheduling & Time（与你 Frozen 规范强一致）

### 6.1 Time Source

* 当前时间
* 来源：

  * RTC
  * NTP
  * 主站
* 最近时间跳变记录

> **强调：抗时钟跳变**

---

### 6.2 Job List

| Job | 类型 | 周期 | 状态 | 下次执行 |
| --- | -- | -- | -- | ---- |

---

### 6.3 Calendar Jobs

* 日冻结
* 月冻结
* 周期抄表

> 使用 **日历时间，不是 tick**

---

### 6.4 Freeze Jobs

* 分钟 / 小时 / 日 / 月
* 执行成功率

---

### 6.5 Execution History

* Job → 执行 → 结果
* 用于 QA & 运维

---

## 七、Data & Storage

### 7.1 DataHub Status

* ring depth
* producer / consumer
* 丢包统计

---

### 7.2 SQLite Tables

* 表名
* 行数
* WAL 状态

---

### 7.3 Data Query

* 条件查询
* 只读

---

### 7.4 Export

* CSV / JSON
* 时间范围

---

## 八、Algorithms

### 8.1 Algorithm List

* 名称
* 输入源
* 输出表

---

### 8.2 Runtime Status

* 当前计算窗口
* 延迟
* 失败次数

---

### 8.3 Aggregation Results

* 500+ 表合计
* 分时结果

---

## 九、System

### 9.1 Configuration

* Bus
* Scheduler
* Arbiter policy

### 9.2 Network

* IP / NTP / VPN

### 9.3 Firmware

* 当前版本
* 升级

### 9.4 Logs

* libuv
* driver
* arbiter

### 9.5 Diagnostics

* 自检
* trace dump

---

## 十、UI ↔ 后端接口约束（关键）

| UI 操作 | 后端                |
| ----- | ----------------- |
| 查看    | shm_ring / SQLite |
| 修改配置  | SQLite            |
| 触发任务  | Scheduler API     |
| 控制设备  | Arbiter           |

❌ UI 不直接访问 driver
❌ UI 不直接访问 shm 内部结构

---

## 十一、结论（很重要）

* 这个 UI：

  * **不是 demo**
  * **不是 B/S 花架子**
  * 是 **工业级边缘系统的可观测层**
* 完全对齐你前面：

  * Arbiter v4
  * Scheduling & Time v1.0 (Frozen)
  * DataHub + shm_ring
  * 五产品统一 SDK

---

## REST API列表

你这个问题问得**非常到位**，而且是一个**工业系统 Web UI 设计里最容易被忽略、但后期代价最大的点**。

结论先给出来：

> **刚才那版 IA 里，“设备参数配置”只隐含存在，但不够明确、也不够工程化。**
> 现在我会：
> 1️⃣ **补齐「设备参数配置界面」的完整 IA 设计**
> 2️⃣ **把整个 IA 严格映射成一份 REST API 列表（后端契约级）**

这份内容你可以直接冻结为
`docs/rest_api_v1_frozen.md`

---

# 一、设备参数配置界面（补齐设计）

## 1️⃣ 为什么参数配置必须“独立成体系”

在你这个系统里，**参数配置 ≠ 普通 UI 表单**，它有几个工业级特性：

* 参数来自 **设备侧（DLMS / Modbus）**
* 有：

  * 只读
  * 可写
  * 写需鉴权
  * 写后需复位
* 写操作必须：

  * 经过 **Arbiter**
  * 可被 **抢占 / 中断**
  * 有完整审计

👉 所以它**绝对不能**混在 Basic Info 或 Control 里随便搞。

---

## 2️⃣ IA 中新增：Device Parameters

### Devices → Device Detail → **Parameters**

```
Parameters
├── Parameter Groups
│   ├── Communication
│   ├── Metering
│   ├── Tariff
│   ├── Event
│   ├── Security
│   └── Vendor Specific
├── Read / Write
├── Pending Changes
└── Operation History
```

---

### 2.1 Parameter Groups（参数分组）

> **不是 UI 分组，而是协议语义分组**

| Group         | 示例       |
| ------------- | -------- |
| Communication | 地址、超时、重试 |
| Metering      | CT/PT、倍率 |
| Tariff        | 费率表      |
| Event         | 事件掩码     |
| Security      | 密钥、权限    |
| Vendor        | 私有 OBIS  |

---

### 2.2 参数表格字段（标准化）

| 字段          | 含义                    |
| ----------- | --------------------- |
| Name        | 参数名                   |
| OBIS / Addr | 协议标识                  |
| Type        | int / string / struct |
| Access      | R / W / RW            |
| Value       | 当前值                   |
| Pending     | 待写值                   |
| Source      | Device / Cache        |
| Status      | OK / Dirty / Error    |

---

### 2.3 写参数流程（非常关键）

1. UI 填写参数
2. 提交 → **生成 Parameter Write Job**
3. 进入：

   * Arbiter（高优先级）
   * IO Executor
4. 成功：

   * 更新 SQLite
   * 清除 Pending
5. 失败：

   * 保留 Pending
   * 记录失败原因

---

### 2.4 Operation History

* 谁
* 何时
* 写了什么
* 结果

👉 **这是工程验收必查项**

---

# 二、IA → REST API 映射（完整列表）

下面是 **后端必须实现的 REST 契约**
（UI / CLI / 自动化测试统一使用）

---

## Ⅰ. System / Dashboard

### GET `/api/v1/system/overview`

返回系统总览

### GET `/api/v1/system/health`

CPU / loop 延迟 / ring 状态

### GET `/api/v1/system/events/live`

SSE / WebSocket / 轮询

---

## Ⅱ. Devices

### GET `/api/v1/devices`

设备列表

### GET `/api/v1/devices/{id}`

设备基本信息

### GET `/api/v1/devices/{id}/communication`

通信状态统计

---

## Ⅲ. **Device Parameters（重点）**

### GET `/api/v1/devices/{id}/parameters`

```json
{
  "groups": [
    {
      "name": "Metering",
      "params": [
        {
          "name": "CT Ratio",
          "obis": "1.0.0.4.2.0",
          "type": "uint16",
          "access": "RW",
          "value": 300,
          "pending": null,
          "status": "OK"
        }
      ]
    }
  ]
}
```

---

### POST `/api/v1/devices/{id}/parameters/read`

触发**批量读参数**

```json
{
  "group": "Metering"
}
```

➡️ Scheduler + Arbiter

---

### POST `/api/v1/devices/{id}/parameters/write`

```json
{
  "params": [
    {
      "obis": "1.0.0.4.2.0",
      "value": 400
    }
  ]
}
```

➡️ 生成 **ParameterWriteJob**

---

### GET `/api/v1/devices/{id}/parameters/history`

审计记录

---

## Ⅳ. Control（透传 / OTA）

### POST `/api/v1/devices/{id}/control/transparent`

### POST `/api/v1/devices/{id}/control/ota`

⚠️ 返回 Arbiter channel 状态

---

## Ⅴ. Buses & Arbiter

### GET `/api/v1/buses`

### GET `/api/v1/arbiter/channels`

### GET `/api/v1/arbiter/policy`

---

## Ⅵ. Scheduling & Time

### GET `/api/v1/time/status`

### POST `/api/v1/time/sync`

---

### GET `/api/v1/scheduler/jobs`

### POST `/api/v1/scheduler/jobs`

```json
{
  "type": "daily_freeze",
  "calendar": "00:00",
  "scope": "all_devices"
}
```

---

### GET `/api/v1/scheduler/history`

---

## Ⅶ. Data & Storage

### GET `/api/v1/datahub/status`

### GET `/api/v1/data/query`

```json
{
  "device": "001",
  "from": "2025-01-01",
  "to": "2025-01-31",
  "type": "daily_freeze"
}
```

---

## Ⅷ. Algorithms

### GET `/api/v1/algorithms`

### GET `/api/v1/algorithms/{name}/status`

### GET `/api/v1/algorithms/{name}/result`

---

## Ⅸ. System Management

### GET `/api/v1/config`

### POST `/api/v1/config`

### GET `/api/v1/logs`

### GET `/api/v1/diagnostics`

---

# 三、关键设计约束（必须写进规范）

### ❗ 参数配置的硬约束

* UI 不直写设备
* 所有写操作：

  * 必须生成 Job
  * 必须走 Arbiter
* 参数有 **Pending 状态**
* SQLite 是 **配置源真相**

---

# 四、你现在已经具备的能力（客观评价）

说一句实在的：

> 你现在这套：
>
> * Arbiter v4
> * Scheduling & Time Frozen
> * DataHub / shm_ring
> * 参数配置 Job 化

---

好，这一步**非常关键**。
我给你一版 **“可以直接放进《EdgeOS SDK / Web UI 设计文档》的权威级时序图”**，把你刚才关心的 **数据生成、Web UI、REST、DataHub、Pipeline** 全部串在一条“血管”里。

我会分三层来画，**避免抽象失真**：

1. **数据生成（南向）**
2. **数据处理（Pipeline）**
3. **Web UI / REST（北向）**

最后我会点出**几个必须在实现中强制的约束点**。

---

# 一、完整调用时序图（文字版 UML）

> 场景覆盖：
> 周期抄表 + 设备主动上报 + Web UI 即时查询 + 云上报

---

## ① 数据生成路径（南向 → 系统内部）

```
┌──────────┐
│ Scheduler│
└────┬─────┘
     │ timer (calendar time)
     ▼
┌──────────┐
│ Arbiter  │  (Loop)
└────┬─────┘
     │ select job
     ▼
┌──────────┐
│ Driver   │  (Loop FSM)
│ (RS485)  │
└────┬─────┘
     │ non-blocking write/read
     ▼
┌──────────┐
│ Parser   │
└────┬─────┘
     │ create PipeMsg
     ▼
┌───────────────────────────────┐
│ PipeMsg (single source of truth)
└────┬───────────────┬──────────┘
     │               │
     │               │
     ▼               ▼
┌──────────┐   ┌──────────────┐
│ DataHub  │   │ Stream Engine│
│ (ShmRing)│   │ (Pipeline)   │
└──────────┘   └──────────────┘
```

### ⚠️ 强制约束（Frozen）

* **PipeMsg 只创建一次**
* **写 DataHub 与进入 Pipeline 在同一个上下文**
* **Pipeline 不允许再反查 DataHub**

---

## ② Pipeline 内部处理（事件流）

```
┌──────────────┐
│ StreamEngine │
└────┬─────────┘
     ▼
┌─────────────────────────────┐
│ Stage 1: FilterByTag         │
│  - 不关心 → Drop             │
└────┬────────────────────────┘
     ▼
┌─────────────────────────────┐
│ Stage 2: Transform / Clean  │
│  - scale / unit / quality   │
└────┬────────────────────────┘
     ▼
┌─────────────────────────────┐
│ Stage 3: Export Decision    │
│  - cloud? local? both?      │
└────┬────────────────────────┘
     ▼
┌─────────────────────────────┐
│ Stage 4a: MQTT Export       │
│   success → done             │
│   fail → mark SEND_FAILED    │
└────┬────────────────────────┘
     ▼
┌─────────────────────────────┐
│ Stage 4b: Store & Forward   │
│  - write SQLite retry table │
└─────────────────────────────┘
```

### 关键点（你之前已经踩到精髓了）：

> **Pipeline 处理的是“事件”，不是“状态”**

---

## ③ Web UI 即时查询（北向）

这是你刚才那个“会不会读到很多不关心数据”的核心答案。

### Web UI → REST → DataHub（Shadow Read）

```
┌──────────┐
│ Web UI   │
└────┬─────┘
     │ GET /api/v1/points/{tag_id}
     ▼
┌──────────┐
│ REST API │
└────┬─────┘
     │ validate request
     ▼
┌──────────────┐
│ DataHub Read │
│ (ShmRing)    │
└────┬─────────┘
     │ read slot[tag_id]
     ▼
┌──────────┐
│ REST API │
└────┬─────┘
     │ JSON response
     ▼
┌──────────┐
│ Web UI   │
└──────────┘
```

### ⚠️ 非常重要

* **没有 Pipeline**
* **没有遍历**
* **没有通知**
* **没有副作用**

> Web UI 只看“最新状态”，**不参与事件流**

---

## ④ Web UI 订阅趋势 / 曲线（慢速）

```
┌──────────┐
│ Web UI   │
└────┬─────┘
     │ GET /api/v1/history?tag=1001&period=day
     ▼
┌──────────┐
│ REST API │
└────┬─────┘
     │
     ▼
┌──────────────┐
│ SQLite Query │
│ (read-only)  │
└────┬─────────┘
     ▼
┌──────────┐
│ REST API │
└────┬─────┘
     ▼
┌──────────┐
│ Web UI   │
└──────────┘
```

👉 **历史数据 = SQLite，不是 DataHub**

---

## ⑤ Web UI 下发控制（控制面）

```
┌──────────┐
│ Web UI   │
└────┬─────┘
     │ POST /api/v1/command
     ▼
┌──────────┐
│ REST API │
└────┬─────┘
     │ create CmdMsg
     ▼
┌──────────┐
│ Arbiter  │  (Loop)
└────┬─────┘
     │ priority scheduling
     ▼
┌──────────┐
│ Driver   │
└────┬─────┘
     ▼
┌──────────┐
│ Device   │
└──────────┘
```

* 控制 **不经过 DataHub**
* 控制 **不进 Pipeline**
* 控制 **必须有回执**

---

# 二、为什么这个设计彻底避免“读到不关心数据”

我们用一句话收敛：

> **只有“状态查询”才读 DataHub；
> 所有“事件处理”都发生在 PipeMsg 生成的瞬间。**

所以：

| 场景        | 是否读 DataHub | 是否遍历 |
| --------- | ----------- | ---- |
| Pipeline  | ❌           | ❌    |
| Web UI 点查 | ✅           | ❌    |
| 算法控制      | ✅           | ❌    |
| 云上报       | ❌           | ❌    |
| 设备主动上报    | ❌           | ❌    |

---

# 三、这张图背后隐藏的三个“必须冻结”的设计决策

我非常建议你把这三条**写成红线**：

### 1️⃣ DataHub 永远不产生事件

（否则它会变成第二条 Pipeline）

### 2️⃣ Pipeline 永远只处理“新数据”

（否则你会做重复计算）

### 3️⃣ Web UI 永远不订阅 DataHub

（否则 UI 会影响实时系统）

---
