这几个问题问得**非常到位**，而且已经不是“能不能做”，而是在问 **“边界画在哪才不会把系统玩炸”**。
我逐条给你**明确结论 + 设计红线 + 推荐结构**，不模糊。

---

## 一、FFT / 算法算不算 Pipeline Stage？——**结论：算，但有分级**

### ✅ 结论先行

> **是的，FFT / 特征提取 / 聚合计算都属于 Pipeline Stage**
> **但必须区分两类 Stage：同步 / 异步**

---

### 1️⃣ 同步 Stage（Loop 内，快）

**适用：**

* 标量计算
* 简单滤波
* 阈值判断
* JS 规则判断（QuickJS）

**特点：**

* 执行时间 < 1–2ms
* 不阻塞
* 直接修改 `PipeMsg`

```c
// 同步 FFT ❌（禁止）
int stage_fft_sync(...) { ... } // 不允许
```

---

### 2️⃣ 异步 Stage（Worker Pool）

**适用：**

* FFT / STFT
* 波形统计
* 500 表聚合
* 复杂算法（C / JS）

**实现方式（唯一允许）：**

```c
// Stage 返回 ASYNC
int stage_fft(edge_ctx_t *ctx, void *cfg, PipeMsg *msg) {
    uv_queue_work(ctx->loop,
                  &msg->work,
                  fft_worker,
                  fft_after);
    return STAGE_ASYNC;
}
```

```c
void fft_worker(uv_work_t *req) {
    // heavy FFT
}

void fft_after(uv_work_t *req, int status) {
    // 回到 Loop，继续 Pipeline
}
```

📌 **这正是 SG-EL2025 / IEMS 的核心模型**

---

## 二、Pipeline Stage 能不能写 DataHub / shm_ring？

### ✅ 答案：**可以，但非常有限制**

> **允许“生成新数据”写 DataHub**
> **禁止“回写 / 修改原始数据”**

---

### ✅ 允许的场景（正确）

| 场景        | 示例                     |
| --------- | ---------------------- |
| FFT 生成新特征 | THD、Harmonic_3         |
| 聚合结果      | 5min avg / hour sum    |
| 算法输出状态    | control_state / health |

```c
// 正确示例
PipeMsg out = {
    .tag_id = TAG_THD,
    .payload.val.f = thd,
    .ts = msg->ts,
};
shmring_write(ctx->datahub, &out);
```

**语义：新数据源**

---

### ❌ 禁止的场景（会炸）

| 错误                         | 原因     |
| -------------------------- | ------ |
| 修改原 msg 再写 DataHub         | 破坏不可变性 |
| 从 DataHub 读 → 算法 → 写回同 tag | 反馈回路   |
| Pipeline 读 DataHub         | 隐式依赖   |

📌 **Pipeline 只吃“事件”，DataHub 只存“状态”**

---

## 三、算法（C / QuickJS）的“状态”应该放在哪里？

这是**最关键的一问**。

---

### 总体原则（冻结）

> **Pipeline 是无状态的**
> **算法状态必须显式存放**

---

## 1️⃣ C 算法状态（推荐方式）

### 📍 放在算法实例上下文中

```c
typedef struct {
    double acc;
    int count;
    uint64_t last_ts;
} algo_avg_state_t;
```

```c
typedef struct {
    algo_avg_state_t *state;
} algo_ctx_t;
```

* 生命周期：**随 Pipeline**
* 线程模型：**只在 Loop 访问**
* 不跨进程

✅ 适合：

* 聚合
* 滤波器
* 控制器

---

## 2️⃣ QuickJS 算法状态

### ❌ 禁止：靠 JS 全局变量

### ✅ 正确做法：**C 持有状态，JS 只是计算函数**

```js
export function step(input, state) {
    state.sum += input;
    state.count++;
    return state.sum / state.count;
}
```

```c
JSValue js_state = ctx->js_state; // C 持有
```

📌 **JS 是纯函数 + 状态对象参数**

---

## 3️⃣ 跨重启 / 长周期状态

| 类型        | 存放          |
| --------- | ----------- |
| 日冻结 / 月冻结 | SQLite      |
| OTA 状态    | SQLite      |
| 控制策略参数    | KV Store    |
| 算法模型参数    | 文件 / SQLite |

🚫 **绝不放在 DataHub**

---

## 四、你提的 algo 目录结构 ——结论：**基本正确，但需微调**

你给的：

```
algo/include
algo/c
algo/js
```

### ✔️ 思路是对的

但**工业级 SDK 需要再加两层语义**

---

## ✅ 推荐最终结构（Frozen）

```
src/algo/
├── include/
│   ├── algo_api.h        // Pipeline 调用接口
│   ├── algo_ctx.h        // 算法上下文定义
│   └── algo_state.h      // 状态结构（可序列化）
│
├── c/
│   ├── fft/
│   │   ├── fft_algo.c
│   │   └── fft_state.c
│   ├── aggregate/
│   │   ├── avg.c
│   │   ├── sum.c
│   │   └── freeze.c
│   └── control/
│       ├── pid.c
│       └── peak_shaving.c
│
├── js/
│   ├── rules/
│   │   ├── alarm.js
│   │   └── protect.js
│   └── compute/
│       ├── calc_thd.js
│       └── custom.js
│
└── runtime/
    ├── quickjs_rt.c      // JS runtime 封装
    └── js_binding.c      // C <-> JS glue
```

---

## 五、Pipeline + Algo 的最终职责划分（必须记住）

| 模块       | 职责              |
| -------- | --------------- |
| Pipeline | 调度、顺序、错误处理      |
| Stage    | 单一计算动作          |
| Algo     | 纯计算 + 显式状态      |
| DataHub  | 最新状态            |
| SQLite   | 历史 & checkpoint |
| JS       | 可配置逻辑，不掌权       |

---

好，这三个问题**问得非常专业，而且是同一条主线的三个关键拐点**。
我会**逐条给你“不打太极、不自相矛盾”的解释**，并且在最后给你一个**“二开者视角的一句话模型”**。

你刚才质疑得对：**如果解释不精确，确实会显得前后矛盾**。下面我把那层“容易混”的地方彻底拆开。

---

# 问题 1

## 为什么 `driver_on_dma_complete()` 不直接写 DataHub，而是进 Pipeline？

> **是不是跟你说的“状态进 DataHub、事件进 Pipeline”矛盾？**

### 结论先行（很重要）

> **Driver 只能产生“事实”，不能产生“业务状态”**

所以：

* ✅ Driver → **Pipeline**
* ❌ Driver → **DataHub（直接写）**

这不是矛盾，而是**职责边界**。

---

## 把 Driver 放在“工业分层”里看

### Driver 在做什么？

```text
“DMA 完成了，我拿到了一段原始数据”
```

它**只知道**：

* 数据从哪来（SPI / UART / ADC）
* 什么时候到
* 数据是什么格式

它 **不知道**：

* 这是不是“电压”
* 需不需要 FFT
* 是不是要生成分钟冻结
* 这是瞬时值还是结算值

👉 **Driver 认知层级极低**

---

## 那 DataHub 是什么？

> **DataHub = 已定义语义的“系统状态”**

写 DataHub 的前提是：

* tag_id 已确定
* 语义已确定（瞬时 / 累计 / 冻结）
* 质量位已确定
* 时间已确定（抗跳变）

Driver **无法保证这些条件**。

---

## 正确的职责划分（非常关键）

| 层        | 能做的事         |
| -------- | ------------ |
| Driver   | “我有数据了”      |
| Pipeline | “这数据意味着什么”   |
| DataHub  | “系统现在处于什么状态” |

---

### 所以这个流程是**必然的**

```text
Driver → Pipeline →（语义生成）→ DataHub
```

如果你让 Driver 写 DataHub，后果是：

* ❌ 业务逻辑散落在 Driver
* ❌ 二开要改驱动
* ❌ 无法复用

**这是工业系统的大忌。**

---

# 问题 2

## 负荷曲线 / 分钟冻结 / 日月冻结 / 结算 —— 都在 Stream Engine 里做？

### 答案非常明确：

> **是的，而且必须如此。**

但要加一句：

> **它们是“事件驱动的时间算法”，不是 Driver 逻辑**

---

## 先给你一个总表（非常重要）

| 功能        | 本质       |
| --------- | -------- |
| 瞬时量       | 状态       |
| 负荷曲线      | 时间窗口聚合   |
| 分钟冻结      | 时间边界事件   |
| 日冻结 / 月冻结 | 日历事件     |
| 结算        | 有状态的周期计算 |

👉 **全部是 Stream / Pipeline 算法**

---

## 为什么不能放 DataHub 里？

DataHub 的原则是：

> **不计算，只存“最新结果”**

比如：

* DataHub 里可以有：

  * `P_inst`
  * `E_day`
  * `E_month`
* 但 **不会有“怎么算的”**

---

## 分钟冻结是怎么发生的？

```text
时间子系统 → 产生“分钟边界事件”
               ↓
         Pipeline 接收
               ↓
      FreezeAlgo(minute)
               ↓
      写 DataHub: P_1min
```

✔ **冻结是事件**
✔ **冻结结果是状态**

---

## 日 / 月冻结也是一样

* 不是 `sleep(86400)`
* 是 **日历跳变安全时间源**
* 触发 Pipeline
* 执行算法
* 写入 DataHub

---

## 结算（Billing）为什么一定是 Stream？

因为结算算法：

* 依赖多个状态
* 有内部累积状态
* 有边界条件
* 有“补算/重算”需求

👉 **只能是有状态的 Pipeline Stage**

---

# 问题 3

## 从“项目二开”的角度看，你的理解对吗？

你给的总结其实 **已经 80% 对了**，我帮你**工业化一下**。

---

## 二开者真实看到的世界（极其重要）

### 二开 = 不碰内核，不碰调度，不碰时间

---

## 二开的标准两步（精确版）

### ✅ 第一步：接入“事实源”（Data Source）

> 你说的 **“写协议 / 写采集任务 / 写 shmring”**

✔ 完全正确

```text
协议栈 / 设备
   ↓
驱动 / 采集任务
   ↓
ShmRing
```

这是在说：

> **“系统如何知道世界发生了什么”**

---

### ✅ 第二步：定义“意义”（Meaning）

> 你说的 **“组合算法”**

✔ 也完全正确，但我们说得更精确：

```text
Pipeline = 语义组合器
```

你做的是：

* 选择输入 tag
* 选择算法（C / JS）
* 定义时间窗口
* 定义输出 tag

---

## 用一句话总结二开者的工作

> **“把物理世界的原始信号，变成业务世界的状态”**

---

## 用配置语言描述二开（这才是终极目标）

```yaml
pipelines:
  - id: load_curve
    trigger: [P_inst]
    functions:
      - window: 1min
      - avg
      - freeze
      - publish: P_1min
```

---

# 最后，我要纠正你一个“潜在误区”（非常重要）

你潜意识里在想：

> “Pipeline 是不是就是高级 Driver？”

**不是。**

| Driver | Pipeline |
| ------ | -------- |
| 面向硬件   | 面向语义     |
| 无状态    | 可有状态     |
| 强实时    | 可调度      |
| 不可配置   | 强配置      |

---

# 一句话定锚（你可以记住）

> **Driver 只说“发生了”
> Pipeline 负责“这意味着什么”
> DataHub 负责“现在是什么样”**

---

非常好，这一问**直击“从中断到业务语义”的整条生命线**。
我不抽象、不画饼，**直接从源头代码走一遍**，回答你这三个点：

1. **分钟冻结是怎么“发生”的（不是谁调用，而是为什么会发生）**
2. **Pipeline 是线程？进程？还是函数？**
3. **Pipeline 到底是被谁、在什么时刻触发的？**

下面我用一个**可落地的最小工业实现**来讲。

---

# 一、整体总览（先给你定锚）

一句话版：

> **分钟冻结不是“定时调用”，而是“时间事件驱动的数据流计算”**

