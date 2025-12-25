系统中有哪些角色 哪些组件 哪些线程 各自作用? (典型场景)相互间如何交互?
我按你说的来：**角色 / 组件 / 线程 / 典型场景 / 交互**，给你一张“能装进脑子里”的**体系地图**。

> 目标：
> **你看完能画出系统图、写 README、给新人讲清楚、自己也不再混乱**

---

# 一、系统的「六大角色」（先定人，再定线程）

这套 SDK + 五产品，**本质只有 6 个角色**，不是更多。

| 角色                       | 本质是什么    | 核心职责               |
| ------------------------ | -------- | ------------------ |
| **Main Loop**            | 城市中枢     | 所有调度、决策、回调发生地      |
| **Arbiter**              | 大脑 / 调度官 | 资源仲裁、优先级、独占        |
| **Driver / IO Executor** | 手脚       | 与真实硬件总线打交道         |
| **DataHub**              | 神经系统     | 高速数据广播 / 分发        |
| **Compute / Algorithm**  | 大脑皮层     | 融合、计算、预测           |
| **Persistence / IO**     | 记忆体      | SQLite / Flash / 云 |

> 🔴 **关键共识**
>
> * 只有 **Driver 会阻塞**
> * 只有 **Arbiter 会“想”**
> * 只有 **DataHub 在“喊数据来了”**

```
[ Brain / Main Loop ]
           (唯一的决策中心，单线程，无锁)
          /            |            \
     [Scheduler]   [Arbiter]    [Protocol FSM]
          \            |            /
           \           | (Job Dispatch)
            \          v
    -------------------------------------------------- 线程边界
             |         |          |
 [Arm 1: 485]  [Arm 2: CAN]  [Arm 3: SQLite]  <-- (Driver Threads / Workers)
     (手脚)        (手脚)         (手脚)
       |             |              |
    (IO Poll)     (IO Poll)      (Blocking)
       |             |              |
   [ Meter A ]    [ BMS ]        [ Disk ]

```
---

# 二、线程模型（这是所有困惑的根源，一次讲清）

## 🧵 1️⃣ Main Thread（唯一）

> **整个 SDK 只有一个真正的“控制平面线程”**

**运行内容：**

* libuv loop
* Arbiter（纯逻辑）
* 协议状态机（DLMS / Modbus / OCPP）
* Timer / Retry / Backoff
* API 回调（对上层）

**不能做的事：**

* ❌ 阻塞 I/O
* ❌ poll 硬件 fd
* ❌ 直接算 FFT / SQL

👉 **这是你系统的“宪法”**

---

## 🧵 2️⃣ Driver Threads（1~N 个，强隔离）

> **每一种“物理独占总线” = 一个 Driver Thread**

典型：

| 总线      | Driver           |
| ------- | ---------------- |
| RS485-1 | Driver Thread #1 |
| RS485-2 | Driver Thread #2 |
| RS485-3 | Driver Thread #3 |
| HPLC    | Driver Thread #4 |

**Driver Thread 做什么？**

* poll(fd)
* write → read → silence detect
* 帧级完整性保证
* **一次只服务一个 Job**

**Driver 不做什么？**

* ❌ 不排队
* ❌ 不决定“谁先来”
* ❌ 不调用上层回调

> 👉 Driver 是**执行器，不是调度器**

---

## 🧵 3️⃣ Worker Pool（libuv 自带）

> **CPU / 阻塞但“非硬件”的事情**

* FFT / 滤波
* SQLite step
* 加解密
* 压缩

⚠️ 规则：

* 不持硬件 fd
* 不保留状态
* 必须可重入

---

## 🧵 4️⃣ Optional：Algorithm Thread（高端型号）

> 当算法变复杂时（SG-EL2025）

* 长生命周期
* 吃 shm_ring
* 输出控制建议

---

# 三、组件分解（SDK 里到底有哪些模块）

## 1️⃣ Arbiter（最容易被误解的）

**它是什么？**

* 一个 **纯逻辑状态机**
* 跑在 main loop

**管理什么？**

* Job 队列（优先级）
* 总线是否独占
* OTA / 透传抢占
* Retry / 超时

**它如何“驱动” Driver？**

* 写 eventfd / pipe
* `uv_async_send`
* 不直接调用阻塞函数

---

## 2️⃣ Job（系统的一等公民）

每一个操作都是一个 Job：

* 抄表
* 搜表
* 透传
* 升级
* 主站点抄

```c
struct job {
  bus_id;
  priority;
  is_exclusive;
  timeout;
  on_complete;
};
```

> 🔑 **系统不是“函数调用”，而是 Job 流动**

---

## 3️⃣ Driver（Loop-bound or Thread-bound）

Driver 是**物理世界的翻译官**：

```
Job -> 帧 -> 字节 -> 线路
线路 -> 字节 -> 帧 -> Result
```

---

## 4️⃣ DataHub（shm_ring）

> **不是数据库，不是消息队列**

它是：

* 固定结构
* 高频
* 多读者
* 低延迟

用于：

* 抄表数据
* 事件上报
* 状态变化

**大数据？**

* 只放引用（fd / offset）

---

## 5️⃣ Protocol FSM（协议状态机）

* DLMS
* Modbus
* OCPP

运行在：

* Main loop
* 只操作 Job + DataHub

---

# 四、典型场景（你要的“活的”）

## 场景 1：周期抄表（DLMS / RS485）

```
Timer
  ↓
Arbiter: create Job(read_meter)
  ↓
Driver Thread: 执行帧交互
  ↓
Main Loop: on_complete
  ↓
DataHub publish
  ↓
Algorithm / SQLite / MQTT
```

---

## 场景 2：电表主动上报

```
Driver Thread poll(fd)
  ↓
frame detect
  ↓
uv_async_send(event)
  ↓
Protocol FSM
  ↓
DataHub
```

---

## 场景 3：OTA 抢占

```
OTA Job (exclusive)
  ↓
Arbiter 阻断所有普通 Job
  ↓
Driver 只服务 OTA
```

---

## 场景 4：算法融合 500+ 表计

```
shm_ring (最新值)
  ↓
Algorithm Thread
  ↓
控制策略 Job
  ↓
Arbiter
```

---

# 五、五个产品只是“参数集不同”

| 产品        | Driver 数 | 算法  | shm | SQLite |
| --------- | -------- | --- | --- | ------ |
| HEMS      | 少        | 简单  | 可选  | 同步     |
| IEMS      | 多        | 中   | 必须  | 异步     |
| SG-EL2025 | 多        | 重   | 必须  | 异步     |
| EVSE      | 少        | 状态机 | 轻   | 少      |
| DCU       | 多        | 少   | 中   | 重      |

> 🧠 **SDK 不变，产品只是“配置 + 裁剪”**

---

# 六、一句话总结（给你一个锚点）

> **这是一个：**
>
> * 以 **Main Loop 为唯一决策中心**
> * 以 **Arbiter 统治物理资源**
> * 以 **Driver 独占总线**
> * 以 **DataHub 解耦数据**
>
> 的工业级边缘 SDK。

---

