# Core Contract（核心契约）

> 目标：
>
> * **驯服“千奇百怪的设备 / 接口 / 协议 / 数据点”**
> * 用一套 *稳定、可演进、可落地* 的 Core Contract 作为 **设备 → 系统核心** 的唯一边界
> * 任何新设备、新协议、新采集方式，只要满足本契约，就能接入
>
> 本文档是 **直接干活用的工程规范**，不是概念说明书。

---

## 0. 核心思想（一句话版）

> **设备世界无限复杂，Core Contract 必须极度克制。**
>
> 👉 所有复杂性，止步于 Driver / Adapter；
> 👉 Core 只接受 *统一结构化事实*。

---

## 1. Core Contract 在整体架构中的位置

```
[ Device / Sensor / PLC / MCU ]
        |
        |  (物理接口 + 协议)
        v
+-----------------------------+
| Driver / Adapter Layer      |
|  - 串口 / TCP / CAN / SPI   |
|  - Modbus / OPC-UA / 私有协议|
+-----------------------------+
        |
        |  ✅ Core Contract
        v
+-----------------------------+
| DataHub (shmring)           |
| Stream Engine / Pipeline    |
| Scheduler / Freeze / Algo   |
+-----------------------------+
```

📌 **Core Contract 是唯一允许进入 Core 的“入口格式”**。

---

## 2. Core Contract 的职责边界（非常重要）

### 2.1 Core Contract *负责什么*

* 统一 **数据点模型（Data Point Model）**
* 统一 **时间语义（Time Semantics）**
* 统一 **质量与状态表达（Quality / Status）**
* 统一 **事件 vs 状态的边界**
* 统一 **控制与采集的分离**

### 2.2 Core Contract *坚决不负责什么*

❌ 不关心物理接口（RS485 / CAN / Ethernet）
❌ 不关心协议（Modbus / 私有帧）
❌ 不关心寄存器地址 / bit 位
❌ 不关心设备厂商 / 型号
❌ 不关心采集实现是中断、轮询还是 DMA

👉 **这些都属于 Driver 私域。**

---

## 3. 数据点（Data Point）统一模型

### 3.1 数据点是 Core 的最小事实单元

> Core 眼里只有“数据点变化事实”，没有“寄存器”。

### 3.2 Data Point 结构定义

```c
struct core_datapoint {
    uint32_t    tag_id;        // 全局唯一 ID
    uint64_t    ts_ns;         // 采集时间（单调 or 校准后）
    int64_t     value_i64;     // 整型值（统一承载）
    double      value_f64;     // 浮点值（可选）
    uint32_t    flags;         // 质量 / 状态 / 语义标志
};
```

### 3.3 设计原则

* **tag_id 是唯一身份，不是名字**
* **value 统一数值化**（字符串 = Driver 内部消化）
* **时间戳必须在 Driver 层确定**

---

## 4. Tag ID 规则（生死线）

### 4.1 Tag ID 是系统“神经编号”

* 全系统唯一
* 永不复用
* 永不携带业务语义

### 4.2 Tag ID 的来源

```text
[设备建模 / 工程配置阶段]
        ↓
生成 tag_id
        ↓
固化到 Driver + Pipeline + UI
```

📌 **运行期不允许动态生成 tag_id**。

---

## 5. 时间语义（Time Semantics）

### 5.1 Core 只接受三种时间

| 类型           | 含义       | 说明   |
| ------------ | -------- | ---- |
| Event Time   | 设备真实发生时间 | 首选   |
| Acquire Time | 实际采集时间   | 允许   |
| System Time  | 系统当前时间   | 最后兜底 |

### 5.2 决策点

* 时间戳 **必须在 Driver 层确定**
* Core 不推测、不修正

---

## 6. 数据质量与状态（Flags）

### 6.1 Flags 位定义（示例）

```c
#define CORE_FLAG_VALID        (1 << 0)
#define CORE_FLAG_OVERRANGE    (1 << 1)
#define CORE_FLAG_COMM_LOST    (1 << 2)
#define CORE_FLAG_SUBSTITUTED  (1 << 3)
#define CORE_FLAG_MANUAL       (1 << 4)
```

### 6.2 原则

* **质量不是算法算出来的，是采集事实**
* Pipeline / 冻结只“使用”，不“创造”质量

---

## 7. 状态流 vs 事件流（关键分界）

### 7.1 状态（State）

* 当前值
* 可覆盖
* 进入 DataHub（shmring）

示例：

* 温度当前值
* 开关当前状态

### 7.2 事件（Event）

* 一次性发生
* 不覆盖
* 只进入 Stream Engine

示例：

* 报警
* 跳变
* 故障

📌 **Driver 必须在上送前完成区分**。

---

## 8. Driver → Core 的唯一接口

### 8.1 写入 DataHub（状态）

```c
core_publish_state(struct core_datapoint *dp);
```

### 8.2 推送事件（事件）

```c
core_emit_event(struct core_datapoint *dp);
```

❗ Driver **不得**：

* 直接操作 Pipeline
* 直接写 SQLite

---

## 9. 千奇百怪协议的处理准则

### 9.1 协议适配的止损线

| 层级            | 允许复杂度 |
| ------------- | ----- |
| Driver        | 无上限   |
| Core Contract | 零复杂度  |

### 9.2 必须在 Driver 层解决的问题

* bit → 数值
* 多寄存器拼接
* 字节序
* 校验 / CRC
* 异常帧

Core **一律不知情**。

---

## 10. 控制类数据的契约（反向）

### 10.1 控制不是数据点

* 控制是 **命令**
* 数据点是 **事实**

### 10.2 Core 控制接口

```c
struct core_command {
    uint32_t target_id;
    uint32_t cmd_id;
    int64_t  param;
};
```

```c
core_send_command(struct core_command *cmd);
```

📌 控制流：

```
Web / Scheduler
   ↓
Main Loop / Arbiter
   ↓
Driver FSM
```

永不走 Pipeline。

---

## 11. 与 Stream Engine / Pipeline 的关系

* Core Contract 是 **Pipeline 的唯一输入格式**
* Pipeline 永远假设：

  * tag_id 正确
  * 时间可信
  * 质量已判定

Pipeline **只做：**

* 组合
* 计算
* 冻结
* 转发

---

## 12. 工程落地检查清单（照着干）

### 新接一个设备，必须回答：

* [ ] 设备有哪些“状态型数据点”？
* [ ] 哪些是“事件型数据点”？
* [ ] tag_id 是否已分配并登记？
* [ ] 时间戳在 Driver 哪里产生？
* [ ] 异常/失联如何体现在 flags？

### 写 Driver 前必须通过：

* [ ] 不向 Core 泄露寄存器概念
* [ ] 不在 Core 出现字符串解析
* [ ] 不在 Pipeline 判断通信质量

---

## 13. 最终原则（请背下来）

> **让 Driver 变脏，让 Core 变干净。**

> **所有不可避免的混乱，都必须在 Core Contract 之前结束。**

---


好，这一版 **core_contract.md** 已经是**“能直接照着干活”的工程级规范**了，而且我刻意做了几件事来避免你之前反复指出的那些坑：

### ✅ 这版文档已经做到的几件关键事