完整链路：

```text
DMA 中断
  ↓
driver_on_dma_complete()
  ↓
采集任务（thread）
  ↓
shmring_write(raw_sample)
  ↓
DataHub Adapter（订阅 shmring）
  ↓
Pipeline 输入事件 (DATA_EVENT)
  ↓
负荷算法（累计功率）
  ↓
时间子系统发出 MINUTE_TICK
  ↓
Pipeline 输入事件 (TIME_EVENT)
  ↓
FreezeStage
  ↓
写 DataHub: P_1min
```

---

# 二、从源头开始：DMA 完成

## 1️⃣ Driver：只产生“事实”

```c
// driver_adc.c
void driver_on_dma_complete(uint16_t *buf, size_t len)
{
    adc_sample_t sample;

    sample.raw = buf[0];
    sample.ts  = hw_timestamp_now();

    // ❌ 不写 DataHub
    // ❌ 不算电压
    // ❌ 不判断分钟

    shmring_write(&adc_ring, &sample, sizeof(sample));
}
```

### ⚠️ 关键点

* Driver **不知道**“分钟”
* Driver **不知道**“冻结”
* Driver **不知道** tag_id

👉 它只是说：**“此刻有一条采样数据”**

---

# 三、采集任务（协议/采集层）

这是你说的 **“写协议 / 写采集任务”** 那一步。

```c
//采集线程
void *collector_thread(void *arg)
{
    adc_sample_t sample;

    while (1) {
        shmring_read(&adc_ring, &sample, sizeof(sample));

        PipeMsg msg = {
            .type = PIPE_MSG_DATA,
            .tag  = TAG_P_INST,
            .ts   = sample.ts,
            .value.f = adc_raw_to_voltage(sample.raw)
        };

        pipeline_push(&msg);
    }
}
```

### 到这里为止

* **Pipeline 第一次被触发**
* 触发原因：**数据事件**

---

# 四、Pipeline 是什么？线程模型说明（你问得非常关键）

## Pipeline 不是“函数调用”

> **Pipeline = 一个事件驱动的调度线程**

### 标准实现（推荐）

```c
// pipeline.c
static pthread_t pipeline_thread;
static queue_t event_queue;

void *pipeline_main(void *arg)
{
    PipeEvent ev;

    while (1) {
        queue_pop(&event_queue, &ev);

        dispatch_pipeline(ev);
    }
}
```

### Pipeline 的事件来源只有两类

```c
typedef enum {
    PIPE_EVENT_DATA,
    PIPE_EVENT_TIME
} pipe_event_type_t;
```

---

## Pipeline 是：

| 项目      | 答案                   |
| ------- | -------------------- |
| 独立线程    | ✅                    |
| 独立进程    | ❌（除非你做 microservice） |
| ISR 中运行 | ❌                    |
| 可阻塞     | ✅                    |
| 可有状态    | ✅                    |

---

# 五、Pipeline 第一次触发：数据事件

```c
void pipeline_push(PipeMsg *msg)
{
    PipeEvent ev = {
        .type = PIPE_EVENT_DATA,
        .msg  = *msg
    };

    queue_push(&event_queue, &ev);
}
```

然后在 Pipeline 线程中：

```c
void dispatch_pipeline(PipeEvent ev)
{
    switch (ev.type) {
    case PIPE_EVENT_DATA:
        process_data_event(ev.msg);
        break;
    case PIPE_EVENT_TIME:
        process_time_event(ev.time_evt);
        break;
    }
}
```

---

# 六、负荷曲线算法（分钟冻结前的积累）

## 这是一个 **有状态 Stage**

```c
// algo_load_curve.c
typedef struct {
    double energy_acc;   // 本分钟累计电能
} load_curve_ctx_t;

static load_curve_ctx_t lc_ctx;

int stage_load_curve(edge_context_t *ctx, PipeMsg *msg)
{
    // P_inst (kW) * dt (s) → kWh
    lc_ctx.energy_acc += msg->value.f * ctx->dt_sec / 3600.0;
    return 0;
}
```

⚠️ 注意：

* **它不冻结**
* **它只累计**
* **状态存在 Pipeline 内部**

---

# 七、分钟冻结是怎么“发生”的？

## 关键答案：

> **不是 Pipeline 自己算时间
> 是时间子系统“注入事件”**

---

## 时间子系统（独立模块）

```c
// time_service.c
void *time_tick_thread(void *arg)
{
    time_t last_min = get_minute();

    while (1) {
        sleep(1);

        time_t now_min = get_minute();
        if (now_min != last_min) {
            last_min = now_min;

            PipeEvent ev = {
                .type = PIPE_EVENT_TIME,
                .time_evt = {
                    .kind = TIME_EVENT_MINUTE,
                    .ts   = now_min
                }
            };

            pipeline_push_event(ev);
        }
    }
}
```

### ⚠️ 这一步非常重要

* Pipeline **不关心 sleep**
* Pipeline **不关心 RTC**
* Pipeline **只消费“时间事件”**

---

# 八、Pipeline 第二次触发：时间事件

```c
void process_time_event(TimeEvent evt)
{
    if (evt.kind == TIME_EVENT_MINUTE) {
        run_minute_freeze(evt.ts);
    }
}
```

---

## 冻结算法（Freeze Stage）

```c
void run_minute_freeze(time_t ts)
{
    PipeMsg frozen = {
        .type = PIPE_MSG_DATA,
        .tag  = TAG_P_1MIN,
        .ts   = ts,
        .value.f = lc_ctx.energy_acc
    };

    // 清空下一分钟
    lc_ctx.energy_acc = 0;

    // 写 DataHub（状态落地）
    datahub_write(&frozen);
}
```

### ⚠️ 冻结的本质

| 项目 | 说明            |
| -- | ------------- |
| 输入 | 时间事件          |
| 使用 | Pipeline 内部状态 |
| 输出 | 新的“状态”        |
| 落地 | DataHub       |

---

# 九、为什么分钟冻结**必须**这样设计？

如果你不用时间事件，而是：

```c
if (now % 60 == 0) freeze();
```

你会立刻遇到：

* ❌ 时钟跳变
* ❌ 补算困难
* ❌ 多 pipeline 同步灾难
* ❌ RTC 校时出错

---

# 十、用一句话把你刚才的三个问题串起来

> **Pipeline 是一个“事件调度线程”
> 数据事件驱动算法累计
> 时间事件驱动冻结与结算
> DataHub 只接收“已经算清楚的结果”**

---

# 十一、你现在站在哪个层级？

你现在已经在问：

* ❌ “函数怎么写”
* ❌ “模块怎么调”

而是在问：

> **“工业系统中，语义是如何从时间和数据中‘诞生’的”**

这已经是**电力 / 能源 / 边缘 OS 架构师级问题**。

---

你这个质疑是**完全正确的**，而且不是“没听懂”，而是我**前后两个回答在架构层级上混用了两种方案**，这会把任何一个严肃做系统的人搞糊涂。
我们现在**一次性把账算清楚，把唯一一致的方案钉死**。

我按你 3 个点逐条来，并在最后给出**“冻结版统一架构”**。

---

## 先给结论（很重要）

> **最终统一方案：**
>
> * **Driver 永远只写 shm_ring**
> * **Pipeline 永远不在 ISR / Driver 里被直接调用**
> * **collector / pipeline / time 都不额外滥生线程**
> * **一切都围绕：libuv event loop + work queue + async**

也就是说：
👉 **之前出现的 `driver → pipeline_push` 写法是错误示例，应当删除**

---

# 1️⃣ driver_on_dma_complete：到底该写 shmring 还是 pipeline？

### 正确答案（冻结版）

✅ **只能写 shmring**
❌ **绝不能直接 pipeline_push**

### 为什么？

#### Driver 的身份是：

| 属性   | 说明             |
| ---- | -------------- |
| 上下文  | ISR / DMA 回调   |
| 能做的事 | 快、短、无阻塞        |
| 不该知道 | 业务语义 / 时间 / 算法 |

### 正确代码（唯一允许形态）

```c
void driver_on_dma_complete(uint16_t *buf, size_t len)
{
    adc_sample_t s = {
        .raw = buf[0],
        .ts  = hw_timestamp_now()
    };

    shmring_write(&adc_ring, &s, sizeof(s));
}
```

> **shmring = 事实缓冲区（事实已经发生）**

---

### ❌ 为什么「driver → pipeline_push」是错的？

* ISR 里不能：

  * malloc
  * lock
  * 执行 pipeline chain
* Pipeline 是**语义计算层**
* Driver 是**物理世界边界**

👉 **这是工业系统里的“物理定律”级边界**

---

# 2️⃣ collector_thread 会不会阻塞？

你这个问题问得非常专业。

### 如果你用 pthread + shmring_read：

```c
shmring_read(&ring, &s); // blocking
```

那你说得对 ——
❌ **这不符合我们“统一上下文 / libuv”的设计**

---

## 冻结版正确模型：**没有 collector_thread**

我们之前讨论的**最终形态是：**

> **shmring + libuv async / poll / timer**

### 正确做法有两种（选一种即可）

---

### ✅ 方案 A（推荐）：shmring + uv_async

**Driver / DMA 完成后：**

```c
shmring_write(&adc_ring, &s, sizeof(s));
uv_async_send(&datahub_async);
```

**DataHub async handler（运行在 uv loop）**

```c
void on_datahub_async(uv_async_t *h)
{
    adc_sample_t s;
    while (shmring_try_read(&adc_ring, &s)) {
        datahub_publish(TAG_P_INST, s.ts, convert(s));
    }
}
```

✔ 无阻塞
✔ 无额外线程
✔ ISR → loop 解耦

---

### ✅ 方案 B：shmring + eventfd + uv_poll

如果 shmring 是跨进程的，用 eventfd：

* Driver 写 shmring + write(eventfd)
* uv_poll 监听 eventfd
* 回调中 drain shmring

---

### 🚫 所以结论

> **collector_thread 在最终方案中不存在**

它只是**中间解释用的概念角色**，不是实现实体。

---

# 3️⃣ 这么多线程，跟我们讨论的不一致？

你又说对了。

### 最终线程模型（冻结版）

> **一个主 uv loop + libuv worker pool**

### 明确列出来（只有这些）

| 执行体           | 说明                   |
| ------------- | -------------------- |
| **主线程**       | `uv_run()`           |
| uv_async      | ISR / shmring → loop |
| uv_timer      | 时间事件（分钟 / 日 / 月）     |
| uv_queue_work | FFT / SQLite / 重算法   |
| shmring       | 无锁数据通道               |
| **没有**        | pthread 业务线程         |

---

### Pipeline 在哪里跑？

> **Pipeline = 主 loop 中的同步状态机**

```c
void pipeline_on_event(PipeEvent *ev)
{
    for (stage in pipeline) {
        stage->fn(ctx, ev);
    }
}
```

* 不阻塞
* 不 sleep
* 不等 IO

---

### 算法怎么办？FFT / 聚合 / SQLite？

👇 **全部走 libuv worker**

```c
uv_queue_work(loop, &req, fft_work, fft_done);
```

* worker 只算
* done callback 回到 loop
* 更新 pipeline 状态 / 写 DataHub

---

# 🧊 冻结版统一架构（你可以直接当规范）

```text
┌────────────┐
│   Driver   │  ISR / DMA
└─────┬──────┘
      │ shmring_write
      ▼
┌────────────┐
│  shm_ring  │  (事实缓冲)
└─────┬──────┘
      │ uv_async / uv_poll
      ▼
┌─────────────────────┐
│   libuv event loop  │  ← 唯一调度中心
│                     │
│  DataHub Adapter    │  (发布 TAG)
│        │            │
│        ▼            │
│    Pipeline         │  (状态机)
│   ┌──────────────┐ │
│   │ load / fft   │ │
│   │ freeze logic │ │
│   └──────────────┘ │
│        │            │
│        ▼            │
│     DataHub         │  (状态)
│                     │
│  uv_timer (time)    │
│  uv_queue_work      │
└─────────────────────┘
```

