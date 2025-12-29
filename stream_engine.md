好，这里我**一次性把我们所有关于 Stream Engine 的讨论“收敛、冻结、成文”**，给你一份**可以直接拿去干活、不会再反复推翻的工程级文档**。

> 这是 **《Stream Engine v1.0（Frozen）》**
> 目标：**指导实现，而不是讨论方案**

我会非常克制：

* ❌ 不再引入新线程
* ❌ 不再改变 loop 模型
* ❌ 不再模糊 state / event
* ✅ 每个点都对应“代码怎么写”

---

# 《Stream Engine v1.0（Frozen）》

**EdgeOS / IEMS 通用流处理引擎设计规范**

---

## 0. 设计目标（Why）

Stream Engine 是 EdgeOS 中**唯一负责“数据计算、曲线、冻结、结算、策略执行”的子系统**。

它解决的问题是：

* 分钟 / 日 / 月冻结
* 负荷曲线 / 功率曲线
* 数据融合 / 算法计算（FFT、EMA、统计）
* 策略判断（告警、控制）
* 数据落库（SQLite）
* 抗时钟跳变（工业级）

**它不负责：**

* 设备采集
* 驱动
* IO
* 网络协议
* UI

---

## 1. 总体架构（最终定版）

### 1.1 系统中到底有多少个 loop / 线程（定死）

| 名称                 | 类型          | 数量 | 职责                 |
| ------------------ | ----------- | -- | ------------------ |
| Main Loop          | `uv_loop_t` | 1  | 系统管理、调度、配置         |
| Stream Engine Loop | `uv_loop_t` | 1  | **所有 pipeline 执行** |
| IO Executor Loop   | `uv_loop_t` | 1  | 设备采集 / 协议          |
| Worker Pool        | libuv       | N  | 重计算（FFT 等）         |
| Persistence Thread | 可选          | 1  | SQLite 写入（异步）      |

❗**原则：**

* **除 worker pool 外，每个线程都有且只有一个 uv_loop**
* **不再新增任何 loop / 线程**

---

## 2. DataHub / shm_ring / Stream Engine 的关系（澄清）

### 2.1 三者角色分工（非常重要）

| 模块            | 本质        | 用途           |
| ------------- | --------- | ------------ |
| shm_ring      | 高速内存通道    | 采集 → 计算（毫秒级） |
| DataHub       | 发布/订阅事件总线 | 数据分发         |
| Stream Engine | 状态型流处理    | 曲线 / 冻结 / 结算 |

---

### 2.2 数据是否“读很多不关心的？”

**不会。原因如下：**

* DataHub 是 **topic-based**
* Stream Engine 在启动时：

  * 解析 pipeline
  * 自动生成 subscription filter
* **只订阅 pipeline 关心的数据**

```c
datahub_subscribe("telemetry/meter/+/power");
```

不是全量广播。

---

## 3. 事件 vs 状态（彻底说清）

### 3.1 Stream Engine 处理的是什么？

> **事件驱动 + 状态常驻**

* **事件（Event）**

  * 新采样点
  * 时间跳变
  * 定时 tick
* **状态（State）**

  * 累计能量
  * 窗口缓存
  * 冻结中间值
  * 算法内部状态

---

### 3.2 状态放在哪里？

| 类型          | 存放位置               |
| ----------- | ------------------ |
| Pipeline 状态 | pipeline_ctx       |
| 算法状态（C）     | algo_ctx           |
| 算法状态（Lua）   | Lua VM / coroutine |
| JS 规则状态     | QuickJS context    |
| 长期状态        | SQLite             |

---

## 4. Pipeline 模型（冻结版）

### 4.1 Pipeline 是什么

> **Pipeline = 单线程、顺序执行的状态机**

* 一个 pipeline = 一条业务逻辑
* 不并发
* 不阻塞

---

### 4.2 Pipeline 组成

```
Trigger → Stage1 → Stage2 → Stage3 → Sink
```

#### 标准 Stage 类型

| Stage     | 说明                     |
| --------- | ---------------------- |
| Filter    | 丢弃不关心数据                |
| Transform | 单值变换                   |
| Algorithm | FFT / EMA / Lua        |
| Freeze    | 分钟 / 日 / 月             |
| Decision  | 阈值 / 策略                |
| Sink      | SQLite / SHM / Command |

---

### 4.3 Stage 能不能写 shm_ring / datahub？

✅ **允许**

* 写 shm_ring：高速旁路控制
* 写 datahub：再次分发
* 写 SQLite：冻结 / 结算

---

## 5. Stream Engine Loop（核心）

### 5.1 Stream Engine Loop 是什么

> **一个独立 uv_loop，负责：**