1. **彻底封死复杂性外泄**

   * 物理接口 / 协议 / 寄存器 / bit 位 **全部止损在 Driver**
   * Core / Pipeline / Stream Engine **永远看不到这些概念**

2. **状态 vs 事件 一刀切清楚**

   * 状态 → DataHub（shmring）
   * 事件 → Stream Engine
   * Driver 层必须判定，Core 不兜底

3. **时间语义可直接支撑**

   * 分钟曲线
   * 日冻结 / 月冻结
   * 结算 / 追溯
     👉 前提是 Driver 给出 *抗时钟跳变的日历时间*

4. **为二开留好路径**

   * 二开者只需要：

     1. 写 Driver，把“脏协议”翻译成 `core_datapoint`
     2. 用 Pipeline / Stream Engine 组合算法
   * 不需要理解 Core 内部实现

5. **契约是“唯一入口”**

   * Driver **只能**调用：

     * `core_publish_state`
     * `core_emit_event`
   * 禁止直连 Pipeline / SQLite / Scheduler

---

### 🧠 和你前面所有讨论的内容是“完全对齐”的

* ✔ DataHub 不再是“什么都往里扔”
* ✔ Pipeline 不背锅（不判断质量、不判时间）
* ✔ 冻结 / 曲线 / 结算 → Pipeline / Stream Engine → SQLite
* ✔ Core Contract ≠ EdgeX 的全家桶，但**吸收了它的精华**

---

这是个**非常关键、而且你问到“刀口上”的问题**。
一句话先给结论：

> **tag_id ≠ 协议 ID（OBIS / Modbus 地址 / CAN ID）**
> **tag_id 是“协议无关的、系统内部的语义 ID”**

下面我把这件事**彻底讲清楚**，并且给你一个**可直接照着实现的映射模型**。

---

## 一、三层 ID 必须严格区分（这是铁律）

### ① 协议 ID（Protocol ID）

这是**物理世界的残酷现实**：

| 协议     | 示例                             |
| ------ | ------------------------------ |
| DLMS   | OBIS: `1-0:1.8.0`              |
| Modbus | `(slave=1, fc=3, addr=0x1000)` |
| IEC101 | ASDU / IOA                     |
| CAN    | CAN ID + signal bit            |
| 自定义    | 随便你                            |

特点：

* 和协议 **强绑定**
* 不可避免千奇百怪
* **只能活在 Driver 层**

👉 **Core / Pipeline 永远不应该看到它**

---

### ② tag_id（Core Contract 层）

这是你系统的**“世界观 ID”**。

```c
typedef uint32_t tag_id_t;
```

例如：

| tag_id | 语义       |
| ------ | -------- |
| 1001   | 三相A电压    |
| 1002   | 三相B电压    |
| 2001   | 有功功率     |
| 3001   | 累计电能（正向） |
| 9001   | 设备运行状态   |

特点：

* **协议无关**
* **设备无关**
* 稳定、可冻结（写进 contract）
* 是 Pipeline / 冻结 / 算法 / UI 的共同语言

👉 **tag_id 是 Core 世界的“通用语言”**

---

### ③ 数据实例 ID（Instance / Source）

同一个 tag，在不同设备 / 不同通道上会有多个实例。

```c
typedef struct {
    device_id_t device;
    channel_id_t channel;
    tag_id_t tag;
} datapoint_key_t;
```

例如：

* `Meter01 / L1 / Voltage`
* `Meter02 / L1 / Voltage`

👉 **tag_id 只表达“是什么”，不表达“来自哪”**

---

## 二、DLMS OBIS 和 tag_id 的真实关系

### ❌ 错误理解（很多系统都这么做）

```text
tag_id = hash("1-0:1.8.0")
```

问题：

* tag_id 不可读
* 语义丢失
* 不可治理（冻结、计量、审计都完蛋）

---

### ✅ 正确模型：Driver 内部映射表

**DLMS Driver 内部维护映射关系**

```c
typedef struct {
    const char *obis;
    tag_id_t tag;
    data_type_t type;
    scale_t scale;
} dlms_map_t;
```

```c
static const dlms_map_t dlms_map[] = {
    { "1-0:32.7.0", TAG_VOLTAGE_L1, TYPE_FLOAT, 0.1 },
    { "1-0:52.7.0", TAG_VOLTAGE_L2, TYPE_FLOAT, 0.1 },
    { "1-0:72.7.0", TAG_VOLTAGE_L3, TYPE_FLOAT, 0.1 },
    { "1-0:1.8.0", TAG_ENERGY_IMPORT, TYPE_UINT64, 1.0 },
};
```

Driver 工作流程：

```text
OBIS → Driver decode → tag_id → core_publish_state()
```

👉 **OBIS 在 Driver 中“寿终正寝”**

---

## 三、为什么必须做这一层 tag 抽象？

### 1️⃣ 支撑“同构业务、异构协议”

同一个 tag：

| 设备  | 协议     | 协议 ID     |
| --- | ------ | --------- |
| 电表A | DLMS   | 1-0:1.8.0 |
| 电表B | Modbus | 0x1000    |
| 电表C | IEC101 | IOA=123   |

但在 Core 看来：

```c
tag_id = TAG_ENERGY_IMPORT
```

👉 **冻结 / 结算 / 算法完全复用**

---

### 2️⃣ 支撑算法与 Pipeline 的可移植性

```c
// 算法只认 tag_id
if (msg->tag == TAG_ACTIVE_POWER) {
    calc_fft(msg);
}
```

如果 tag_id 绑定协议：

* 算法 = 不可复用垃圾
* Pipeline = 协议泥潭

---

### 3️⃣ 支撑 Web UI / REST 的长期演进

UI 不可能懂 OBIS：

```json
{
  "tag": "Energy.Import.Total",
  "unit": "kWh",
  "value": 12345.6
}
```

tag_id → tag_name → UI Label
这是**工业级系统必须具备的稳定契约**

---

## 四、tag_id 从哪来？谁来定义？

### ✅ tag_id 的来源必须是**Core Contract**

建议你这样分层：

```
contracts/
 ├── tag_catalog.yaml   # 全系统 tag 权威表
 ├── unit.yaml
 └── data_type.yaml
```

```yaml
- tag_id: 3001
  name: Energy.Import.Total
  type: uint64
  unit: kWh
  semantic: cumulative
  billing: true
```

然后：

* C 头文件自动生成
* Driver / Pipeline / UI 共用

👉 **tag_id 一旦发布，原则上不改**

---

## 五、tag_id ≠ 点表（Point Table）

你可能会问：

> 这不就是“点表”吗？

**不是。**

| 项        | 传统点表 | tag_id |
| -------- | ---- | ------ |
| 是否绑定协议   | 是    | 否      |
| 是否设备相关   | 是    | 否      |
| 是否可跨产品   | 否    | 是      |
| 是否支撑算法复用 | ❌    | ✅      |

tag_id 是**“业务语义原子”**，不是“采集点”。

---

## 六、用一句工程化的话总结

> **协议 ID 是现实世界的噪声**
> **tag_id 是你系统的语言、法律和货币**

可以，而且**这是一个非常高级、非常正确的方向** 👍
结论先行：

> **完全可以，而且非常值得**
> 👉 用 **HCS / DTS 这类“声明式硬件描述语言”在编译期生成 tag / 协议映射 / 能力宏**
> 👉 你的这个项目，比传统 Linux 驱动 **更适合** 这种做法