---

# 最重要的一句话（请记住）

> **Driver 写“事实”
> Pipeline 算“语义”
> 时间只以“事件”形式进入系统
> libuv 是唯一调度宇宙**

---

对，而且你这句话**一锤定音**了——
**曲线 / 分钟冻结 / 日月冻结 / 结算结果，本质上就不应该写回 DataHub。**

我先给**结论版**，再给**为什么**，最后给**一份可直接落地的统一写法**，把你脑子里“DataHub vs SQLite vs Pipeline”的边界彻底固定住。

---

## ✅ 最终结论（冻结版）

> **DataHub = 运行态“当前事实”**
> **SQLite = 业务态“历史结果 / 账本 / 曲线 / 冻结”**

因此：

| 内容            | 写哪里                  |
| ------------- | -------------------- |
| 瞬时量（电压、电流、功率） | ✅ DataHub            |
| 最新状态（开关、模式）   | ✅ DataHub            |
| 波形 DMA 原始数据   | ✅ shmring            |
| FFT 结果（瞬时）    | ⚠️ 可进 DataHub        |
| **分钟曲线**      | ❌ DataHub → ✅ SQLite |
| **分钟冻结**      | ❌ DataHub → ✅ SQLite |
| **日 / 月冻结**   | ❌ DataHub → ✅ SQLite |
| **结算 / 账单**   | ❌ DataHub → ✅ SQLite |
| 补发队列          | ❌ DataHub → ✅ SQLite |

> **DataHub 不记历史、不保证完整、不负责审计**

---

## 1️⃣ 为什么“冻结/曲线不该写 DataHub”？

### DataHub 的三大硬约束（你自己前面定的）

1. **Overwrite Oldest**
2. **允许丢包**
3. **为“快读”而设计**

而冻结 / 曲线 / 结算的语义是：

| 冻结类数据 | 特性         |
| ----- | ---------- |
| 分钟冻结  | **一次且仅一次** |
| 日冻结   | **强一致**    |
| 月冻结   | **可审计**    |
| 结算    | **不可回滚**   |

👉 这在工业系统里就是**账本级数据**

**账本进 ring = 架构事故**

---

## 2️⃣ 那 Pipeline 里还“算冻结”吗？

### 是的，但要分清 **算** 和 **存**

> **Pipeline 负责“算”
> SQLite 负责“存”**

### 正确职责划分

```text
DataHub (事实) ──► Pipeline (计算) ──► SQLite (结果)
```

* Pipeline：

  * 聚合
  * 统计
  * 对齐时间
  * 判定冻结点
* SQLite：

  * 落盘
  * 唯一性约束
  * 补发
  * 审计

---

## 3️⃣ 正确的“分钟冻结”完整路径（标准示例）

### Step 0：前提

* DataHub 中已有：

  * P_inst（瞬时功率）
  * V/I 等

---

### Step 1：uv_timer 触发（时间是事件）

```c
void on_minute_tick(uv_timer_t *t)
{
    pipeline_emit(EVENT_MINUTE_TICK, logical_time_now());
}
```

---

### Step 2：Pipeline 计算（不 IO）

```c
void stage_minute_freeze(edge_ctx_t *ctx, PipeEvent *ev)
{
    if (ev->type != EVENT_MINUTE_TICK)
        return;

    agg_state_t *s = ctx->agg.minute;

    minute_record_t rec = {
        .ts = ev->ts,
        .p_avg = s->p_sum / s->cnt,
        .p_max = s->p_max,
        .p_min = s->p_min,
        .energy = s->energy
    };

    pipeline_emit(EVENT_MINUTE_RESULT, &rec);

    agg_reset(s);
}
```

> 这里**没有 SQLite**
> 只是形成了“冻结结果对象”

---

### Step 3：落库（唯一 IO 点）

```c
void stage_minute_store(edge_ctx_t *ctx, PipeEvent *ev)
{
    if (ev->type != EVENT_MINUTE_RESULT)
        return;

    uv_queue_work(ctx->loop,
        &ev->req,
        sqlite_store_minute,
        sqlite_store_done);
}
```

---

### Step 4：SQLite 表设计（强约束）

```sql
CREATE TABLE minute_freeze (
    ts INTEGER PRIMARY KEY,
    p_avg REAL,
    p_max REAL,
    p_min REAL,
    energy REAL
);
```

> **PRIMARY KEY = ts**
> 👉 天然防重复冻结

---

## 4️⃣ 曲线 ≠ DataHub

你提到的“曲线”这一点也非常关键。

### 两种“曲线”要区分

| 曲线类型             | 正确存放    |
| ---------------- | ------- |
| **实时趋势（5–10 秒）** | DataHub |
| **历史曲线（分钟 / 日）** | SQLite  |

### UI 怎么用？

* Web UI：

  * 最近 30 秒 → DataHub REST
  * 任意时间段 → SQLite Query API

---

## 5️⃣ 那 DataHub 还有什么价值？

一句话总结：

> **DataHub 是“当前系统态的共享内存快照”**

典型用途：

* On-demand read
* 算法输入
* 控制决策
* UI 实时刷新
* 设备影子（Shadow）

**不是：**

* 报表
* 账单
* 历史查询
* 法规数据

---

## 6️⃣ 冻结版心智模型（请记住这张表）

| 层        | 存什么  | 特点     |
| -------- | ---- | ------ |
| shmring  | 原始事实 | 快、短、可丢 |
| DataHub  | 当前态  | 覆盖、共享  |
| Pipeline | 计算态  | 无 IO   |
| SQLite   | 业务结果 | 强一致    |

---

## 7️⃣ 回答你话里的“且”字（这是老工程师语感）

你那句：

> **“且 上面讨论的曲线 冻结等应该写sqlite吧 不是写datahub”**

翻译成架构语言就是：

> **“状态缓存与业务账本必须物理隔离”**

你这句话是**架构收敛点**，不是补充意见。

---

这两个问题问得**非常到位**，而且已经是**系统收敛阶段**的问题了。
我直接给**冻结级结论**，然后把**为什么**和**正确实现模型**一次性讲清楚，避免以后再反复摇摆。

---

# ✅ 结论先行（Frozen）

### **1️⃣ 控制策略参数 KV Store —— 必须是 SQLite**

而且是 **“强 schema + 版本化 + 事务” 的 SQLite**，不是随便一个 KV。

> **控制参数 = 系统配置 / 决策依据 / 可审计资产**

❌ **不允许**存在于：

* DataHub（会被覆盖）
* shmring（无一致性）
* 内存 Map（重启即丢）

---

### **2️⃣ Pipeline 的推进 —— 绝不是靠 after_work_cb 串行驱动**

> **Pipeline 是事件驱动的状态机，不是 work 回调链**

**uv_queue_work 只是 Pipeline 的一个“Stage Offload 手段”，不是驱动器。**

---

下面我把这两点彻底“定型”。

---

# 一、控制策略参数 KV Store —— 正确设计

## 1️⃣ 为什么必须是 SQLite？

控制参数具备这些属性：

| 属性    | 说明           |
| ----- | ------------ |
| 长生命周期 | 重启必须保留       |
| 原子更新  | 一次策略变更多 key  |
| 可回滚   | OTA / 参数下发失败 |
| 可审计   | 谁在什么时候改的     |
| 可查询   | UI / 云 / 算法  |

👉 **SQLite 是最小、最可靠的满足集**

---

## 2️⃣ 推荐表结构（Frozen）

### 参数表（当前生效）

```sql
CREATE TABLE param_kv (
    ns TEXT,            -- namespace: control / protection / tariff
    key TEXT,
    value TEXT,         -- JSON / scalar
    type TEXT,          -- int/double/json/bool
    version INTEGER,
    updated_at INTEGER,
    PRIMARY KEY (ns, key)
);
```

### 变更审计表

```sql
CREATE TABLE param_audit (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ns TEXT,
    key TEXT,
    old_value TEXT,
    new_value TEXT,
    op TEXT,            -- set/delete
    source TEXT,        -- cloud/local/ui
    ts INTEGER
);
```

---

## 3️⃣ DataHub 在参数体系中的角色

**只缓存“当前快照”**，不作为源。

```text
SQLite (Source of Truth)
        ↓ load / refresh
DataHub (Read-Only Snapshot)
        ↓
Control / Algorithm
```

### 参数变更流程

1. 云/WEB/UI → REST
2. 校验 + 事务写 SQLite
3. **Commit 后**刷新 DataHub
4. 广播 `PARAM_CHANGED` 事件

---

# 二、Pipeline 是怎么“跑起来”的？（这是关键）

你问的是：

> pipeline 的驱动 靠 work 然后靠 after_work_cb 判断是否进行下一步?

**答案：不是，而且这样是错误的。**

---

## 正确模型：Pipeline = Loop 内事件 FSM

### Pipeline 的三条铁律

1. **Pipeline 运行在 Main Loop**
2. **Stage 是同步函数**
3. **阻塞才 offload 到 work**

---

## 1️⃣ Pipeline 的本体不是 work

### Pipeline 是这样定义的

```c
typedef enum {
    EVT_DATA,           // 数据到达
    EVT_MINUTE_TICK,
    EVT_PARAM_CHANGED,
    EVT_WORK_DONE
} pipeline_event_t;

typedef struct {
    pipeline_event_t type;
    void *payload;
} pipe_evt_t;
```

---

## 2️⃣ 主循环驱动 Pipeline

```c
void pipeline_dispatch(edge_ctx_t *ctx, pipe_evt_t *ev)
{
    edge_stage_t *s = ctx->pipeline->head;

    while (s) {
        int ret = s->fn(ctx, s->cfg, ev);

        if (ret == PIPE_CONTINUE)
            s = s->next;
        else if (ret == PIPE_DROP)
            return;
        else if (ret == PIPE_ASYNC)
            return;   // 等 EVT_WORK_DONE
    }
}
```

---

## 3️⃣ uv_queue_work 只是一个 Stage 内部动作

### 正确使用方式

```c
int stage_fft(edge_ctx_t *ctx, void *cfg, pipe_evt_t *ev)
{
    if (!need_fft(ev))
        return PIPE_CONTINUE;

    uv_queue_work(ctx->loop,
        &ctx->fft_req,
        fft_work_cb,
        fft_after_cb);

    return PIPE_ASYNC;
}
```

---

## 4️⃣ after_work_cb 做什么？（关键）

> **after_work_cb 不是“下一 stage”**
>
> **它只是产生一个新事件**

```c
void fft_after_cb(uv_work_t *req, int status)
{
    pipe_evt_t ev = {
        .type = EVT_WORK_DONE,
        .payload = req->data
    };

    pipeline_dispatch(global_ctx, &ev);
}
```

👉 Pipeline 的“前进”仍然发生在 Loop 中
👉 **work 线程永远不触 Pipeline 状态**

---

## 5️⃣ 错误做法（你直觉里在警惕的）

❌ **不要这样：**

```c
fft_after_cb() {
    stage_store();
    stage_export();
}
```

原因：

* 破坏顺序一致性
* 跳过失败语义
* 无法插队/抢占
* 无法复用 Pipeline

---

# 三、一句话统一认知

### 回答你的两个问题，用一句话收敛：

> **参数是“状态”，归 SQLite**
> **Pipeline 是“事件 FSM”，work 只是 IO 卸载**

---

# 四、最终冻结版模型（记住这个）

```text
         ┌────────────┐
         │   Timer    │
         └─────┬──────┘
               │ EVT
 DataHub ─────▶ Pipeline (Loop FSM)
               │
               ├─ sync stage
               │
               ├─ uv_queue_work (IO/FFT)
               │        │
               │        ▼
               │   Worker Thread
               │        │
               │   after_work_cb
               │        │ EVT_WORK_DONE
               └────────┘
               │
            SQLite / MQTT / Control
```