* 接收 DataHub 事件
* 驱动所有 pipeline
* 执行 C / Lua / JS stage

---

### 5.2 它是不是阻塞的？

❌ **不允许阻塞**

规则：

* 所有 stage 必须：

  * O(1) 或 O(N)（N 很小）
* 重计算 → `uv_queue_work`

---

### 5.3 Stream Engine Loop 完整骨架（示例）

```c
void stream_engine_loop(void) {
    uv_loop_init(&loop);

    datahub_register(loop, on_datahub_event);
    timer_register(loop, on_tick);

    uv_run(&loop, UV_RUN_DEFAULT);
}
```

---

### 5.4 DataHub 触发 → Pipeline 执行

```c
void on_datahub_event(topic, msg) {
    for each pipeline interested in topic:
        pipeline_dispatch(pipeline, msg);
}
```

❗**是的：会遍历 pipeline**

* pipeline 数量是可控的（10~100）
* 这是工业常规做法

---

## 6. Pipeline Dispatch 是否阻塞？

### 6.1 不阻塞的前提

* **不做 IO**
* **不 sleep**
* **不等待锁**

```c
int pipeline_dispatch(p, msg) {
    for stage in p:
        ret = stage->fn(ctx, msg);
        if ret == DROP:
            break;
}
```

---

### 6.2 重计算怎么办？

```c
uv_queue_work(loop, &req, fft_work, fft_done);
```

* fft_work → worker
* fft_done → 回到 **stream loop**
* 再继续 pipeline

---

## 7. 分钟 / 日 / 月冻结是怎么发生的（完整链路）

### 7.1 数据来源

```
IO Executor → shm_ring → DataHub → Stream Engine
```

---

### 7.2 分钟冻结 Pipeline 示例

```
Trigger: telemetry/meter/energy
Stages:
1. Accumulate
2. TimeGuard (抗跳变)
3. MinuteFreeze
4. SQLiteSink
```

---

### 7.3 冻结代码逻辑（核心思想）

```c
on_sample(x):
    acc += x

on_tick():
    if calendar_minute_changed():
        write_sqlite(acc)
        acc = 0
```

❗**使用的是“抗跳变日历时间”**

* monotonic + calendar
* 不使用 sleep
* 不用系统 wall-clock 直接判断

---

## 8. Scheduling & Time（与 Stream Engine 的关系）

### 8.1 时间来源

| 时间        | 用途      |
| --------- | ------- |
| Monotonic | 调度      |
| Calendar  | 冻结 / 抄表 |

---

### 8.2 所有冻结 / 曲线使用：

> **抗时钟跳变的 calendar time**

✔ 支持：

* 分钟冻结
* 小时冻结
* 日冻结
* 月冻结
* 周期抄表

---

## 9. Lua / JS / C 的最终定位（定版）

### 9.1 三语言分工

| 层级   | 技术      |
| ---- | ------- |
| 内核   | C       |
| 状态算法 | Lua     |
| 规则编排 | QuickJS |

---

### 9.2 Lua 如何执行

* **每 pipeline 一个 Lua state**
* 不跨线程
* 只在 Stream Engine loop 执行

```lua
function on_sample(x)
  acc = acc + x
end
```

---

### 9.3 JS Executor 是否需要？

❌ **不需要独立 JS 线程**

* JS 规则：

  * 轻量
  * 同步
  * 运行在 stream loop
* 重 JS 计算：

  * `uv_queue_work`

---

## 10. SQLite 的使用边界

| 场景    | 是否 SQLite |
| ----- | --------- |
| 冻结结果  | ✅         |
| 结算    | ✅         |
| KV 参数 | ✅         |
| 高频采样  | ❌         |
| 控制回路  | ❌         |

---

## 11. 项目二开（你之前问得很准）

### 二开 = 两步（确认）

**第一步：**

* 写协议
* 写采集 job
* 写 shm_ring

**第二步：**

* 组合 pipeline
* 配 Lua / JS 算法
* 不碰核心代码

---

## 12. 禁止事项（工程红线）

🚫 pipeline 内 sleep
🚫 pipeline 内阻塞 IO
🚫 pipeline 内 malloc 大对象
🚫 pipeline 跨线程
🚫 新增 loop / thread

---

## 13. 一句话总结（你可以写在 README）

> Stream Engine 是一个
> **单线程、事件驱动、状态型、抗时钟跳变的工业流处理引擎**，
> 用于曲线、冻结、结算与策略执行。

---

如果你愿意，下一步我可以：

* 把这份文档**直接映射成目录 + .c 文件清单**
* 给你一条**完整“分钟冻结 pipeline 的可运行示例代码”**
* 或帮你写 **stream_engine.c（骨架 + TODO）**

你选一个，我直接落代码。