下面我分层讲清楚：**能不能 → 怎么用 → 用到哪一层 → 怎么不踩坑**。

---

## 一、为什么 HCS / DTS 思路「刚好命中你的问题」

你现在面对的核心矛盾是：

* 协议：DLMS / Modbus / HPLC / IEC / 私有
* 设备：千奇百怪
* 业务：tag_id 必须稳定
* Driver：必须知道“这个 OBIS / 地址 对应哪个 tag”

👉 **这是典型的「结构化、静态、可编译期确定」的信息**

这正是：

* **DTS**
* **HCS（鸿蒙 Hardware Configuration Script）**
* **hc_gen / dtc**

最擅长解决的场景。

---

## 二、这三类技术本质上是一回事

| 技术           | 本质                     |
| ------------ | ---------------------- |
| DTS / dtc    | Linux 世界的「硬件语义描述」      |
| HCS / hc_gen | 鸿蒙的「设备能力 & 参数 DSL」     |
| 你的需求         | 「设备 + 协议 + tag 映射 DSL」 |

👉 **你不是“借鉴”，而是“同一问题的另一实例”**

---

## 三、在你这个系统中，它应该放在哪一层？

### 正确答案（非常重要）

> **只用于：Driver → Core 的“协议 → tag_id 映射”**
>
> ❌ 不进入 Pipeline
> ❌ 不进入 Algorithm
> ❌ 不进入 UI

### 架构定位图

```
[DTS / HCS]
     ↓ 编译期生成
[driver_xxx_map.h / .c]
     ↓ runtime
[IO Executor / Driver]
     ↓ tag_id
[DataHub / shm_ring]
     ↓
[Pipeline / Algorithm / SQLite]
```

---

## 四、一个「完全可落地」的设计方案

### 方案 A：用 DTS（最接近 Linux 工程）

#### 1️⃣ 定义你自己的 binding（关键）

```dts
meter@0 {
    compatible = "edgeos,dlms-meter";

    channel@1 {
        obis = "1-0:1.8.0";
        tag-id = <3001>;
        type = "uint64";
        scale = <1>;
    };

    channel@2 {
        obis = "1-0:32.7.0";
        tag-id = <1001>;
        type = "float";
        scale = <10>;
    };
};
```

👉 这比 XML / JSON **强太多**：

* 有层级
* 有类型
* 有校验
* 有 binding 规范

---

#### 2️⃣ 用 dtc 编译成中间产物

```bash
dtc -I dts -O dtb meter.dts -o meter.dtb
```

或者直接：

```bash
dtc -I dts -O dts meter.dts > meter_flat.dtsi
```

---

#### 3️⃣ 生成 C 宏（关键步骤）

你可以写一个 **dtb → C 的生成器**（很多项目都这么干）：

```c
// auto-generated
#define DLMS_OBIS_1_0_1_8_0   3001
#define DLMS_OBIS_1_0_32_7_0  1001
```

或者更工业一点：

```c
typedef struct {
    const char *obis;
    tag_id_t tag;
    data_type_t type;
    int scale;
} dlms_map_t;

extern const dlms_map_t dlms_map[];
extern const size_t dlms_map_count;
```

---

### 方案 B：用 HCS（如果你偏鸿蒙风格）

HCS 非常适合你这种 **“参数 + 能力 + 资源”** 的系统。

```hcs
device dlms_meter {
    channel {
        obis = "1-0:1.8.0";
        tag_id = 3001;
        unit = "kWh";
    }
    channel {
        obis = "1-0:32.7.0";
        tag_id = 1001;
        unit = "V";
    }
}
```

`hc_gen` 输出：

```c
#define HCS_TAG_ENERGY_IMPORT 3001
#define HCS_TAG_VOLTAGE_L1   1001
```

👉 **非常适合你未来做“产品型号差异化”**

---

## 五、你这个系统里「最漂亮」的用法

### Driver 代码会变成这样（非常干净）

```c
static const dlms_map_t *map = dlms_get_map();

tag_id_t tag = map_find_by_obis(map, "1-0:1.8.0");

core_publish(tag, value);
```

**Driver 不再有 if/else / switch / 魔法字符串**。

---

## 六、为什么“必须是编译期”，而不是 JSON？

这是你这个系统的关键点之一：

| 项    | JSON  | DTS / HCS |
| ---- | ----- | --------- |
| 校验   | 运行时   | 编译期       |
| 发现错误 | 晚     | 早         |
| 性能   | 查表/解析 | 直接指针      |
| 安全   | 可被篡改  | 固化        |
| 工业感  | ❌     | ✅         |

👉 **计量 / 冻结 / 结算系统，必须“越静态越好”**

---

## 七、需要明确的边界（不然会翻车）

### ❌ 不要做的事

* ❌ 用 DTS 定义算法
* ❌ 用 DTS 定义 pipeline
* ❌ 用 DTS 定义 UI 行为

### ✅ 正确边界

| 内容          | 是否适合 |
| ----------- | ---- |
| 协议 → tag 映射 | ✅    |
| 协议参数（波特率等）  | ✅    |
| 设备能力声明      | ✅    |
| 业务逻辑        | ❌    |

---

## 八、如果你让我“拍板”的推荐

> **Phase 3 正式方案建议：**

1. **协议映射：DTS / HCS 编译期生成**
2. **业务编排：JSON / YAML（Pipeline）**
3. **状态与数据：tag_id + shm_ring**
4. **算法：C / JS / Lua**

这是一个**非常工业、非常长期可维护的组合**。

---

很好，这三件事**正好可以一次性打成一个“工业级闭环方案”**。
我下面不是“示例”，而是**你可以直接照着落地的工程模板**。

我按 **① → ② → ③** 给你，最后再给一个**总装配图**，保证以后不乱。

---

# ① edgeos-meter.dts + binding + 生成 C 代码模板（可直接落地）

## 1. DTS 设计目标（先定原则）

**DTS 只干三件事：**

1. 协议对象（OBIS / Modbus Addr / PointID）
2. 与 **tag_id 的静态绑定**
3. 数据语义（类型 / 单位 / scale）

**DTS 绝不包含：**

* 算法
* Pipeline
* UI
* 业务逻辑

---

## 2. binding 规范（edgeos,dlms-meter.yaml）

```yaml
# bindings/edgeos,dlms-meter.yaml
title: EdgeOS DLMS Meter Binding

properties:
  compatible:
    const: edgeos,dlms-meter

patternProperties:
  "^channel@[0-9a-f]+$":
    type: object
    required: [obis, tag-id, data-type]
    properties:
      obis:
        description: DLMS OBIS code
        type: string
      tag-id:
        description: Stable EdgeOS tag identifier
        type: integer
      data-type:
        enum: [int32, uint32, int64, uint64, float, double]
      scale:
        type: integer
        default: 1
      unit:
        type: string
```

👉 **这一步的价值**：

* `dtc` 就能帮你检查配置是否“物理 + 语义正确”

---

## 3. edgeos-meter.dts（正式版本）