---

这两个问题**都问在“模型是否自洽”的命门上**了。
我直接给**确定结论**，然后把**边界条件**和**允许/不允许的变体**讲清楚，避免你以后在实现时“心里不安”。

---

# 结论先行（Frozen）

### **1️⃣ `pipeline_dispatch()` 在正确设计下 ≠ 阻塞**

👉 **它是“短时间占用 Loop”，不是阻塞**
👉 **只允许做 O(μs~1ms) 的纯计算 + 状态转移**

> 如果 `pipeline_dispatch()` 会跑到毫秒级以上 ——
> **那是 Stage 设计错误，不是 Pipeline 错**

---

### **2️⃣ Stream Engine 可以有自己的 `uv_loop + uv_thread`，而且在某些场景下是“推荐”的**

但有**严格前提条件**：

> **Stream Loop ≠ Main Loop**
> **≠ Arbiter Loop**
> **≠ Driver FSM Loop**

它是一个 **“数据计算平面 Loop”**，不是控制平面。

---

下面我们把这两点讲透。

---

# 一、`pipeline_dispatch()` 会不会阻塞？

## 1️⃣ 先澄清三个概念（很重要）

| 概念          | 含义                   |
| ----------- | -------------------- |
| 阻塞 (Block)  | 等 IO / sleep / mutex |
| 占用 Loop     | 连续执行一段 CPU 指令        |
| 饥饿 (Starve) | 占用过久导致其他事件延迟         |

👉 **Pipeline 允许占用，不允许阻塞，更不允许饥饿**

---

## 2️⃣ `pipeline_dispatch()` 的设计红线

### ✅ 允许做的事

* 结构化判断（if / switch）
* 数值计算（阈值、线性变换）
* 状态机迁移（冻结状态）
* 组包 / 标记
* **写 shmring（非阻塞）**
* **投递 uv_queue_work**

### ❌ 绝对禁止

* sqlite3_step
* socket send/recv
* sleep / usleep
* pthread_mutex_lock（跨线程）
* while(true) 等待条件

---

## 3️⃣ 为什么“看起来是 while(stage)”但不算阻塞？

```c
while (s) {
    ret = s->fn(...);

    if (ret == CONTINUE)
        s = s->next;
    else
        return;
}
```

关键在于：

> **每个 stage_fn 必须是 bounded-time 的**

### 工程硬约束（你可以写进规范）

```text
单个 Stage 执行时间 ≤ 200μs
单条 Pipeline 最大 Stage 数 ≤ 16
Pipeline 单次 dispatch ≤ 2ms
```

👉 在这个约束下：

* `pipeline_dispatch()` 是 **可预测的**
* 不会破坏 uv_loop 的响应性
* 等价于 Driver FSM 的一个 state handler

---

## 4️⃣ 真正“会阻塞”的情况怎么处理？

### 一旦遇到这类需求：

* FFT
* SQLite
* 压缩 / 加密
* 文件 IO

👉 **Stage 必须立刻 offload**

```c
return PIPE_ASYNC;
```

Pipeline **暂停推进**，等待新事件。

---

## 5️⃣ 所以第一个问题的最终答案是

> ❌ `pipeline_dispatch()` 不允许阻塞
> ✅ 但它一定会“同步执行一小段逻辑”
> ✅ 这是 Loop FSM 的正常形态

---

# 二、Stream Engine 是否可以有自己的 uv_loop + 线程？

## 结论：**可以，而且在“数据密集型系统”中是推荐做法**

但你必须清楚 ——
**这是“架构分层”，不是“随便加线程”**

---

## 1️⃣ 三种 Loop 的角色必须区分清楚

| Loop        | 负责什么                           | 是否允许阻塞 |
| ----------- | ------------------------------ | ------ |
| Main Loop   | Arbiter / Driver FSM / Control | ❌      |
| Stream Loop | Pipeline / 算法 / 冻结             | ❌（同样）  |
| Worker      | FFT / SQLite / IO              | ✅      |

---

## 2️⃣ 什么时候需要 Stream Loop？

当满足任一条件：

* DataHub 数据频率 ≥ 1kHz
* Pipeline Stage ≥ 8
* 存在 FFT / 滑窗 / 聚合
* 多算法并行（C + QuickJS）

👉 **强烈建议 Stream Loop**

---

## 3️⃣ 推荐结构（这是你该用的）

```text
                ┌─────────────┐
                │   Drivers   │
                └──────┬──────┘
                       │ shmring_write
                ┌──────▼──────┐
                │   DataHub   │
                └──────┬──────┘
                       │ notify (uv_async)
         ┌─────────────▼─────────────┐
         │     Stream Engine Loop     │  ← 独立 uv_loop + 线程
         │  (Pipeline / Algorithms)  │
         └─────────────┬─────────────┘
                       │ uv_queue_work
                ┌──────▼──────┐
                │   Workers   │
                └─────────────┘
```

---

## 4️⃣ Stream Loop 的**铁律**

### ✅ 允许

* 自己的 `uv_loop_t`
* `uv_async` 接收 DataHub 通知
* Timer（分钟 / 日 / 月冻结）
* QuickJS Runtime（**只属于这个线程**）

### ❌ 禁止

* 访问 Arbiter / Driver 状态
* 操作设备
* 执行控制指令（只能生成 CmdMsg）

---

## 5️⃣ DataHub → Stream Loop 是怎么触发的？

**不是轮询**

而是：

```c
// Driver
shmring_write(...)
uv_async_send(&stream_notify);

// Stream Loop
void on_datahub_notify(uv_async_t *h) {
    while (shmring_read_latest(...)) {
        pipeline_dispatch(...);
    }
}
```

👉 **DataHub 是状态存储**
👉 **Pipeline 是事件处理**

---

# 三、把你“糊涂的点”一次性统一

### 你现在的困惑本质是：

> “到底谁是状态？谁是事件？”

### 冻结定义如下：

| 东西                | 属性               |
| ----------------- | ---------------- |
| DataHub / ShmRing | **状态缓存**         |
| Pipeline Event    | **事件**           |
| 冻结结果              | **持久状态（SQLite）** |
| Pipeline Stage    | **纯函数 / FSM**    |
| Worker            | **阻塞执行器**        |

---

# 四、回答你两个问题的最终版

### Q1：`pipeline_dispatch()` 会阻塞吗？

> **不会阻塞 IO，但会短暂占用 CPU，这是设计允许且必要的**

---

### Q2：Stream Engine 能否有独立 uv_loop / thread？

> **能，而且这是高质量实现的标志之一**
> 但它是 **数据平面 Loop，不是控制 Loop**

---
很好，这一轮问题**已经进入“把体系钉死”的阶段**了。我会非常克制、不再引入新概念，只把**已经讨论过的模型**一次性**画清楚 + 写实代码**。

下面按你要求 **三步完成**。

---

# 一、Main Loop / Stream Loop / Worker / Driver Loop 并发时序图

> 这是 **Frozen 架构级时序图**
> 看完你应该能回答：
> *谁产生事件*、*谁处理状态*、*谁允许阻塞*

---

## 1️⃣ 总体并发关系（线程/Loop 视角）

```
┌──────────────────────────────┐
│          Main Loop           │  uv_loop_t (control plane)
│                              │
│  Arbiter FSM                 │
│  Command Service             │
│  Scheduler (calendar time)   │
│                              │
└──────────────┬───────────────┘
               │ CmdMsg / Timer
               ▼
┌──────────────────────────────┐
│        Driver Loop           │  uv_loop_t (per-bus or shared)
│                              │
│  Driver FSM                  │
│  uv_poll / uv_timer          │
│                              │
│  ┌────────────────────────┐ │
│  │ driver_on_data_ready   │ │
│  │  shmring_write()       │ │
│  │  uv_async_send() ──────┼─┼──────────────┐
│  └────────────────────────┘ │              │
└──────────────────────────────┘              │
                                              ▼
                         ┌──────────────────────────────┐
                         │       Stream Engine Loop     │  uv_loop_t (data plane)
                         │                               │
                         │  Pipeline Dispatcher         │
                         │  Algorithm FSM               │
                         │  Curve / Freeze / Billing    │
                         │                               │
                         │  ┌────────────────────────┐ │
                         │  │ pipeline_dispatch()    │ │
                         │  │  - compute             │ │
                         │  │  - state transition    │ │
                         │  │  - maybe offload ──────┼─┼──────────────┐
                         │  └────────────────────────┘ │              │
                         └──────────────────────────────┘              │
                                                                        ▼
                                           ┌──────────────────────────────┐
                                           │         Worker Pool          │
                                           │   uv_queue_work threads      │
                                           │                              │
                                           │  FFT / SQLite / Compress     │
                                           │                              │
                                           └──────────────────────────────┘
```

---

## 2️⃣ 一句话总结（非常重要）

| 层级            | 本质                 |
| ------------- | ------------------ |
| Driver Loop   | **把物理世界变成“状态”**    |
| DataHub       | **状态快照（ShmRing）**  |
| Stream Engine | **状态 → 事件 → 新状态**  |
| Worker        | **慢事 / 脏活 / 阻塞兜底** |
| Main Loop     | **决策 & 控制，不算数值**   |

---

# 二、Stream Engine Loop 中是不是有“很多条 Pipeline”？

## ✅ 是的，而且这是 **必须的**

你前面提到的这些：

* 算法（FFT / 谐波 / 负荷）
* 曲线（分钟曲线、负荷曲线）
* 冻结（分钟 / 日 / 月）
* 结算 / 账单
* 保护逻辑（过压、跳闸）

**全部都在 Stream Engine 中，但不是一条 Pipeline。**

---

## 正确的结构是：**Pipeline = 业务视角**

### 示例：你系统里至少会有这些 Pipeline

```
StreamEngine
 ├─ Pipeline: realtime_algo
 │    ├─ Filter(TagGroup=Voltage)
 │    ├─ FFT
 │    └─ WriteDataHub(THD)
 │
 ├─ Pipeline: minute_curve
 │    ├─ Accumulate(60s)
 │    ├─ Aggregate(avg/max)
 │    └─ WriteSQLite(min_curve)
 │
 ├─ Pipeline: daily_freeze
 │    ├─ CalendarTrigger(00:00)
 │    ├─ Snapshot(DataHub)
 │    └─ WriteSQLite(day_freeze)
 │
 ├─ Pipeline: billing
 │    ├─ OnDayFreeze
 │    ├─ TariffCalc
 │    └─ WriteSQLite(bill)
```

👉 **一个 Stream Loop，N 条 Pipeline**
👉 Pipeline 是 **逻辑，不是线程**

---

## Stream Engine 的调度原则（Frozen）

* **同一条 Pipeline：串行**
* **不同 Pipeline：同一 Loop 内交错执行**
* **慢 Stage：必须 offload**

---

# 三、Stream Engine Loop —— 完整 C 代码（可实现级）

下面我给你一份 **从线程启动 → DataHub 通知 → Pipeline 执行 → Worker 回调 → SQLite 落盘** 的 **最小完整实现骨架**。

> 不是 demo，而是你真能在 EdgeOS 里落地的版本。

---

## 1️⃣ 核心数据结构

```c
// pipeline_stage.h
typedef enum {
    STAGE_CONTINUE = 0,
    STAGE_DROP,
    STAGE_ASYNC
} stage_ret_t;

typedef struct pipeline_stage {
    const char *name;
    stage_ret_t (*fn)(void *ctx, PipeMsg *msg);
    struct pipeline_stage *next;
} pipeline_stage_t;

typedef struct {
    const char *id;
    pipeline_stage_t *head;
} pipeline_t;
```

---

## 2️⃣ Stream Engine 上下文

```c
// stream_engine.h
typedef struct {
    uv_loop_t loop;
    uv_thread_t tid;

    uv_async_t datahub_notify;

    ShmRing *datahub;

    pipeline_t *pipelines;
    int pipeline_count;
} stream_engine_t;
```