```dts
/dts-v1/;

meter@0 {
    compatible = "edgeos,dlms-meter";
    model = "SG-EL2025-A";

    channel@1 {
        obis = "1-0:1.8.0";
        tag-id = <3001>;        // 正向有功总电能
        data-type = "uint64";
        scale = <1>;
        unit = "kWh";
    };

    channel@2 {
        obis = "1-0:32.7.0";
        tag-id = <1001>;        // A相电压
        data-type = "float";
        scale = <10>;
        unit = "V";
    };
};
```

---

## 4. 生成 C 代码（核心）

### 生成目标

```c
// auto-generated: dlms_meter_map.h
#pragma once

#include <stdint.h>

typedef enum {
    TAG_VOLTAGE_L1 = 1001,
    TAG_ENERGY_IMPORT = 3001,
} tag_id_t;

typedef enum {
    DT_UINT64,
    DT_FLOAT,
} data_type_t;

typedef struct {
    const char *obis;
    tag_id_t tag;
    data_type_t type;
    int scale;
} dlms_channel_map_t;

extern const dlms_channel_map_t g_dlms_channels[];
extern const size_t g_dlms_channel_count;
```

```c
// auto-generated: dlms_meter_map.c
#include "dlms_meter_map.h"

const dlms_channel_map_t g_dlms_channels[] = {
    { "1-0:1.8.0", TAG_ENERGY_IMPORT, DT_UINT64, 1 },
    { "1-0:32.7.0", TAG_VOLTAGE_L1, DT_FLOAT, 10 },
};

const size_t g_dlms_channel_count =
    sizeof(g_dlms_channels) / sizeof(g_dlms_channels[0]);
```

---

## 5. Driver 使用方式（非常干净）

```c
void dlms_on_value(const char *obis, double raw)
{
    const dlms_channel_map_t *m = find_map_by_obis(obis);
    if (!m) return;

    double val = raw / m->scale;

    shmring_write(m->tag, &val, sizeof(val));
}
```

**Driver 再也不关心业务，只负责“协议 → tag”**。

---

# ② HCS 风格的「多产品型号差异化方案」

这一部分是**给你产品线用的**。

---

## 1. 设计目标

* **同一套代码**
* 不同产品：

  * 协议点不同
  * tag 覆盖不同
  * 能力不同

👉 **完全符合 HCS 的设计哲学**

---

## 2. HCS 结构示例

```hcs
root {
    product SG_EL2025_A {
        include "dlms_common.hcs";
        include "meter_basic.hcs";
    }

    product SG_EL2025_PRO {
        include "dlms_common.hcs";
        include "meter_basic.hcs";
        include "meter_harmonics.hcs";
    }
}
```

---

### meter_basic.hcs

```hcs
channel energy_import {
    obis = "1-0:1.8.0";
    tag_id = 3001;
    type = "uint64";
}

channel voltage_l1 {
    obis = "1-0:32.7.0";
    tag_id = 1001;
    type = "float";
}
```

### meter_harmonics.hcs（Pro 才有）

```hcs
channel harmonic_3 {
    obis = "1-0:13.7.0";
    tag_id = 5103;
    type = "float";
}
```

---

## 3. 编译产物差异

```bash
hc_gen --product SG_EL2025_A   -> dlms_map.c
hc_gen --product SG_EL2025_PRO -> dlms_map.c
```

**C 代码不变，能力自然变化**。

---

# ③ tag_catalog.yaml ↔ DTS 打通（一源多用）

这是**系统级能力**，非常关键。

---

## 1. tag_catalog.yaml（单一事实源）

```yaml
tags:
  - id: 1001
    name: voltage_l1
    unit: V
    type: float
    category: telemetry

  - id: 3001
    name: energy_import
    unit: kWh
    type: uint64
    category: billing
```

---

## 2. 自动生成 DTS 片段

```dts
channel@1001 {
    tag-id = <1001>;
    data-type = "float";
    unit = "V";
};

channel@3001 {
    tag-id = <3001>;
    data-type = "uint64";
    unit = "kWh";
};
```

再由 **人工 / 工具补充协议字段（OBIS / addr）**。

---

## 3. 同一份 catalog 还能生成什么？

| 产物           | 用途              |
| ------------ | --------------- |
| tag_ids.h    | C 编译期常量         |
| openapi.yaml | REST API Schema |
| ui_meta.json | Web UI 展示       |
| dts/hcs      | Driver 静态映射     |

👉 **这一步直接把系统“拉齐”**

---

# ④ 总装配图（最终拍板版本）

```
          tag_catalog.yaml
                  |
      +-----------+-----------+
      |                       |
   DTS / HCS             OpenAPI / UI
      |
  dtc / hc_gen
      |
  auto-gen C map
      |
   Driver / IO Executor
      |
   shm_ring / DataHub
      |
  Stream Engine / Pipeline
      |
   SQLite / MQTT / Control
```

---


**可以这样写，但不能“直接这样用”**。
如果你真的按你这个写法上工程，**三个月后一定会炸**。
我给你一个**可行 + 可维护 + 可编译期可校验**的做法。

下面我分 **结论 → 正确姿势 → 示例 → 为什么不能混写 → 最终推荐模板** 五步。

---

## 一句话结论（先给答案）

> **可以在一个 channel 中“描述多协议映射”，
> 但必须做到：**
>
> **① 同一时刻只激活一个协议**
> **② 协议字段必须结构化 + 可校验**
> **③ 生成 C 代码时必须是“已裁剪态”**

---

## 你写的这个问题在哪里（很关键）

```dts
channel {
    obis = "1-0:1.8.0";
    can_id = ;
    modbus_addr=;
    tag_id = 3001;
    unit = "kWh";
}
```

### ❌ 三个致命问题

1. **语义冲突**

   * OBIS 是 DLMS
   * modbus_addr 是 Modbus
   * can_id 是 CAN
     👉 一个 channel **不可能同时属于三个协议**

2. **无法做 binding 校验**

   * dtc / hcs 无法判断：

     * 哪个协议生效？
     * 哪些字段是必填？
     * 哪些是非法组合？

3. **生成 C 代码必然污染**

   * Driver 会看到一堆空字段
   * 二开的人不知道该用哪个

---

## 正确设计原则（拍板）

### ✅ 原则 1：**tag 是协议无关的**

```text
tag_id = 业务语义唯一键
```

### ✅ 原则 2：**协议是 mutually exclusive**

> 一个 channel → 一个协议实现

### ✅ 原则 3：**多协议 ≠ 多字段**

而是：

> **多协议 = 多 subnode / union**

---

## 推荐的 DTS 正确写法（工业级）

### ✅ 方案 A（强烈推荐）：协议子节点（类似 device tree 的 reg/interrupt）

```dts
channel@3001 {
    tag-id = <3001>;
    unit = "kWh";

    dlms {
        obis = "1-0:1.8.0";
    };

    modbus {
        addr = <40001>;
        func = <3>;
    };

    can {
        can-id = <0x18FF50E5>;
        scale = <100>;
    };
};
```

### 含义非常清晰：

| 层级              | 含义        |
| --------------- | --------- |
| channel         | 业务点（语义唯一） |
| dlms/modbus/can | 协议实现      |
| 生成阶段            | **只选一个**  |

---

## binding 如何约束（非常关键）

### edgeos,channel.yaml

```yaml
properties:
  tag-id:
    type: integer

oneOf:
  - required: [dlms]
  - required: [modbus]
  - required: [can]

properties:
  dlms:
    type: object
    required: [obis]

  modbus:
    type: object
    required: [addr]

  can:
    type: object
    required: [can-id]
```

👉 **dtc 会直接报错**：

* 同时定义 dlms + modbus ❌
* 一个都没定义 ❌

---

## C 代码生成（干净到极致）

### auto-generated channel_map.h

```c
typedef enum {
    PROTO_DLMS,
    PROTO_MODBUS,
    PROTO_CAN,
} proto_t;

typedef struct {
    uint32_t tag_id;
    proto_t proto;

    union {
        struct {
            const char *obis;
        } dlms;

        struct {
            uint16_t addr;
            uint8_t func;
        } modbus;

        struct {
            uint32_t can_id;
            uint16_t scale;
        } can;
    };
} channel_map_t;
```

### 生成时只保留一个 union 分支

---

## Driver 侧使用（非常舒服）

```c
void on_protocol_value(const channel_map_t *ch, double raw)
{
    double val = raw;

    shmring_write(ch->tag_id, &val, sizeof(val));
}
```

**Driver 永远不关心 tag 是干嘛的**。

---

## ❌ 为什么不推荐你原来的“平铺字段”

| 问题                 | 后果      |
| ------------------ | ------- |
| can_id / obis 混在一起 | 语义不清    |
| 空字段                | 二开误用    |
| 无法校验               | 配错即运行时炸 |
| 无法裁剪               | 代码臃肿    |

---

## 最终推荐模板（你可以直接锁定）

```dts
channel@3001 {
    tag-id = <3001>;
    name = "energy_import";
    unit = "kWh";

    dlms {
        obis = "1-0:1.8.0";
    };
};
```

或：

```dts
channel@1001 {
    tag-id = <1001>;
    unit = "V";

    modbus {
        addr = <300>;
        func = <4>;
        scale = <10>;
    };
};
```

---

## 总结（你现在该怎么做）

✔ **你这个直觉是对的：一个 channel 描述所有协议可能性**
❌ **但实现方式必须是“结构化互斥”，不是字段堆砌**

完全可行，而且是工程级 **静态生成 tag_id + 协议映射表 + 快速查找 API** 的最佳实践。下面我把方案彻底整理成一套可落地、可自动化执行的设计，并给出 **DTS/HCS → 枚举 + 协议表 → C 代码自动生成** 的具体细则和示例。

---

# 🧠 核心目标

我们要实现这样的能力：

✔ 使用 **统一的配置源（DTS / HCS / YAML）** 描述所有点位及协议映射
✔ 编译时自动生成：

* **tag_id 枚举（C 枚举/宏）**
* **协议映射表（DLMS OBIS / Modbus Addr / CAN ID / …）**
* **反向查找函数（tag_id → 协议信息）**
  ✔ 支持 **多产品差异化**（同一 tag 不同产品对应不同协议属性）
  ✔ 运行时只需通过 **tag_id 访问所有元数据**
  ✔ 确保 tag_id 全局唯一、稳定、可追溯

---

# 📦 方案结构（从规范到代码）

我们提供三个层级：

1. **tag_catalog.dsl**（统一数据源，一源多用）
2. **代码生成模板（Python/Generator）**
3. **自动生成结果** → C 枚举 + 协议映射表 + 查询 API

---

# 🧾 一、DSL 样例：统一定义 tag + 协议 + 产品差异

我们使用一种接近 DTS / HCS 语法的 DSL（也可由设备树 *.dts 自动转换而来）。

**edgeos-meter.tagdsl**

```dts
// 统一 Tag 定义 + 协议映射表
// 支持多协议字段与产品覆盖

tag_catalog {
    channel {
        name = "ActiveEnergy";
        tag_id = 3001;
        unit = "kWh";

        // Protocol mappings
        protocols {
            dlms {
                obis = "1-0:1.8.0";
                scaler = 0.001;
            }
            modbus {
                addr = 0x0100;
                length = 2;
                type = "uint32";
            }
            can {
                can_id = 0x181;
                factor = 1.0;
            }
        }

        // Product override (optional)
        products {
            HEMS {
                modbus {
                    addr = 0x0110;
                }
            }
            IEMS {
                dlms {
                    obis = "1-0:1.8.1";
                }
            }
        }
    }

    channel {
        name = "VoltageRMS";
        tag_id = 3002;
        unit = "V";
        protocols {
            modbus {
                addr = 0x0200;
                length = 1;
                type = "uint16";
                scale = 0.1;
            }
        }
    }
}
```

---

# 📊 二、编译期生成代码设计

我们将从 DSL → 生成以下三个文件：

```
include/tag_catalog.h
src/tag_catalog.c
include/tag_lookup.h
```

---

## 🔹 1. 自动生成 `tag_catalog.h`

```c
#ifndef TAG_CATALOG_H
#define TAG_CATALOG_H

typedef enum {
    TAG_INVALID = 0,
    TAG_ACTIVE_ENERGY = 3001,
    TAG_VOLTAGE_RMS   = 3002,
    TAG_MAX
} tag_id_t;

// 通用单位
const char* tag_unit(tag_id_t tag);

#endif // TAG_CATALOG_H
```

---

## 🔹 2. 自动生成 `tag_catalog.c`

```c
#include "tag_catalog.h"

// 统一标签单位表
static const struct {
    tag_id_t tag;
    const char *unit;
} s_tag_units[] = {
    { TAG_ACTIVE_ENERGY, "kWh" },
    { TAG_VOLTAGE_RMS,   "V"   },
    { TAG_INVALID,       NULL  }
};

const char* tag_unit(tag_id_t tag) {
    for (int i = 0; s_tag_units[i].unit; i++) {
        if (s_tag_units[i].tag == tag) return s_tag_units[i].unit;
    }
    return NULL;
}
```

---

## 🔹 3. 协议映射表（多协议支持）

针对每个协议生成独立或统一查询接口。

### example: DLMS 映射表

```c
typedef struct {
    tag_id_t tag;
    const char *obis;
    double scaler;
} tag_dlms_map_t;

static const tag_dlms_map_t s_tag_dlms_map[] = {
    { TAG_ACTIVE_ENERGY, "1-0:1.8.0", 0.001 },
    { TAG_VOLTAGE_RMS, NULL, 1.0 },
    { TAG_INVALID, NULL, 0 }
};

const tag_dlms_map_t* tag_dlms_lookup(tag_id_t tag) {
    for (int i=0; s_tag_dlms_map[i].tag != TAG_INVALID; i++) {
        if (s_tag_dlms_map[i].tag == tag) return &s_tag_dlms_map[i];
    }
    return NULL;
}
```

### example: Modbus 映射表

```c
typedef struct {
    tag_id_t tag;
    uint16_t addr;
    uint8_t  length;
    const char* type;
    double    scale;
} tag_modbus_map_t;

static const tag_modbus_map_t s_tag_modbus_map[] = {
    { TAG_ACTIVE_ENERGY, 0x0100, 2, "uint32", 1.0 },
    { TAG_VOLTAGE_RMS,   0x0200, 1, "uint16", 0.1 },
    { TAG_INVALID,       0,      0, "", 0 }
};

const tag_modbus_map_t* tag_modbus_lookup(tag_id_t tag) {
    for (int i=0; s_tag_modbus_map[i].tag != TAG_INVALID; i++) {
        if (s_tag_modbus_map[i].tag == tag) return &s_tag_modbus_map[i];
    }
    return NULL;
}
```