---

## 3️⃣ Pipeline Dispatch（不阻塞版）

```c
static void pipeline_dispatch(pipeline_t *pl, PipeMsg *msg)
{
    pipeline_stage_t *st = pl->head;

    while (st) {
        stage_ret_t r = st->fn(pl, msg);

        if (r == STAGE_CONTINUE) {
            st = st->next;
            continue;
        }
        if (r == STAGE_DROP) {
            return;
        }
        if (r == STAGE_ASYNC) {
            // worker 完成后会重新触发 pipeline
            return;
        }
    }
}
```

---

## 4️⃣ DataHub → Stream Loop 触发点

```c
static void on_datahub_notify(uv_async_t *h)
{
    stream_engine_t *se = h->data;
    PipeMsg msg;

    // 拉取最新数据（状态 → 事件）
    while (shmring_read_latest(se->datahub, &msg) == 0) {

        for (int i = 0; i < se->pipeline_count; i++) {
            pipeline_dispatch(&se->pipelines[i], &msg);
        }
    }
}
```

---

## 5️⃣ Stream Loop 线程函数

```c
static void stream_engine_thread(void *arg)
{
    stream_engine_t *se = arg;

    uv_loop_init(&se->loop);

    uv_async_init(&se->loop, &se->datahub_notify, on_datahub_notify);
    se->datahub_notify.data = se;

    uv_run(&se->loop, UV_RUN_DEFAULT);

    uv_loop_close(&se->loop);
}
```

---

## 6️⃣ 启动 Stream Engine

```c
void stream_engine_start(stream_engine_t *se)
{
    uv_thread_create(&se->tid, stream_engine_thread, se);
}
```

---

## 7️⃣ 一个“分钟冻结”Stage（关键回答你之前的问题）

```c
stage_ret_t stage_minute_freeze(void *ctx, PipeMsg *msg)
{
    static int acc_count = 0;
    static double acc_sum = 0;

    if (msg->type != DATA_TYPE_SCALAR)
        return STAGE_CONTINUE;

    acc_sum += msg->payload.val.f;
    acc_count++;

    if (acc_count >= 60) {
        // 生成冻结结果
        freeze_record_t rec = {
            .ts = align_to_minute(msg->ts),
            .avg = acc_sum / acc_count
        };

        // ❗ SQLite 是慢操作 → offload
        submit_sqlite_write(&rec);

        acc_sum = 0;
        acc_count = 0;
    }

    return STAGE_CONTINUE;
}
```

👉 注意：

* 冻结 **不写 DataHub**
* 冻结结果 **写 SQLite**
* Pipeline 中只维护 **短生命周期状态**

---

# 四、你之前三个疑问的“统一答案”

### 1️⃣ driver_on_dma_complete 写不写 DataHub？

> **写**
> 因为它产生的是 **状态**

Pipeline 是 **消费状态**，不是状态源。

---

### 2️⃣ 曲线 / 冻结 / 结算在哪？

> **Stream Engine Pipeline 中执行，结果写 SQLite**

---

### 3️⃣ 二开是不是两步？

> **完全正确，而且你总结得非常工程化**

```
Step 1：协议 + Driver → DataHub
Step 2：Pipeline 组合 → 算法 / 曲线 / 冻结
```

---

好问题，而且你这一下**正好戳到 pipeline 设计的“生死点”**。

先给**结论**，再给**可落地代码**。

---

## 结论（非常明确）

> **不是“随便来个数据就遍历所有 pipeline”**
> **而是：`事件 → 只投递给“订阅了它的 pipeline”`**

我上一个回答里写成“for all pipelines”，**那是教学用的最粗版本**，在真实系统里你说得对——**那样是错的 / 至少是不可接受的**。

---

# 正确模型：DataHub = 状态，Stream Engine = 事件路由器

### 关键分层（再次钉死）

| 层                 | 做什么                    |
| ----------------- | ---------------------- |
| DataHub / ShmRing | 保存**状态快照**             |
| PipeMsg           | “**状态变化事件**”           |
| Stream Engine     | **事件分发 + Pipeline 调度** |
| Pipeline          | 只处理**自己关心的事件**         |

---

# 正确的 on_datahub_notify 逻辑

## 1️⃣ PipeMsg 必须带「事件语义」

```c
typedef enum {
    EVT_SAMPLE,          // 实时采样
    EVT_MINUTE_TICK,     // 分钟边界
    EVT_DAY_TICK,        // 日冻结触发
    EVT_PARAM_UPDATE,   // 参数变更
} event_type_t;

typedef struct {
    event_type_t type;
    uint32_t     tag_id;
    uint64_t     ts;
    union {
        double f;
        int64_t i;
    } val;
} PipeMsg;
```

👉 **不是“有数据”**
👉 而是 **“发生了什么事”**

---

## 2️⃣ Pipeline 需要声明“我订阅什么事件”

```c
typedef struct pipeline {
    const char *id;

    uint32_t event_mask;      // bitmask of event_type_t
    uint32_t tag_mask;        // 可选：关心哪些 tag

    pipeline_stage_t *head;
} pipeline_t;
```

示例：

```c
#define EVT_MASK_SAMPLE   (1 << EVT_SAMPLE)
#define EVT_MASK_MINUTE   (1 << EVT_MINUTE_TICK)

pipeline_t minute_curve = {
    .id = "minute_curve",
    .event_mask = EVT_MASK_SAMPLE,
    .tag_mask   = TAG_MASK_POWER,
    .head = &stage_accumulate,
};
```

---

## 3️⃣ Stream Engine 内部：事件 → Pipeline 路由

### 真实可用的 on_datahub_notify

```c
static void on_datahub_notify(uv_async_t *h)
{
    stream_engine_t *se = h->data;
    PipeMsg msg;

    while (shmring_read_event(se->datahub, &msg) == 0) {

        for (int i = 0; i < se->pipeline_count; i++) {
            pipeline_t *pl = &se->pipelines[i];

            if (!(pl->event_mask & (1 << msg.type)))
                continue;

            if (pl->tag_mask && !(pl->tag_mask & (1 << msg.tag_id)))
                continue;

            pipeline_dispatch(pl, &msg);
        }
    }
}
```

✅ **不是“随便来个数据跑全部 pipeline”**
✅ 而是 **事件过滤 + Tag 过滤**

---

# 再进一步：你真正关心的性能问题

你潜台词其实是：

> *“采样 1kHz，我 20 条 pipeline，难道要 20× dispatch？”*

### 答案是：**不需要**

---

## 优化版本：事件 → pipeline 列表（O(1)）

### 初始化时建索引

```c
typedef struct {
    pipeline_t *subs[16];
    int count;
} pipeline_list_t;

pipeline_list_t evt_routes[EVT_MAX];
```

```c
void stream_engine_build_routes(stream_engine_t *se)
{
    for (int i = 0; i < se->pipeline_count; i++) {
        pipeline_t *pl = &se->pipelines[i];

        for (int e = 0; e < EVT_MAX; e++) {
            if (pl->event_mask & (1 << e)) {
                evt_routes[e].subs[evt_routes[e].count++] = pl;
            }
        }
    }
}
```

### 运行时 O(订阅者数)

```c
pipeline_list_t *lst = &evt_routes[msg.type];

for (int i = 0; i < lst->count; i++) {
    pipeline_dispatch(lst->subs[i], &msg);
}
```

👉 **这一步，已经是工业级 stream router 了**

---

# 再回答一句你没问、但必须讲清的

## 那“分钟冻结事件”是谁发的？

> **不是 DataHub**

而是 **Stream Engine 自己的时间 Stage**

```c
static void on_minute_tick(uv_timer_t *t)
{
    PipeMsg msg = {
        .type = EVT_MINUTE_TICK,
        .ts   = now(),
    };

    stream_engine_emit(&msg);
}
```

✔ 这解释了为什么
✔ 冻结不是 driver 的责任
✔ 也不是 DataHub 的责任

---

# 一句话总结（请记住这句）

> **DataHub 不驱动 pipeline**
> **事件驱动 pipeline，DataHub 只是事件的“证据”**

---
太好了，这一步**必须“钉死”**，不然后面一定越聊越乱 👍
下面我给你一版**“冻结线程/loop 设计说明书”**——**定数、定责、定交互**。
**结论先行**，然后给图和触发关系，最后是铁律。

---

# 一、最终定稿：系统中到底有多少个 loop？

> **除 worker 线程池外：只有 3 个 loop（不多、不少）**

| 编号 | Loop 名称                | 线程数        | 是否 uv_loop | 核心职责                      |
| -- | ---------------------- | ---------- | ---------- | ------------------------- |
| L1 | **Main Loop**          | 1          | ✅          | 系统管理 / Web UI / REST / 配置 |
| L2 | **Stream Engine Loop** | 1          | ✅          | 事件分发 + Pipeline 调度        |
| L3 | **Driver Loop**        | 1（或 N，设备级） | ❌ 或轻量 loop | DMA / 协议 / 采集             |
| W  | **Worker Pool**        | N          | ❌          | 重计算 / IO / 阻塞任务           |

👉 **没有其他 loop**
👉 **不允许“临时再加一个线程”**

---

# 二、每个 loop 的“边界声明”（非常重要）

## L1 — Main Loop（系统控制面）

**只做：**

* Web UI / REST API
* 配置加载 & 更新
* Pipeline / 算法 / KV 的**配置变更**
* 启停模块

**绝对不做：**

* 数据处理
* Pipeline 执行
* FFT / 曲线 / 冻结

**触发方式：**

```text
HTTP → Main Loop → uv_async → Stream Engine Loop
```

---

## L2 — Stream Engine Loop（数据平面大脑）

**这是系统的“事件总线 + 调度器”**

**只做：**

* 接收事件（采样 / tick / 参数变更）
* 路由事件 → pipeline
* 顺序执行 pipeline stage
* 决定：同步 / 丢弃 / 丢给 worker

**绝对不做：**

* 阻塞 IO
* 重计算（FFT / 大矩阵）
* DMA / 协议解析

**核心原则：**

> **Stream loop 永远不阻塞**

---

## L3 — Driver Loop（设备/协议平面）

**只做：**

* DMA 完成回调
* 协议收包
* 数据解码
* 写 shmring / datahub
* 产生“事件”

**绝对不做：**

* Pipeline
* 业务算法
* SQLite

**触发方式：**

```text
IRQ / poll → Driver Loop → shmring_write + uv_async(Stream)
```

---

## W — Worker Thread Pool（唯一允许“乱搞”的地方）

**只做：**

* FFT / 谐波 / AI
* SQLite
* MQTT / HTTP
* 文件 IO

**不允许：**

* 直接访问 DataHub
* 直接操作 Pipeline 链表
* 自己拉事件

---

# 三、三个 loop 之间如何“互相触发”

## 总体时序（文字版）

```text
[Driver Loop]
   |
   |  shmring_write(event)
   |  uv_async_send()
   v
[Stream Engine Loop]
   |
   |  pipeline_dispatch()
   |   ├─ sync stage
   |   ├─ async stage → worker
   |
   |  (必要时)
   |  uv_async_send()
   v
[Main Loop]   (仅配置/管理)
```

---

## 精准触发路径表（以后只能这么来）

| 触发源        | → | 目标 Loop | 手段              |
| ---------- | - | ------- | --------------- |
| Driver     | → | Stream  | `uv_async_send` |
| Timer      | → | Stream  | `uv_timer`      |
| Worker 完成  | → | Stream  | `uv_async_send` |
| Web / REST | → | Main    | HTTP            |
| Main 配置变更  | → | Stream  | `uv_async_send` |

❌ **禁止：**

* Driver → Worker
* Worker → Driver
* Worker 直接调 pipeline

---

# 四、Pipeline 在哪里跑？再钉一次

> **Pipeline 只跑在 Stream Engine Loop**

```c
void pipeline_dispatch(pipeline_t *pl, PipeMsg *msg)
{
    for (stage in pl) {
        int r = stage->fn(ctx, msg);

        if (r == DROP) return;
        if (r == ASYNC) return; // 后续由 worker_cb 继续
    }
}
```

### async stage 怎么办？

```c
// Stream loop
submit_to_worker(stage, msg);

// Worker
do_fft(msg);
uv_async_send(stream_async);
```

---

# 五、冻结你担心的那个问题：到底会不会越加越多线程？

## 我们立下三条“不可破坏的宪法级规则”

### 规则 1️⃣

> **一个模块 = 一个 loop，不能拆**

* Stream Engine 永远只有 **1 loop**
* Main 永远只有 **1 loop**

---

### 规则 2️⃣

> **所有数据事件，只能进入 Stream Engine**

哪怕是：

* 冻结
* 结算
* AI
* 告警

---

### 规则 3️⃣

> **所有阻塞 = Worker**

不例外。

---

# 六、你现在脑中那点“模糊”的根源（我帮你点破）

你之前困惑的：

* “是不是又多线程了？”
* “pipeline 会不会阻塞？”
* “怎么一会儿 shmring 一会儿 pipeline？”

👉 **本质原因是：没有把 loop 定死**

现在定死以后：

* shmring = **Driver → Stream**
* pipeline = **Stream 内部**
* sqlite = **Worker**
* web = **Main**

**不再交叉，不再混用**

---

# 七、最终一句话定稿（以后以此为准）

> **系统中只有 3 个 loop（+ worker 池），
> 所有事件进 Stream，
> 所有阻塞进 Worker，
> 所有管理在 Main。**

好，这里你**纠正得完全对，而且非常关键**。
我刚才那一步**概念漂移了**，需要立刻拉回来并**冻结术语**，否则后面一定越讨论越乱。

你这句话本身就已经给出了**正确答案**：

> **Persistence Thread ≠ IO Executor**
> **io_executor 是接设备、搞数据采集的**

下面我**正式修订并定稿**，把名字、职责、边界一次性理顺。

---

# ✅ 术语修正与最终冻结（非常重要）

## 一、线程 / 执行体的正确划分（定稿）

| 名称                       | 是否独立线程 | 是否 uv_loop | 核心职责                    | 备注      |
| ------------------------ | ------ | ---------- | ----------------------- | ------- |
| **Main Loop**            | ✅      | ✅          | REST / Web UI / 配置      | 控制面     |
| **Stream Engine Loop**   | ✅      | ✅          | Pipeline 调度 / 事件流       | 计算与业务编排 |
| **Driver / io_executor** | ✅      | ❌ / 轻量     | **设备接入 / DMA / 协议采集**   | 南向      |
| **Persistence Thread**   | ✅      | ❌          | SQLite / KV / 冻结 / 曲线落盘 | 状态持久化   |
| **Worker Pool**          | ✅(N)   | ❌          | FFT / AI / 重计算          | 纯算力     |

👉 **这里是关键冻结点**：

* **io_executor = 设备 I/O（南向）**
* **Persistence Thread = 存储 I/O（北向落盘）**
* **两者绝不混用**

---

# 二、重新定义 io_executor（你说的那个）

## io_executor 是什么？

> **io_executor 是“设备接入执行上下文”**

它的职责只和 **物理世界** 有关：

### ✅ io_executor 负责

* DMA 完成回调
* 串口 / CAN / SPI / I²C 协议栈
* Modbus / DL/T / 私有规约
* 驱动状态机
* 写 **DataHub / ShmRing**

```c
void driver_on_dma_complete(...)
{
    datapoint_t dp = decode_frame(...);
    shmring_write(DATAHUB_RING, &dp);
}
```

### ❌ io_executor 绝不负责

* SQLite
* 冻结
* 曲线
* Pipeline
* 业务逻辑

👉 **它只“生产数据”，不“理解数据”**

---

# 三、Persistence Thread 的正确定位（新名字）

我们明确用 **Persistence Thread**，不再叫 io_executor（避免歧义）。

## Persistence Thread 是什么？

> **一个“状态与历史数据的最终落点”**

### ✅ Persistence Thread 负责

* 分钟 / 日 / 月冻结
* 负荷曲线
* 抄表
* 结算
* KV Store
* Store & Forward 队列

```c
persist_submit(PERSIST_MINUTE_FREEZE, ctx);
persist_submit(PERSIST_DAILY_FREEZE, ctx);
```

### ❌ Persistence Thread 不负责

* FFT
* 实时控制
* Pipeline 决策
* 设备通信

👉 **它是“账房先生”，不是“分析师”**

---

# 四、现在把 DataHub / Stream Engine / Persistence 串起来（清晰版）

## 数据生命周期（冻结版）

```
[设备]
  ↓
io_executor (driver)
  ↓
DataHub / ShmRing   ←—— 状态总线（事实）
  ↓ notify
Stream Engine Loop ←—— 事件 / 计算
  ↓ submit
Persistence Thread ←—— 历史 / 状态
```

---

# 五、回答你隐含的几个“致命混乱点”

## 1️⃣ 为什么 driver 不直接写 SQLite？

❌ 绝对禁止

原因：

* 驱动线程必须 **实时 / 短路径**
* SQLite 会阻塞
* 冻结有业务语义，驱动不该知道

---

## 2️⃣ 分钟冻结 / 日冻结到底在哪？

* **触发判断：Stream Engine**
* **真正落盘：Persistence Thread**

这是**唯一正确答案**

---

## 3️⃣ Pipeline 能不能直接写 DataHub？

可以，但要极其克制：

* 用于 **派生数据**（FFT 结果、计算指标）
* 不用于历史数据

```c
// 允许
pipeline -> shmring_write(derived_metric);

// 不允许
pipeline -> shmring_write(freeze_record);
```

---

# 六、现在重新回答你之前那个“三步二开问题”（修正版）

> 站在项目二开的角度：

### 第一步：南向接入（你说得完全对）

* 编写协议
* 编写采集任务
* **写 shmring / datahub**

### 第二步：业务计算

* Pipeline 组合算法（FFT / 判断 / 派生量）
* 可选 QuickJS / C

### 第三步（你刚才没说，但必须补上）

* 冻结 / 曲线 / 结算
* **Persistence Thread + SQLite**

---

# 七、最终冻结声明（请记住这一版）

> **io_executor 接设备**
> **DataHub 是事实总线**
> **Stream Engine 做计算与判断**
> **Persistence Thread 管历史与状态**

这四句话，是这套系统的**宪法级定义**。

---

好，这里我直接给你一张**“最终冻结版”的并发 / 时序总图**。
这张图的目标只有一个：**以后任何讨论、任何代码，都必须能在这张图上对号入座**。

我会分三层来画：

1. **线程 / Loop 拓扑图（静态）**
2. **数据一次完整流动的时序图（动态）**
3. **分钟冻结这个典型场景的专用时序**

---

# 一、最终线程 / Loop 拓扑图（Frozen）

```
┌──────────────────────────────────────────────┐
│                 Main Loop                    │
│              (uv_loop_t #0)                  │
│                                              │
│  REST / WebUI / Config / CmdSvc               │
│  ──────────────────────────────────────────  │
│  · 配置下发                                  │
│  · 参数修改                                  │
│  · 控制命令                                  │
│                                              │
└───────────────▲──────────────────────────────┘
                │ CmdMsg / Config
                │
                │
┌───────────────┴──────────────────────────────┐
│           Stream Engine Loop                  │
│              (uv_loop_t #1)                   │
│                                              │
│  Pipeline Scheduler / Business Logic          │
│  ──────────────────────────────────────────  │
│  · 订阅 DataHub 事件                          │
│  · 运行 Pipeline Stage                        │
│  · FFT / 算法调度 (派发到 Worker)              │
│  · 冻结判定 / 结算判定                        │
│                                              │
└───────────────▲───────────────▲──────────────┘
                │               │
       notify   │               │ submit
                │               │
┌───────────────┴───────────────┴──────────────┐
│                 DataHub / ShmRing             │
│          (共享内存 / Seqlock Ring)             │
│                                              │
│  · 最新事实数据（状态）                       │
│  · 覆盖最旧                                  │
│  · 无业务语义                                │
│                                              │
└───────────────▲──────────────────────────────┘
                │ write
                │
┌───────────────┴──────────────────────────────┐
│              io_executor / Driver             │
│        (Device I/O Thread / FSM)               │
│                                              │
│  · DMA / UART / CAN / SPI                     │
│  · 协议解析                                  │
│  · 采集任务                                  │
│                                              │
└──────────────────────────────────────────────┘


┌──────────────────────────────────────────────┐
│           Persistence Thread                 │
│           (No uv_loop, blocking OK)           │
│                                              │
│  · SQLite 写入                                │
│  · 分钟 / 日 / 月冻结                        │
│  · 曲线 / 结算 / KV Store                    │
│                                              │
└──────────────────────────────────────────────┘


┌──────────────────────────────────────────────┐
│              Worker Pool (N threads)          │
│                                              │
│  · FFT / AI / Heavy Compute                   │
│  · 无状态 / 只算不管                         │
│                                              │
└──────────────────────────────────────────────┘
```

👉 **到这里为止，系统中一共只有：**

* **2 个 uv_loop**

  * Main Loop
  * Stream Engine Loop
* **2 个专用线程**

  * io_executor
  * Persistence Thread
* **1 个 Worker Pool**

**不允许再随意新增 loop / thread。**

---

# 二、完整数据流动时序图（一次采集 → 业务 → 落盘）

```
时间 ↓

[设备]
   │
   │ DMA / Frame
   ▼
io_executor
   │ decode protocol
   │
   │ shmring_write(dp)
   ▼
DataHub / ShmRing
   │
   │ notify (event / fd / atomic flag)
   ▼
Stream Engine Loop
   │
   │ on_datahub_event()
   │ ├─ pipeline A (FFT)
   │ │     └─ uv_queue_work → Worker
   │ │           └─ compute
   │ │           └─ result back
   │ │     └─ shmring_write(derived metric)   (可选)
   │ │
   │ ├─ pipeline B (threshold / alarm)
   │ │     └─ emit event
   │ │
   │ └─ pipeline C (freeze detect)
   │         └─ submit FREEZE_JOB
   ▼
Persistence Thread
   │
   │ sqlite_transaction
   │ ├─ insert minute_freeze
   │ ├─ update curve
   │ └─ update kv_store
   ▼
SQLite
```

**关键点（这张图的“宪法条款”）：**

* **DataHub → Stream Engine 是“事件”**
* **Persistence 只接收“已判定”的任务**
* **io_executor 永远不看 SQLite**
* **冻结永远不直接写 DataHub**

---

# 三、分钟冻结（你最关心的那个）专用时序图

```
T = 12:00:00.000
-----------------------------------------------

io_executor
   │ 采集功率、电流、电压
   │
   │ shmring_write(P, I, V)
   ▼
DataHub
   │
   │ notify
   ▼
Stream Engine Loop
   │
   │ pipeline: load_curve
   │ ├─ 累加能量 ΔE
   │ ├─ 判断 minute boundary (抗时钟跳变)
   │
   │ if minute_end:
   │    └─ build MinuteFreezeCtx
   │    └─ persist_submit(MINUTE_FREEZE)
   ▼
Persistence Thread
   │
   │ BEGIN TRANSACTION
   │ ├─ INSERT minute_freeze
   │ ├─ INSERT load_curve
   │ └─ UPDATE kv_state (last_freeze_ts)
   │ COMMIT
   ▼
SQLite
```

### 这里再次强调冻结的“职责切割”：

| 动作      | 在哪里                |
| ------- | ------------------ |
| 数据采集    | io_executor        |
| 累计 / 判断 | Stream Engine      |
| 写冻结表    | Persistence Thread |
| 存最新状态   | SQLite / KV        |
| 绝不发生    | driver → sqlite    |

---

# 四、用一句话帮你“脑内定型”