---

# 🛠 三、快速查找函数设计

为了简化调用，可生成统一接口：

```c
typedef struct {
    tag_id_t tag;
    const char *protocol;
    void *info;
} tag_lookup_result_t;

int tag_lookup(tag_id_t tag, const char *protocol, tag_lookup_result_t *out) {
    if (strcmp(protocol, "dlms") == 0) {
        const auto *r = tag_dlms_lookup(tag);
        if (r) { out->tag=tag; out->protocol="dlms"; out->info=(void*)r; return 0; }
    }
    if (strcmp(protocol, "modbus")==0) {
        const auto *r = tag_modbus_lookup(tag);
        if (r) { out->tag=tag; out->protocol="modbus"; out->info=(void*)r; return 0; }
    }
    return -1;
}
```

这样你在驱动中可以写：

```c
tag_lookup_result_t info;
if (tag_lookup(TAG_ACTIVE_ENERGY, "modbus", &info)==0) {
    auto *m = (tag_modbus_map_t*)info.info;
    do_modbus_read(m->addr, m->length, m->type, m->scale, ...);
}
```

---

# 🔄 四、如何结合 DTS/HCS 自动生成

### 🔹 DTS → DSL

如果你有设备树 DTS，例如：

```dts
channel@3001 {
    compatible = "edgeos,channel";
    tag-id = <3001>;
    obis-code = "1-0:1.8.0";
    modbus-addr = <0x100>;
    scale = <1000>;
};
```

你可以写一个 Python 脚本，类似：

```python
import dtcparser

d = dtcparser.parse("edgeos-meter.dts")
for node in d.nodes:
    # 提取 compatible/tag-id/protocol props
    emit_tagdsl(node)
```

将 DTS 转换为我们的 DSL。

---

# 🧬 五、多产品型号差异化支持

在 tagdsl 中，我们已经支持 `products { ... }` 覆盖定义。

例如：

```dts
channel@3001 {
    compatible = "edgeos,channel";
    tag-id = <3001>;
    ...
    product-overrides = <"HEMS", "IEMS">;
};
```

生成器会输出 **不同产品的映射表**：

```c
// 在 tag_catalog.c
static const tag_modbus_map_t s_map_IEMS[] = {...};
```

并在 lookup 时按当前产品选择合适表。

---

# 📈 六、优点总结

| 特性             | 收益            |
| -------------- | ------------- |
| 一源多用（DSL）      | 避免重复维护映射表     |
| 编译时生成          | 无运行时性能开销      |
| tag_id 与协议映射分离 | 驱动和 Core 解耦   |
| 产品覆盖支持         | 不同产品可复用相同命名体系 |
| 自动查找 API       | 驱动层调用方便       |

---

# 🧩 七、自动化生成流程建议

建议按下面步骤集成到工程：

```
edgeos/definitions/
    edgeos-meter.dts
    edgeos-other.dts
build/gen_tag_catalog.py  ← 生成器
generated/
    tag_catalog.h
    tag_catalog.c
    tag_lookup.h
```

在 CMake 配置中：

```cmake
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/tag_catalog.c
    COMMAND python3 ${PROJECT_SOURCE_DIR}/build/gen_tag_catalog.py ...
)
```

---

# 🧠 最佳实践 & 开发者指南

📌 **驱动开发者只关心 tag_id → protocol info**
📌 **协议逻辑放在 Driver 层，不出现在 Core / Pipeline**
📌 **tag_catalog 绝对不可动态生成（编译期生成保持稳定）**
📌 **协议覆盖尽量放在 DSL（版本控制）**

---

如果你需要，我还可以：

✅ 提供 **完整 Python 生成器代码**
✅ 提供 **OpenAPI schema 自动生成 tagdsl 的能力**
✅ 与你现有工具链（如编译时 Dynamo / CMake）无缝集成

---

总结一下：
✔ 使用 DTS/HCS/YAML 一套 DSL 描述所有 tag 与协议
✔ 编译期 → 生成 tag 枚举、映射表和查找 API
✔ 支持多产品覆盖和快速运行时查找
✔ 大幅提升 Driver 层开发效率和一致性


你这一步提得**非常关键，而且是“能把 EdgeOS 从工程堆砌拉到工程体系”的那一步**。
我这次**不再泛泛而谈**，而是给你一套**可以直接落地、编译期可验证、运行期零歧义**的完整方案。

下面内容我分 **5 层**来讲，**每一层都可直接干活**：

---

# Phase 3 · 高质量版

## 基于 **HCS / DTS** 的 Tag 体系：

> **编译期生成 tag_id 枚举 + 协议映射表 + O(1) 查找函数**

---

## 一、先把原则说死（这是前面混乱的根源）

### ❌ 错误做法（一定要避免）

* tag_id **运行期动态分配**
* tag_id = hash(obis / modbus addr / can id)
* 协议 ID = tag_id（强绑定）

### ✅ 正确原则（EdgeOS 必须遵守）

> **tag_id 是系统内部的“语义主键”，与任何具体协议解耦**

| 概念                  | 含义                         |
| ------------------- | -------------------------- |
| **tag_id**          | EdgeOS 内部统一语义 ID（全系统唯一、稳定） |
| OBIS / Modbus / CAN | **只是获取该语义的不同“物理路径”**       |
| 协议可以变               | tag_id **不能变**             |
| 一个 tag_id           | **可映射多个协议通道**              |

👉 这跟 **DLMS、IEC、EdgeX、OPC UA** 的设计思想是完全一致的。

---

## 二、HCS / DTS 语法 —— 这次用“真的能编译”的写法

> **HCS = Harmony Config Source**
> 本质是 **DTS 超集**，支持：

* node
* property
* string / int / array
* label
* phandle

### 1️⃣ 顶层：tag catalog（唯一源头）

### `edgeos-meter.hcs`

```dts
/tag-catalog {
    compatible = "edgeos,tag-catalog";

    tag_3001: tag@3001 {
        tag-id = <3001>;
        name = "active_energy_total";
        unit = "kWh";
        type = "float64";
        direction = "meter->system";

        protocols {
            dlms {
                obis = "1-0:1.8.0";
            };

            modbus {
                holding-reg = <40001>;
                scale = <1000>;
            };

            can {
                can-id = <0x18FF50E5>;
                offset = <0>;
            };
        };
    };

    tag_3002: tag@3002 {
        tag-id = <3002>;
        name = "voltage_l1";
        unit = "V";
        type = "float32";

        protocols {
            dlms {
                obis = "1-0:32.7.0";
            };
        };
    };
};
```

🔒 **关键点**

* `tag@3001` → **人类可读**
* `tag-id = <3001>` → **机器唯一**
* `protocols {}` → 可扩展、可裁剪

---

## 三、生成目标（你真正想要的东西）

通过 **hc_gen / dtc + 自定义后处理**，生成：

---

## 1️⃣ tag_id 枚举（编译期常量）

### `edgeos_tag_id.h`

```c
#pragma once

typedef enum {
    TAG_ACTIVE_ENERGY_TOTAL = 3001,
    TAG_VOLTAGE_L1          = 3002,
} edgeos_tag_id_t;
```

✔ 可用于：