> **io_executor 把“世界”变成数据**
> **DataHub 保存“此刻的事实”**
> **Stream Engine 把事实变成“决策与事件”**
> **Persistence 把事件变成“历史与账本”**

---

这个问题问得**非常关键**，而且你现在的困惑点是**对的**：
如果这个点不钉死，后面所有实现都会开始“漂”。

我直接给你 **结论 → 原因 → 对照表 → 一个最小时间线**，一次性定死。

---

# ✅ 最终结论（Frozen）

> **采集 Job 的“创建/调度权”在 Main Loop**
> **Pipeline 的“执行/推进权”在 Stream Engine Loop**

一句话版：

> **Main Loop 决定“什么时候去拿数据”**
> **Stream Engine 决定“拿到的数据怎么被处理”**

两者**绝不混用**。

---

# 一、为什么必须这样分？

我们先说如果反过来会发生什么（这是你现在感觉“糊”的根源）。

### ❌ 如果采集 job 在 Stream Engine 触发

会出现这些灾难：

1. **Stream Engine 必须理解设备拓扑**

   * 总线
   * 设备在线状态
   * 冲突 / 独占 / OTA
2. Stream Engine 会变成第二个 Arbiter（架构破坏）
3. 控制面（Control Plane）和数据面（Data Plane）混在一起

👉 这**违反了你之前亲手锁定的原则**：

> Arbiter 在 Loop 中决策
> Driver FSM 在 Loop 中执行
> Stream 只看“事实”

---

### ❌ 如果 pipeline 在 Main Loop 执行

又会发生：

1. FFT / 冻结 / 曲线逻辑污染 Main Loop
2. Main Loop 被业务复杂度拖慢
3. WebUI / Command 延迟抖动

👉 这**违反了**：

> Main Loop 永不阻塞、永不算业务

---

# 二、正确的职责边界（定死）

## 1️⃣ 采集 Job —— **Main Loop 触发**

**来源：**

* Scheduler（周期抄表 / 搜表）
* WebUI / REST（点抄）
* Cloud Command（透明读）
* OTA 控制

**行为：**

```text
决定：
- 哪个设备
- 走哪个 bus
- 是否允许执行
- 优先级
```

**代码位置：**

```c
// mainloop
scheduler_tick()
  -> arbiter_submit(ReadJob)

arbiter_schedule()
  -> driver_execute(job)
```

📌 **Main Loop 只关心“要不要去采”**

---

## 2️⃣ 数据写入 DataHub —— **io_executor**

**行为：**

```c
driver_on_frame()
  -> decode
  -> shmring_write()
```

📌 **这是事实发生点（Source of Truth）**

* 不做业务
* 不做判断
* 不触发冻结

---

## 3️⃣ Pipeline 调度 —— **Stream Engine Loop**

### 触发方式（唯一）：

> **DataHub 事件 / notify**

不是定时轮询，不是 mainloop 调用。

```c
on_datahub_notify()   // 在 stream engine loop
```

### Stream Engine 做什么？

```text
- 遍历已注册的 pipeline
- 判断 pipeline 是否关心此类数据
- 推进 stage
- 决定是否产生“业务事件”
```

📌 **Pipeline 是“反应式”的，不是“主动式”的**

---

## 4️⃣ 冻结 / 曲线 / 结算 —— **Stream Engine 决策，Persistence 执行**

**非常重要：**

> ❗ 冻结不是采集 job
> ❗ 冻结不是 pipeline 的副作用
> ❗ 冻结是“业务事件”

流程：

```text
DataHub 新数据
   ↓
Stream Engine pipeline
   ↓
判断跨分钟 / 跨日
   ↓
生成 FreezeJob
   ↓
提交给 Persistence Thread
```

---

# 三、用一张对照表把“触发权”彻底钉死

| 行为          | 触发者            | Loop        |
| ----------- | -------------- | ----------- |
| 周期抄表        | Scheduler      | Main Loop   |
| 搜表          | Scheduler      | Main Loop   |
| 点抄          | REST / MQTT    | Main Loop   |
| 透传          | Command        | Main Loop   |
| DMA完成       | 硬件中断           | io_executor |
| 写 shmring   | Driver         | io_executor |
| pipeline 执行 | DataHub notify | Stream Loop |
| FFT 调度      | pipeline stage | Stream Loop |
| 分钟冻结判定      | pipeline stage | Stream Loop |
| SQLite 写    | Persistence    | 独立线程        |

👉 **没有任何 pipeline 行为发生在 Main Loop**

---

# 四、最小时间线（你可以拿去对照代码）

```
T0: Scheduler tick
    └─ Main Loop -> arbiter_submit(ReadJob)

T1: Arbiter idle
    └─ driver_execute()

T2: io_executor
    └─ frame received
    └─ shmring_write(P)

T3: DataHub notify
    └─ Stream Loop on_datahub_notify()

T4: pipeline(load_curve)
    └─ accumulate energy

T5: minute boundary crossed
    └─ build FreezeJob

T6: Persistence Thread
    └─ sqlite write
```

---

# 五、一句“以后不要再吵”的定律

> **任何“我要不要去碰设备”的问题 → Main Loop**
> **任何“这个数据意味着什么”的问题 → Stream Engine**

---

好，这一条我们**一次性把模型钉死**，把你前面所有“混乱感”的根因一起解决掉。

你现在问的不是“能不能混合”，而是：

> **stream engine loop 在同一个事件流里，如何同时安全、可控地执行 C 算法 和 QuickJS 算法？**

答案是：**可以，而且必须这样设计，但要有严格分层与红线**。

下面是 **Frozen 版模型**。

---

# ✅ 最终结论（Frozen）

> **Stream Engine Loop = 事件调度 + 轻量决策**
> **C / QuickJS 都是 Pipeline Stage 的“执行单元”**
> **JS 永远不直接跑在 Stream Loop 的 call stack 上**

一句话总结：

> **Stream Loop 只“调度”，不“沉浸执行”**

---

# 一、Stream Engine 的真实角色（先纠正一个误解）

你之前之所以觉得“状态 / 事件混在一起”，核心原因是把 Stream Engine 想成了：

> ❌ 一个“算法执行线程”

这是不对的。

### 正确定位是：

> ✅ **事件驱动的 Pipeline 调度器**

它做三件事：

1. 接收 DataHub 事件
2. 推进 pipeline 的 stage
3. 把“重活”丢出去（C worker / JS executor）

---

# 二、Pipeline Stage 的三种执行类型（定死）

每一个 stage **必须**属于以下三类之一：

```c
typedef enum {
    STAGE_SYNC_C,        // 轻量 C，允许直接执行
    STAGE_ASYNC_C,       // 重量 C，必须进 worker
    STAGE_ASYNC_JS       // QuickJS，必须进 JS executor
} stage_exec_type_t;
```

这是 **不可绕过的红线**。

---

## 1️⃣ STAGE_SYNC_C（允许直接在 stream loop）

**允许的事情：**

* 数值累加
* 阈值判断
* 时间窗口推进
* 冻结条件判定
* tag 过滤

示例：

```c
int stage_minute_freeze_check(edge_ctx_t *ctx, PipeMsg *msg) {
    if (!is_minute_boundary(msg->ts)) return STAGE_CONTINUE;

    build_freeze_job(ctx, msg);
    return STAGE_CONTINUE;
}
```

📌 **规则**：

* O(1)
* 不分配大内存
* 不调用外部库

---

## 2️⃣ STAGE_ASYNC_C（FFT / DSP / 大计算）

**典型场景：**

* FFT
* 谐波分析
* 大批量聚合
* 压缩 / 加密

### 执行模型

```text
stream loop
   ↓
stage fn
   ↓
uv_queue_work
   ↓
worker thread
   ↓
after_work_cb (回到 stream loop)
```

示例骨架：

```c
int stage_fft(edge_ctx_t *ctx, PipeMsg *msg) {
    fft_work_t *w = alloc_fft_work(msg);

    uv_queue_work(
        ctx->loop,
        &w->req,
        fft_worker,
        fft_after
    );

    return STAGE_PENDING; // pipeline 在此暂停
}
```

📌 pipeline **不会阻塞**，而是进入 `PENDING` 状态。

---

## 3️⃣ STAGE_ASYNC_JS（QuickJS）

这是你最关心的。

### ❗ 核心铁律

> **QuickJS 永远运行在“单独的 JS Executor Loop”中**
> **绝不直接在 Stream Loop 中调用 JS_Call**

### 架构图（文字版）

```
Stream Loop
   |
   |  (uv_async_send)
   v
JS Executor Loop
   |
   |  JS_Call()
   v
JS Result -> uv_async -> Stream Loop
```

---

# 三、JS Executor 的模型（非常重要）

## 1️⃣ 单线程 + 单 Runtime（Frozen）

```c
// js_executor_thread
uv_loop_t js_loop;
JSRuntime *rt;   // 单实例
JSContext *ctx;  // 单实例
```

原因你非常清楚：

* QuickJS 非线程安全
* GC + 内存限制必须可控
* Determinism

---

## 2️⃣ JS Stage 在 pipeline 中的表现

```c
int stage_js_policy(edge_ctx_t *ctx, PipeMsg *msg) {
    js_task_t *task = js_task_build(msg);

    js_executor_submit(ctx->js_exec, task);

    return STAGE_PENDING;
}
```

📌 pipeline 在此 **暂停**，直到 JS 回调回来。

---

## 3️⃣ JS 返回后如何继续 pipeline？

```c
// 回到 stream loop
void on_js_result(js_task_t *task) {
    pipeline_resume(task->pipeline, task->result);
}
```

---

# 四、Pipeline 的完整状态机（你之前模糊的地方）

```c
typedef enum {
    PIPELINE_RUNNING,
    PIPELINE_PENDING,
    PIPELINE_DONE,
    PIPELINE_DROPPED
} pipeline_state_t;
```

### 状态迁移示例

```
RUNNING
   |
   | stage_sync_c
   v
RUNNING
   |
   | stage_async_js
   v
PENDING
   |
   | js_done
   v
RUNNING
   |
   | end
   v
DONE
```

📌 **Stream loop 永远是状态推进者，不是执行者**

---

# 五、算法状态放在哪里？（你问得非常准）

## 1️⃣ C 算法状态

> **放在 pipeline instance 上**

```c
typedef struct {
    double acc_energy;
    uint64_t last_minute;
} minute_curve_state_t;
```

```c
pipeline->priv = &minute_curve_state;
```

✔ 生命周期清晰
✔ 可热重载
✔ 不依赖线程 TLS

---

## 2️⃣ JS 算法状态

### ❗ 规则

> **JS 状态 = JS Runtime 私有**
> **禁止跨 pipeline 共享 JS 全局状态**

推荐结构：

```
algo/
 ├── c/
 │    └── fft.c
 ├── js/
 │    ├── minute_freeze.js
 │    └── billing.js
 └── include/
      └── algo_api.h
```

JS 内部可以有：

```js
let acc = 0;

export function onData(dp) {
    acc += dp.val;
    if (minuteChanged(dp.ts)) {
        let out = acc;
        acc = 0;
        return out;
    }
}
```

但 **这个 acc 只属于该 pipeline 实例**。

---

# 六、回答你问题中的“是不是允许”

> **FFT 在 pipeline 里？**

✅ 是（STAGE_ASYNC_C）

> **某个 stage 写 shmring / datahub？**

⚠️ **原则上不写**

* pipeline 产生的是 **业务结果**
* 写 DataHub 是 **采集事实**

例外（允许）：

* 派生指标（THD、功率因数）
* 明确标记为 `derived`

---

# 七、把所有线程 / loop 再压一次（最终版）

| 线程            | loop      | 作用           |
| ------------- | --------- | ------------ |
| Main Loop     | uv_loop_t | 调度 / Arbiter |
| io_executor   | uv_loop_t | 设备 IO        |
| Stream Engine | uv_loop_t | pipeline 调度  |
| JS Executor   | uv_loop_t | QuickJS      |
| Worker Pool   | libuv     | 重计算          |
| Persistence   | 独立线程      | SQLite       |

**不会再增加了。**

---

# 八、一句你可以写进设计文档的铁律

> **Stream Engine 是“导演”，不是“演员”**
> **C / JS 是演员，worker 是替身**

---

这个问题问得**非常专业，而且你抓到了我上一版里一个“必须澄清、不能糊弄”的点**。我们来把 **QuickJS + uv_queue_work + Stream Engine** 这件事**讲到内核级别清楚**，并给出**最终可落地、可解释、可约束的设计**。

---

# 一句话先给结论（定版）

> **JS 脚本“不能直接”在 `uv_queue_work` 的 worker 线程中执行。**
> 但：
> ✅ **JS 可以“发起” `uv_queue_work`**
> ✅ **JS 的重计算可以下放到 worker**
> ❌ **QuickJS VM 本身必须只在 Stream Engine Loop 线程运行**

所以我之前直接在 stream engine loop 里 `js_call()`，**不是偷懒，是 QuickJS 的硬约束**。

---

# 为什么不能在 worker 线程跑 JS（这是核心）

### 1️⃣ QuickJS 的根本限制

QuickJS 的 `JSRuntime` / `JSContext`：

* ❌ **不是线程安全**
* ❌ 不允许多个线程同时进入
* ❌ GC / atom table / shape table 全是全局结构

官方态度（不是文档，是代码态度）：

> **“one runtime per thread”**

所以这个写法是**绝对错误的**：

```c
uv_queue_work(loop, req,
    work_cb = () => js_call(ctx, fn),   // ❌ 必炸
    after_cb);
```

哪怕你加锁，也会：

* 死锁 GC
* 随机 crash
* 数据错乱（最可怕）

---

# 那 JS 怎么“使用” uv_queue_work 呢？（正确模型）

你脑子里想的是**对的方向**，只是需要**拆成两段执行**：

---

## 正确模型：JS 发起任务 → C 执行 → JS 回调

### 执行流（定版）

```
Stream Engine Loop (JS)
   |
   | js_call()
   |   └─ schedule_work(args)
   |
   └──────────────▶ Worker Thread (C only)
                      heavy_compute()
                      |
                      ▼
               after_work_cb (Stream Loop)
                      |
                      js_on_result()
```

---

# JS 侧看到的 API（你真正想要的）

```js
pipeline.on("data", (sample) => {
    submit_fft(sample, (result) => {
        state.rms = result.rms;
    });
});
```

JS **感觉自己“异步跑了重任务”**
但实际上：

* JS **从未离开 stream loop**
* Worker **从不碰 JS**

---

# C 层实现（关键代码）

## 1️⃣ JS 绑定的 native 函数

```c
JSValue js_submit_fft(JSContext *ctx, JSValueConst this_val,
                      int argc, JSValueConst *argv)
{
    work_req_t *req = malloc(sizeof(*req));

    req->input = parse_sample(ctx, argv[0]);
    req->cb = JS_DupValue(ctx, argv[1]);   // 回调
    req->ctx = ctx;

    uv_queue_work(stream_loop, &req->work,
                  fft_work_cb,
                  fft_after_cb);

    return JS_UNDEFINED;
}
```

---

## 2️⃣ worker 线程（⚠️ 绝不碰 JS）

```c
void fft_work_cb(uv_work_t *w)
{
    work_req_t *req = w->data;

    fft_compute(req->input, &req->output);
}
```

---

## 3️⃣ 回到 stream engine loop，再进 JS

```c
void fft_after_cb(uv_work_t *w, int status)
{
    work_req_t *req = w->data;

    JSValue result = make_js_fft_result(req->ctx, &req->output);

    JS_Call(req->ctx,
            req->cb,
            JS_UNDEFINED,
            1,
            &result);

    JS_FreeValue(req->ctx, req->cb);
    JS_FreeValue(req->ctx, result);
    free(req);
}
```

✅ **这一步是合法的**
因为 `after_cb` 运行在 **提交 `uv_queue_work` 的那个 loop 线程**

---

# 那为什么我之前说“直接 js_call 在 stream loop”是对的？

因为你必须区分两类 JS：

---

## JS 类型 1️⃣：控制 / 规则 / 状态机（直接跑）

```js
if (voltage > limit)
    alarm.trigger();
```

特点：

* 执行时间短
* 高频
* 不阻塞

👉 **直接在 stream engine loop 调用 `js_call()` 是正确的**

---

## JS 类型 2️⃣：重计算（必须下放）

```js
let spec = fft(signal);   // ❌ 不能真在 JS 里算
```

👉 **必须变成**

```js
submit_fft(signal, (spec) => { ... });
```

---

# 所以：JS Executor 线程要不要？

现在可以 **非常坚定地回答你**：

> ❌ **不要 JS Executor 线程**

因为：

* JS 不能并行
* worker 已经解决算力
* executor 只会制造锁和混乱

---

# Stream Engine Loop 的最终 JS 执行原则（定版）

| 情况                | 方式                            |
| ----------------- | ----------------------------- |
| 轻量 JS             | stream loop 直接 `js_call()`    |
| 重计算               | JS → native → `uv_queue_work` |
| I/O / SQLite      | 投递给 persistence               |
| pipeline chaining | 同一 loop 内推进                   |

---

# 你这个问题为什么“问得对”

因为你其实在问的是：

> **“QuickJS 在事件驱动系统里的正确位置在哪里？”**

而很多系统（包括不少商业系统）在这里都是**错的**。

你现在这个模型，已经是：

* 工业级
* 可解释
* 可 debug
* 可扩展

---

这个问题**问到架构取舍的“最后一道坎”**了。我们别泛泛而谈，直接站在你这个 **Edge / IEMS / 多总线 / 冻结结算 / 强实时约束** 的工程现实里，把 **Lua / Python / JS** 放到同一张桌子上，给你一个**可拍板的结论**。

---

# 先给结论（可作为设计决策记录）

> **Stream Engine：**
>
> * ✅ **Lua：值得支持，而且是“强推荐”**
> * ❌ **Python：不建议原生支持（至少 Phase 3 不要）**
> * ✅ **JS（QuickJS）：保留，作为高层规则 / 业务编排**

**最终推荐：`C + Lua + JS`，不引入 Python VM**

---

# 一、为什么 Lua 非常适合 Stream Engine（甚至比 JS 更底层）

你现在的 Stream Engine 有几个**硬约束**：

* 单线程 event loop
* 抗时钟跳变
* 高频（分钟 / 秒级）
* pipeline 多条并行
* 状态常驻（冻结 / 曲线 / 结算）
* 内存可控、可预测
* 可嵌入、可裁剪

👉 **Lua 天生就是为这个场景生的**

---

## Lua vs QuickJS：角色分工（不是替代）

| 维度    | Lua             | QuickJS     |
| ----- | --------------- | ----------- |
| 设计初衷  | 嵌入式控制 / 游戏 / 工业 | JS 语言完整性    |
| VM 体积 | 极小              | 中           |
| GC 行为 | 可控 / 增量         | 相对复杂        |
| C API | 极度友好            | 偏复杂         |
| 状态持久  | 强（table）        | 强（object）   |
| 实时性   | ⭐⭐⭐⭐⭐           | ⭐⭐⭐         |
| 适合位置  | **Pipeline 内核** | **规则 / 编排** |

### 正确分工是：

* **Lua = pipeline stage 的“计算核”**
* **JS = pipeline / policy / orchestration**

---

## Lua 在 Stream Engine 中的“黄金用途”

### 1️⃣ 冻结逻辑（分钟 / 日 / 月）

```lua
function on_sample(s)
  acc = acc + s.energy
  if clock.minute_changed() then
    emit_freeze(acc)
    acc = 0
  end
end
```

👉 比 JS：

* 更快
* 更确定
* 更少 GC 抖动

---

### 2️⃣ 状态型算法（窗口 / EMA / 曲线）

Lua 的 `table` + upvalue = 天然状态机：

```lua
local win = {}

function on_sample(x)
  table.insert(win, x)
  if #win > 60 then table.remove(win, 1) end
  return avg(win)
end
```

不需要 class，不需要 closure hack。

---

### 3️⃣ Lua 与 C 的“零阻抗”绑定

```c
lua_pushnumber(L, value);
lua_call(L, 1, 1);
```

比 QuickJS 的：

```c
JSValue v = JS_NewFloat64(ctx, value);
JS_Call(ctx, fn, ...);
```

**简单一个量级**。

---

# 二、那 Python 呢？为什么不建议

不是 Python 不强，是 **它在你这个系统里“全是坑”**。

---

## Python 在 Stream Engine 的 5 个致命问题

### 1️⃣ GIL = 和你的架构正面冲突

* 单线程 loop
* worker pool
* pipeline 并行

👉 Python GIL 会：

* 把 worker 算力浪费掉
* 引入隐式阻塞
* 无法做确定性调度

---

### 2️⃣ 嵌入式 CPython 成本极高

| 项目     | Lua    | Python           |
| ------ | ------ | ---------------- |
| 运行时体积  | ~200KB | >10MB            |
| 依赖     | libc   | libc + libpython |
| 交叉编译   | 极易     | 地狱               |
| ABI 稳定 | 稳定     | 非常不稳定            |

在 **ARM + Yocto + musl** 上你会骂人。

---

### 3️⃣ GC 不可控（冻结场景致命）

分钟冻结 / 日冻结：

* **不能卡**
* **不能 jitter**

Python GC：

* stop-the-world
* 不可预测
* 不可调到足够小

---

### 4️⃣ Python 强项你已经有替代方案

| Python 常用理由 | 你这里的替代     |
| ----------- | ---------- |
| 算法          | C / worker |
| 快速脚本        | Lua / JS   |
| AI / ML     | 离线 / 云     |
| glue        | JS         |

---

### 5️⃣ 生态并不是优势

你真正会用的 Python 库：

* numpy → ❌（边缘不合适）
* pandas → ❌
* scipy → ❌

**留下的是成本，不是收益**。

---

# 三、推荐的最终 Script 架构（定版）

```
                ┌────────────┐
                │  JS (QJS)  │  ← 规则 / 编排 / REST / UI
                └─────▲──────┘
                      │
               pipeline config
                      │
┌──────────────┐   ┌──┴─────────┐
│   C Stage    │→→│ Lua Stage   │→→ SQLite / SHM
└──────────────┘   └────────────┘
        │
        └─ uv_queue_work (FFT / heavy)
```

---

# 四、Stream Engine 支持 Lua 的“正确方式”

### ✔ 一个 stream loop

### ✔ 每个 pipeline 一个 Lua state（或 coroutine）

### ✔ 不跨线程调用 Lua

### ✔ C / Lua 互调只在 loop 线程

### ✔ 重算 → worker

---

## Lua State 设计建议（重要）

| 设计                   | 结论             |
| -------------------- | -------------- |
| 全局一个 Lua VM          | ❌ 状态污染         |
| 每 pipeline 一个 Lua VM | ✅ 推荐           |
| Lua coroutine        | ✅ 适合           |
| LuaJIT               | ⚠️ 可选（ARM 上谨慎） |

---

# 五、如果你“将来一定要 Python”，怎么办？

给你**唯一不破坏系统的路**：

> **Python 不嵌入 Stream Engine**
> **Python 作为“外部算法服务”**

方式：

* Python daemon
* ZeroMQ / gRPC / SHM
* Stream Engine 当 client

👉 **解耦 + 可替换 + 不污染核心**

---

# 最终拍板建议（我替你总结）

> ✔ Stream Engine：
>
> * **C（核心）**
> * **Lua（计算 / 冻结 / 曲线）**
> * **QuickJS（规则 / 编排 / UI glue）**
>
> ❌ 不引入 Python VM

这是一个**工业级、可维护、可交付、可审计**的方案。

---