* C switch
* SQLite schema
* REST / JSON
* Stream Engine
* Pipeline

---

## 2️⃣ 协议 → tag_id 对照表（只读）

### DLMS OBIS → tag

```c
typedef struct {
    const char *obis;
    edgeos_tag_id_t tag_id;
} dlms_tag_map_t;

static const dlms_tag_map_t dlms_tag_map[] = {
    { "1-0:1.8.0", TAG_ACTIVE_ENERGY_TOTAL },
    { "1-0:32.7.0", TAG_VOLTAGE_L1 },
};
```

### Modbus → tag

```c
typedef struct {
    uint16_t reg;
    edgeos_tag_id_t tag_id;
    uint32_t scale;
} modbus_tag_map_t;

static const modbus_tag_map[] = {
    { 40001, TAG_ACTIVE_ENERGY_TOTAL, 1000 },
};
```

---

## 3️⃣ 反查表（tag → 协议信息）

```c
typedef struct {
    edgeos_tag_id_t tag_id;
    const char *name;
    const char *unit;
    const char *dlms_obis;
    int modbus_reg;
} tag_desc_t;

static const tag_desc_t tag_desc[] = {
    {
        .tag_id = TAG_ACTIVE_ENERGY_TOTAL,
        .name = "active_energy_total",
        .unit = "kWh",
        .dlms_obis = "1-0:1.8.0",
        .modbus_reg = 40001,
    },
};
```

✔ 用于：

* Web UI
* REST
* SQLite schema
* 调试 dump

---

## 四、快速查找函数（O(1) / O(logN)）

### 1️⃣ 协议 → tag_id

```c
edgeos_tag_id_t tag_from_obis(const char *obis)
{
    for (size_t i = 0; i < ARRAY_SIZE(dlms_tag_map); i++) {
        if (!strcmp(dlms_tag_map[i].obis, obis))
            return dlms_tag_map[i].tag_id;
    }
    return TAG_INVALID;
}
```

> 如果 tag 数 > 1000
> → **生成 perfect hash / binary search table**

---

### 2️⃣ tag_id → 描述

```c
const tag_desc_t *tag_desc_get(edgeos_tag_id_t tag)
{
    for (size_t i = 0; i < ARRAY_SIZE(tag_desc); i++) {
        if (tag_desc[i].tag_id == tag)
            return &tag_desc[i];
    }
    return NULL;
}
```

---

## 五、和你前面系统设计的 **强绑定关系**

### ✔ DataHub / ShmRing

* **只流转 tag_id + value + ts**
* 不关心 OBIS / Modbus

### ✔ Stream Engine

* Pipeline 按 **tag_id 订阅**
* 冻结 / 曲线 / 结算全部是 tag 级别

### ✔ SQLite

```sql
(tag_id INTEGER, ts INTEGER, value REAL)
```

### ✔ Web / REST

```json
{
  "tag_id": 3001,
  "name": "active_energy_total",
  "unit": "kWh"
}
```

---

## 六、你问的那句：**“可以这样写吗？”**

```dts
channel {
    obis = "1-0:1.8.0";
    tag_id = 3001;
}
```

### ❌ 不推荐（原因很明确）

* tag_id **不应该在 channel 里定义**
* channel 是 **协议视角**
* tag 是 **系统语义视角**

### ✅ 正确做法（单一事实源）

> **tag 定义 tag_id
> 协议 channel 只是引用**

---

## 七、最终定版（以后不要再改的结论）

✔ **tag_id**

* 编译期生成
* 全系统唯一
* 与协议解耦

✔ **HCS / DTS**

* 作为 **Tag Catalog 单一源头**
* 自动生成：

  * enum
  * map
  * schema
  * lookup

✔ **运行期**

* 只认 tag_id
* 协议只是输入/输出适配层

---

## 生成 perfect hash / binary search table

好，这一步我们**直接进入工程级“可落地实现”**。
我给你两套方案，**都是真正在工业项目里用的**，你可以二选一，甚至两套并存：

---

# 目标回顾（把问题钉死）

我们要解决的是：

> **协议标识（OBIS / Modbus / CAN） → tag_id**
> 在 **运行期高频调用**
> 要求：
>
> * 快
> * 稳定
> * 零动态分配
> * 最好编译期生成

---

# 方案 A：Perfect Hash（推荐，工业级）

## 什么时候用？

* tag 数量 ≥ 50
* lookup 非常频繁（采集线程 / stream engine）
* 协议 key 是 **字符串**（OBIS / JSON key）

👉 **DLMS OBIS 场景首选**

---

## A1️⃣ 原理一句话

> 用 **gperf** 在编译期生成
> **无冲突、O(1)** 的字符串 → tag_id 映射函数

* 无 for-loop
* 无 strcmp 链
* 无 hash 冲突

---

## A2️⃣ 输入文件（自动生成）

由 `edgeos-meter.hcs` 生成：

### `dlms_tag.gperf`

```gperf
%language=C
%readonly-tables
%struct-type
%define hash-function-name dlms_hash
%define lookup-function-name dlms_tag_lookup
%define slot-name obis
%define initializer-suffix ,0
%define word-array-name dlms_wordlist

struct dlms_entry {
    const char *obis;
    int tag_id;
};

%%
"1-0:1.8.0", 3001
"1-0:32.7.0", 3002
%%
```

✔ 这是**唯一的维护点**
✔ 永远不手写

---

## A3️⃣ gperf 生成代码

```bash
gperf dlms_tag.gperf > dlms_tag_lookup.c
```

生成的核心接口类似：

```c
const struct dlms_entry *
dlms_tag_lookup(const char *str, size_t len);
```

---

## A4️⃣ 封装成 EdgeOS API（你真正用的）

```c
edgeos_tag_id_t tag_from_obis(const char *obis)
{
    const struct dlms_entry *e;

    e = dlms_tag_lookup(obis, strlen(obis));
    if (!e)
        return TAG_INVALID;

    return (edgeos_tag_id_t)e->tag_id;
}
```

---

## A5️⃣ 特性总结（为什么这是“最优解”）

| 特性    | 说明                                   |
| ----- | ------------------------------------ |
| 时间复杂度 | **O(1)**                             |
| 内存    | 只读 `.rodata`                         |
| 线程安全  | 天然                                   |
| 编译期   | 决定一切                                 |
| 工业验证  | Linux kernel / systemd / busybox 都在用 |

---

# 方案 B：Binary Search Table（简单 + 稳）

## 什么时候用？

* tag 数量 ≤ 200
* key 是 **整数**
* 不想引入 gperf 工具链

👉 **Modbus / CAN / 数值型 ID** 首选

---

## B1️⃣ 生成排序表（编译期）

### `modbus_tag_table.c`

```c
typedef struct {
    uint16_t reg;
    edgeos_tag_id_t tag_id;
} modbus_tag_map_t;

/* 必须按 reg 排序 */
static const modbus_tag_map_t modbus_tag_map[] = {
    { 30001, TAG_VOLTAGE_L1 },
    { 40001, TAG_ACTIVE_ENERGY_TOTAL },
    { 40002, TAG_REACTIVE_ENERGY },
};
```

---

## B2️⃣ 二分查找函数（零依赖）

```c
edgeos_tag_id_t tag_from_modbus_reg(uint16_t reg)
{
    int l = 0;
    int r = ARRAY_SIZE(modbus_tag_map) - 1;

    while (l <= r) {
        int m = (l + r) >> 1;

        if (modbus_tag_map[m].reg == reg)
            return modbus_tag_map[m].tag_id;

        if (modbus_tag_map[m].reg < reg)
            l = m + 1;
        else
            r = m - 1;
    }

    return TAG_INVALID;
}
```

---

## B3️⃣ 特性总结

| 特性    | 说明      |
| ----- | ------- |
| 时间复杂度 | O(logN) |
| N=128 | ≈ 7 次比较 |
| 内存    | 极小      |
| 调试    | 极其友好    |

---

# A vs B 怎么选（直接给结论）

| 场景          | 方案                     |
| ----------- | ---------------------- |
| DLMS / OBIS | ✅ **Perfect Hash (A)** |
| JSON key    | ✅ Perfect Hash         |
| Modbus 寄存器  | ✅ Binary Search        |
| CAN ID      | Binary Search 或 switch |
| tag < 20    | for-loop 都行            |

👉 **你这个项目，必然是 A + B 混合**

---

# 和你前面 Tag / Stream Engine 的强耦合点

✔ driver / protocol 层

```c
tag = tag_from_obis(obis);
```

✔ DataHub / ShmRing

```c
{ tag_id, ts, value }
```

✔ Stream Engine

```c
pipeline_subscribe(TAG_ACTIVE_ENERGY_TOTAL);
```

✔ SQLite

```sql
PRIMARY KEY (tag_id, ts)
```

---

# 强烈建议你定下这条“铁律”

> ❗ **EdgeOS 不允许出现运行期 tag 注册**
>
> * 不 malloc tag
> * 不 hash tag
> * 不 string compare tag
>
> **tag 只在编译期存在**

---

## 假如  我说假如  一个tag对应一个obis  但是需求有变  或者为了兼容老表  而老表对应的obis不一样  即要根据表型等信息得到真实的obis  这种需求如何做
这是个**非常专业、而且现实里一定会发生**的问题 👍
你这个“**假如**”，其实已经把系统从“实验室设计”拉到了**真实电表现场**。

我先给结论，再给**工程化落地方案**，最后给**你现在这套 tag / stream engine 体系该怎么接**。

---

## 一句话结论（先稳住）

> **tag_id 永远不变**
> **OBIS 不是 tag 的唯一真相**
>
> **tag →「语义」**
> **OBIS →「某一类设备的实现细节」**

所以：
👉 **不能再用「tag ↔ OBIS 一对一」模型**

---

# 一、正确的抽象层级（非常关键）

### ❌ 错误模型（你已经意识到有问题）

```
tag_id  ↔  obis
```

一变表型就炸

---

### ✅ 正确模型（工业级）

```
            ┌────────────┐
            │ device_model│
            │ device_rev  │
            └─────┬──────┘
                  │
                  ▼
tag_id ──► semantic ──► protocol binding ──► obis / reg / can_id
```

**解释**：

| 概念       | 含义            |
| -------- | ------------- |
| tag_id   | 业务语义（“总有功电能”） |
| semantic | 抽象物理量定义       |
| binding  | 不同表型的映射       |
| OBIS     | 具体设备实现        |

---

# 二、你这个需求在真实项目中怎么做？

## 核心思路：**Tag 不动，Binding 可变**

### 新增一个维度：**Profile / Device Model**

---

## 1️⃣ 引入 Device Profile（不是 EdgeX 那种重）

```c
typedef struct {
    uint16_t vendor;
    uint16_t model;
    uint16_t hw_rev;
    uint16_t fw_rev;
} device_profile_id_t;
```

采集线程启动时就确定：

```c
ctx->profile = detect_device_profile();
```

---

## 2️⃣ Binding 表：tag + profile → OBIS

### 从「一张表」变成「多张表」

#### `dlms_binding.gperf`（逻辑结构）

```text
(profile_id, tag_id) → obis
```

---

## 3️⃣ 工程可行的两种实现方式

---

# 方案 A（推荐）：**Profile 切换 gperf 表**

### 编译期生成 **每个表型一张表**

```
dlms_bindings/
├── acme_a100.c
├── acme_a200.c
├── legacy_x9.c
```

#### 每个文件内部：

```c
edgeos_tag_id_t tag;
const char *obis;
```

启动时绑定函数指针：

```c
struct dlms_binding_ops {
    const char *(*tag_to_obis)(edgeos_tag_id_t tag);
};

ctx->dlms_ops = &acme_a100_ops;
```

### 使用时：

```c
obis = ctx->dlms_ops->tag_to_obis(TAG_ACTIVE_ENERGY_TOTAL);
```

#### ✅ 优点

* 极快（O(1) / switch）
* gperf 表规模小
* 完全编译期
* 调试友好

#### ❌ 缺点

* 表型很多时，代码量增加（但这是现实）

---

# 方案 B：**runtime profile-aware lookup**

### Key = `(profile_id, tag_id)`

#### gperf key 变成字符串：

```text
"acme_a100:3001"
"legacy_x9:3001"
```

```c
snprintf(key, "%s:%d", profile_name, tag);
```

#### 查表：

```c
entry = lookup(key);
```

#### ⚠️ 不推荐原因

* 运行期拼 key
* 不利于 driver 层高频调用
* 混合了 runtime 逻辑

---

## 👉 你的项目 **明显适合方案 A**

---

# 三、那老表 / 兼容性问题怎么解决？

### 举个你说的真实例子

| tag  | 语义    | 新表 OBIS   | 老表 OBIS   |
| ---- | ----- | --------- | --------- |
| 3001 | 总有功电能 | 1-0:1.8.0 | 1-0:1.8.1 |

---

### 在 binding 表里这样：

```c
// acme_new.c
case TAG_ACTIVE_ENERGY_TOTAL:
    return "1-0:1.8.0";

// acme_legacy.c
case TAG_ACTIVE_ENERGY_TOTAL:
    return "1-0:1.8.1";
```

**业务层完全无感知**。

---

# 四、这和你前面 tag / pipeline / sqlite 设计是“完美兼容”的

### DataHub / ShmRing

```c
{ tag_id=3001, value=..., ts=... }
```

✔ 永远不关心 OBIS

---

### Stream Engine

```c
FilterByTag(3001)
```

✔ 不变

---

### SQLite

```sql
(tag_id, ts, value)
```

✔ 不变

---

### 变化只发生在：

```
Driver / Protocol Adapter
```

---

# 五、进一步升级（你明显会走到这一步）

## 同一个 tag 多个 OBIS（高级表）

有些表：

* 瞬时功率 = OBIS A + OBIS B
* 需计算

### Binding 支持「表达式」

```c
typedef enum {
    BIND_SIMPLE,
    BIND_EXPR,
} bind_type_t;

typedef struct {
    bind_type_t type;
    union {
        const char *obis;
        const char *expr; // "1-0:21.7.0 - 1-0:41.7.0"
    };
} dlms_binding_t;
```

表达式直接送进你前面设计的 **Stream Engine / JS / Lua** 👌

---

# 六、一句话帮你把这块设计“钉死”

> **tag_id = 语义契约（core contract）**
> **OBIS = 某一类设备的实现细节**
>
> **兼容性问题，永远在 binding 层解决**

---


