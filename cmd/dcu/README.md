# DL/T 698.45 工业级智能配电终端 (DCU) 生产级设计与执行手册

> **项目状态**：生产级规约（Production-Ready Spec）
> **目标应用**：`/workspaces/edgeos/EdgeC/cmd/dcu`

---

## 一、 集中器（DCU）五位一体生产级核心业务环设计

在配电网台区现场，智能配电终端 (DCU) 的核心研发必须建立在对以下 **五位一体核心业务环** 的深刻理解之上，它们涵盖了从物理感知到控制反馈、北向通信的完整工业因果闭环：

```
+-----------------------------------------------------------------------------------+
|                              五位一体核心业务闭环                                 |
+-----------------------------------------------------------------------------------+
|  1. 计量数据 (0000-1FFF)   ==> 依靠标准规约，原封不动采集沉淀，定时日/月冻结归档。|
|  2. 采集数据 (2000-2FFF)   ==> 高频遥测瞬时感应，影子寄存器DataHub分发。          |
|  3. 处理数据 (边缘算法)    ==> 越限判断、电压不平衡、三相波形畸变滑窗计算。       |
|  4. 控制设备 (Class 13)    ==> 异常保护断电切除、负荷拉闸控制（安全鉴权）。       |
|  5. 数据通信 (Class 25/26) ==> 北向A-XDR通信、链路加密防篡改、SnF断线缓存重发。   |
+-----------------------------------------------------------------------------------+
```

### 1. 计量数据 (Metrology Data) —— 交易结算的“法制基石”
*   **物理对象**：正向有功电量（`OAD 00100200`）、反向有功电量（`OAD 00200200`）等法制电网结算量（`OI 0000-1FFF`）。
*   **业务逻辑**：此类数据具有法律效力和结算性质。集中器通过物理信道读取后，**必须原封不动地存储**。每逢零点触发“时空快照”，执行日冻结/月冻结（Class 9），归档入 SQLite，绝对不许做任何平滑、滤波等破坏法制计量数据原始性与完整性的操作。

### 2. 采集数据 (Acquisition Data) —— 运行监测的“即时触角”
*   **物理对象**：电压（`OAD 20000200`）、电流（`OAD 20010200`）、有功功率（`OAD 20040200`）等电网高频瞬时参量（`OI 2000-2FFF`）。
*   **业务逻辑**：具有极强的时间敏感性（如 5s 轮询）。由 `dcu_poller` 高频抄读，绕过数据库直推 `DataHub` 实时数据总线，以内存“影子寄存器”状态供全系统各消费组件瞬间监听。

### 3. 处理数据 (Processed Data) —— 本地决策的“智能大脑”
*   **物理对象**：电压不平衡率（`OAD 20260200`）、电压/电流谐波含有量（`OAD 200D/200E`）、负载率（`OAD 20280200`）及 Class 12, 14, 15, 16（区间、极值、滑窗需量）衍生数据。
*   **业务逻辑**：由 `edge_stream` 算子管道对 `DataHub` 中的“采集数据”进行高频边缘计算（如卡尔曼滤波、三相不平衡度计算、失压断流越限判定），生成二次高价值指标并重新推回总线，直接触发本地警报与控制闭环。

### 4. 控制设备 (Control / Action) —— 安全防线的“物理闸刀”
*   **物理对象**：Class 13 负荷控制对象类、控制命令状态字（`OAD 20400600`）及外部拉闸/合闸继电器动作。
*   **业务逻辑**：当本地“处理数据”判定发生过压、过载等越限异常，或收到北向拉闸指令时，控制模块在完成 ESAM 芯片安全鉴权与 License 授权检验后，执行下发 `ACTION` 指令切除物理负荷开关，实施主动保护。

### 5. 数据通信 (Data Communication) —— 可靠互联的“传输管道”
*   **物理对象**：北向 A-XDR 报文打包（`northbound_mgr`）、Class 20/21 安全认证链路、Class 25/26 网口/4G 参数、上报方案（`OI 601D`）与 SnF 断线缓存。
*   **业务逻辑**：在北向通道正常时，实时将采集/计量数据上报；当通信链路因台风、电网波动断开时，自动启动 `SnF` 断线缓存，将断面冻结历史数据原子化存盘；通信恢复后按时序完整回放，保障电网通信链路的高可靠性。

---

## 二、 工业级生产背景与产品定位

智能配电终端（Data Concentrator Unit, DCU）部署于电网物理现场的变压器台区或开关柜中。为了达到**可直接量产、工业级可生产**的水平，本系统基于 EdgeOS 事件驱动内核（libuv）进行深度定制开发，绝非简单的报文拼接 Demo。

在实际生产环境中，DCU 面临极度严苛的挑战：
1.  **Flash 写入磨损与寿命危机**：网关通常采用 SPI NAND Flash，如果在每次高频抄表时直接对 SQLite 进行单条写入，会因为垃圾回收（GC）机制导致 Flash 在数月内读写寿命耗尽（Flash Wear-Out）。
2.  **掉电数据完整性**：配电现场电压异常波动、瞬间停电频发。必须保证掉电瞬间数据不损坏、不丢失，重启后系统能 100% 自动恢复。
3.  **并发限流与轮询死锁**：当存在多路 485 物理串口且部分电表离线时，若驱动层发生阻塞或无限制等待，将导致整个事件循环挂起。
4.  **高精度时钟对齐 (MTT)**：电能计量要求历史日冻结（Class 9）必须在 24:00:00.000 毫秒级对齐。

---

## 二、 生产级核心架构设计

```
                         【工业级生产架构图】
                                 
      +--------------------------------------------------------+
      |               北向主站 / 通信前置机                    |
      +--------------------------------------------------------+
                                  ^ (DL/T 698.45 协议报文)
                                  |
                                  v
      +--------------------------------------------------------+
      |                  Northbound Manager                    |
      |   - A-XDR 解析器 / 报文帧校验校验 (FCS16)               |
      |   - RSD 选择子处理器 (Record Selective Descriptor)      |
      +--------------------------------------------------------+
                                  ^
                                  | (DataHub 影子模型更新)
                                  v
      +--------------------------------------------------------+
      |                 DataHub 实时数据总线                    |
      |   - 线程安全内存映射                                   |
      |   - 影子状态分发 (Pub/Sub)                             |
      +--------------------------------------------------------+
             ^                                           |
             | (发布实时遥测)                             | (订阅 OAD 事件)
             |                                           v
+-------------------------+                 +--------------------------+
|  dcu_dynamic_poller     |                 |       dcu_storage        |
|  - 并发串口限流队列      |                 |  - SQLite WAL & 事务合并 |
|  - 485/PLC 主动轮询     |                 |  - Flash 寿命保护缓存池  |
|  - 离线表计心跳熔断器   |                 |  - SnF 断线缓存整合      |
+-------------------------+                 +--------------------------+
             |                                           |
             v (物理 IO 驱动)                            v (文件系统持久化)
+-------------------------+                 +--------------------------+
|    bus_orchestrator     |                 |    SQLite 3 数据库       |
|  (RS485 / PLC 物理通道) |                 |   (dcu.db / 索引深度优化)|
+-------------------------+                 +--------------------------+
```

---

## 三、 生产级关键技术方案

### 1. SQLite 寿命优化与“缓冲池 + 批量事务”合并机制
为了彻底解决 Flash 寿命磨损问题，`dcu_storage` 采用 **内存双缓冲（Double-Buffering Cache）** 架构加 **WAL（Write-Ahead Log）** 模式：
*   **缓冲池设计**：系统开辟双层缓存（Active Cache & Flush Cache）。收到的实时电参量（OAD 2000, 2001）首先写入内存缓存。
*   **批量事务写入**：每积攒 50 条数据或时间间隔达到 10 秒时，触发一次合并写操作。通过 `BEGIN IMMEDIATE TRANSACTION` 在一个事务内将多条记录批量写入 SQLite。这样可将 Flash 实际的写入扇区磨损降低 **95% 以上**。
*   **WAL 模式**：
    ```sql
    PRAGMA journal_mode = WAL;
    PRAGMA synchronous = NORMAL;
    ```
    在保证 ACID 特性的同时，最大化写吞吐量，且避免了写入时阻塞北向主站的高频读操作。

### 2. 物理通道串行队列调度与离线熔断机制 (半双工总线优化)
在配电网现场，RS485 总线是**半双工物理信道**，同一条总线（即同一个 `channel_id`）上挂载了数十只电表。集中器**不可能并发抄表，必须进行串行队列轮询**。
*   **轮询阻塞痛点**：主站下发给集中器的“采集周期”（如：5秒快速遥测，或 15分钟日电能量）是**全局方案配置（OI 6012）**。当某只电表发生损坏、断电离线时，每一次全局轮询都会在物理超时等待（如 `2秒超时 * 2次重试 = 4秒`）上浪费大量时间，从而导致整个物理通道发生严重的**队列拥堵与抄读时序累积漂移**，导致后续正常的在线电表根本无法在规定时间内抄回数据。
*   **队列跳过与静默熔断 (Queue Skipping & Circuit Breaker)**：
    *   **降级实质**：单表降级绝对不是修改单表的“采集周期定时器”（这会引起多路异步抄表冲突）。
    *   **队列过滤**：当某只电表连续 3 次在串行轮询中发生物理超时未响应时，在 `dcu_inventory` 中将其置为 `SUSPENDED`（熔断挂起）状态。
    *   **跳过执行**：后续全局采集方案（5s / 15min）在生成物理轮询队列时，引擎遍历到该表时，**直接跳过 (Skip) 对其的物理通信**。熔断表在正常采集队列中占用的物理总线时间变为 **0 毫秒**，从而彻底恢复了其余在线电表的高频遥测吞吐量。
*   **低频后台心跳探测 (Slow Heartbeat Recovery)**：
    *   轮询引擎在后台启动一个**低优先级、低频（例如每 10 分钟一次）**的专用心跳探测线程。
    *   该任务仅向被熔断的表计发送极轻量的物理握手帧（Get 逻辑名）。一旦子表重新响应，立刻将其状态翻转回 `ACTIVE`，无缝恢复其在全局采集队列中的串行抄读。这保证了即使单表损坏，全台区的秒级遥测更新也不会发生延时抖动。

### 3. 高精度时钟同步与零点冻结对齐 (MTT)
*   系统接收北向的 `SET-Request` 修改 `OAD 19000200`（系统时钟）时，通过 `clock_settime` 或内部高精度计时器进行系统校时，并使用 `adjtime` 实现平滑微调，防止时钟发生跳变导致冻结周期错乱。
*   日冻结触发时，使用精确到毫秒级的定时器。在每日 `23:59:59.900` 提前唤醒冻结线程，于 `24:00:00.000` 瞬间拍下实时电能（`OAD 00100200`）的内存快照，确保数据冻结误差小于 **50 毫秒**。

### 4. 掉电数据保护与原子存储
*   在系统接收到停电告警信号（高可靠网关通常备有大电容或备用电池，能维持 1.5 秒运行）时，中断所有物理轮询，立即对当前的 Telemetry 缓存执行强行刷盘（`fsync`），利用 SQLite 事务的原子性，确保已采集的最后断面历史数据完全落库，不发生数据库文件损坏（Database Corruption）。

---

## 四、 数据库物理模型与高性能索引设计

为应对大规模电表抄表（如 500 只表，采集 3 个月，产生上千万条遥测历史记录）的性能压力，对 SQLite 数据库的物理 Schema 进行深度优化设计：

```sql
-- 1. 电表档案表：记录已注册的电表及其物理连接属性
CREATE TABLE IF NOT EXISTS dcu_meters (
    meter_address TEXT PRIMARY KEY CHECK(length(meter_address) >= 4), -- TSA 地址 (C11 string 边界检验)
    protocol_type TEXT NOT NULL,                                     -- DLT645 / DLT698 / DLMS
    baud_rate INTEGER DEFAULT 2400,                                  -- 波特率
    channel_id INTEGER NOT NULL,                                     -- 关联的物理通道号
    is_active INTEGER DEFAULT 1                                      -- 激活状态
);

-- 2. 实时最新遥测缓存表 (用于支持 Class 1/2/3/4 GET 实时读取)
CREATE TABLE IF NOT EXISTS dcu_telemetry_latest (
    meter_address TEXT NOT NULL,
    oad TEXT NOT NULL,
    value REAL NOT NULL,
    updated_at INTEGER NOT NULL, -- UTC 毫秒级时间戳
    PRIMARY KEY (meter_address, oad),
    FOREIGN KEY(meter_address) REFERENCES dcu_meters(meter_address) ON DELETE CASCADE
);

-- 3. 历史断面冻结数据库表 (用于支持 Class 9 / OAD 5000-5006 GET-Request-Record 范围查询)
CREATE TABLE IF NOT EXISTS dcu_freeze_history (
    meter_address TEXT NOT NULL,
    freeze_oad TEXT NOT NULL,     -- 比如 "50040200" (日冻结), "50050200" (月冻结)
    freeze_time INTEGER NOT NULL,  -- 断面时间戳 (对应 OAD 20210200)
    source_oad TEXT NOT NULL,      -- 源参数 OAD，如 "00100200" (正向有功电量)
    frozen_value REAL NOT NULL,
    created_at INTEGER NOT NULL,
    PRIMARY KEY (meter_address, freeze_oad, freeze_time, source_oad),
    FOREIGN KEY(meter_address) REFERENCES dcu_meters(meter_address) ON DELETE CASCADE
);

-- 4. 高性能物理索引创建 (核心生产优化)
-- 解决北向根据时间范围 (RSD 机制) 批量提取历史冻结记录的性能痛点，避免全表扫描！
CREATE INDEX IF NOT EXISTS idx_freeze_query 
ON dcu_freeze_history (meter_address, freeze_oad, freeze_time);
```

---

## 五、 模块划分与生命周期管理

本应用在 `cmd/dcu` 下将代码组织成高度模块化的生产级结构：

### 1. 目录文件结构
```
edgeos/EdgeC/cmd/dcu/
├── README.md              # 本设计说明与执行计划文档
├── CMakeLists.txt         # CMake 编译配置文件
├── prj.conf               # Kconfig 功能开关
├── main.c                 # 系统入口与 libuv 协调中心
├── dcu_inventory.h        # 档案管理头文件
├── dcu_inventory.c        # 档案解析、动态增删与状态维护
├── dcu_poller.h           # 定时轮询引擎头文件
├── dcu_poller.c           # 并发限流、离线熔断与数据模拟发送
├── dcu_storage.h          # SQLite 缓冲写入头文件
└── dcu_storage.c          # 双缓冲事务合并写与 RSD 查询执行
```

### 2. 核心数据结构与函数原型声明（C11 标准）

#### `dcu_inventory.h` (档案管理)
```c
#ifndef CMD_DCU_INVENTORY_H
#define CMD_DCU_INVENTORY_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_METERS 256
#define MAX_ADDR_LEN 32

typedef struct {
    char address[MAX_ADDR_LEN];
    char protocol[16];
    uint32_t baud_rate;
    uint8_t channel_id;
    bool is_online;
    uint8_t fail_count; // 连续超时失败计数，用于离线熔断
} production_meter_t;

typedef struct {
    production_meter_t meters[MAX_METERS];
    uint16_t count;
    pthread_mutex_t lock; // 线程安全锁
} dcu_inventory_t;

int dcu_inventory_init(dcu_inventory_t *inv);
int dcu_inventory_add(dcu_inventory_t *inv, const char *addr, const char *proto, uint32_t baud, uint8_t channel);
int dcu_inventory_remove(dcu_inventory_t *inv, const char *addr);
production_meter_t* dcu_inventory_get(dcu_inventory_t *inv, const char *addr);
void dcu_inventory_destroy(dcu_inventory_t *inv);

#endif
```

#### `dcu_storage.h` (存储服务)
```c
#ifndef CMD_DCU_STORAGE_H
#define CMD_DCU_STORAGE_H

#include <sqlite3.h>
#include <uv.h>
#include "dcu_inventory.h"

#define WRITE_CACHE_MAX 100

typedef struct {
    char meter_address[MAX_ADDR_LEN];
    char oad[16];
    double value;
    uint64_t ts;
} telemetry_record_t;

typedef struct {
    sqlite3 *db;
    uv_loop_t *loop;
    telemetry_record_t cache[WRITE_CACHE_MAX];
    uint16_t cache_count;
    uv_mutex_t mutex;
    uv_timer_t flush_timer; // 定时批量写入事务触发器
} dcu_storage_t;

int dcu_storage_init(dcu_storage_t *storage, uv_loop_t *loop, const char *db_path);
int dcu_storage_write_telemetry(dcu_storage_t *storage, const char *addr, const char *oad, double value);
int dcu_storage_write_freeze(dcu_storage_t *storage, const char *addr, const char *freeze_oad, uint64_t freeze_time, const char *src_oad, double value);
int dcu_storage_flush(dcu_storage_t *storage);
void dcu_storage_destroy(dcu_storage_t *storage);

#endif
```

---

## 六、 严苛的原子 TDD 项目执行计划

为了保证代码的可生产性，我们完全杜绝零散编码。以下是 6 个 Task 的详细原子级 TDD 路线图。**每一个 Task 必须实现：先写红测试 $\to$ 验证通过 $\to$ Git Commit。**

### Task 1: A-XDR 电参量与时间戳编解码器 (TDD 单元测试)
*   **测试夹具**：`tests/test_dlt698_axdr.c`
*   **测试用例 A**：`test_axdr_voltage_codec` -> 校验浮点电压 `220.5V` (long-unsigned) 的 A-XDR 编解码是否完全符合 Tag 18 规范。
*   **测试用例 B**：`test_axdr_timestamp_codec` -> 校验时间戳 (date_time_s, Tag 28) 字节序列的编解码是否完美无损。
*   **成功准则**：`ctest -R test_dlt698_axdr` 绿灯通过，资源无任何溢出泄露。

### Task 2: 动态档案接收与离线熔断机制 (TDD 单元测试)
*   **测试夹具**：`tests/test_dcu_inventory.c`
*   **测试用例 A**：通过 Mock A-XDR 报文注入，动态调用 `dcu_inventory_add`，验证表计能否被正确注册入内存，以及 `dcu.db` 档案表中是否同步生成行记录。
*   **测试用例 B**：模拟电表通信连续 3 次超时失败，验证熔断状态是否被正确置为 `is_online = false`；再次模拟一次抄表成功，验证熔断是否自动恢复（`is_online = true`）。
*   **成功准则**：`ctest -R test_dcu_inventory` 绿灯通过。

### Task 3: 采集周期方案与定时器编排 (TDD 单元测试)
*   **测试夹具**：`tests/test_dcu_poller.c`
*   **测试用例 A**：配置一个方案：`方案ID = 1`, `周期 = 1秒`, `OAD列表 = [20000200, 20010200]`。在 libuv event loop 中运行 2.5 秒，确认相应的 `uv_timer_cb` 回调正好被触发 2 次，且每次回调中传入的 OAD 列表与方案定义完全吻合。
*   **成功准则**：`ctest -R test_dcu_poller` 绿灯通过。

### Task 4: 基于 DataHub 影子寄存器的生产数据模拟 (TDD 单元测试)
*   **测试夹具**：`tests/test_dcu_polling_loop.c`
*   **测试用例 A**：当 Poller 定时器触发时，模拟向虚拟子表抄读，并向 DataHub 发布 `OAD 20000200`（电压）、`OAD 20010200`（电流）、`OAD 20040200`（有功功率）的消息。在主循环中截获 DataHub 发布通知，判定发布的浮点数值处于物理合理区间。
*   **成功准则**：`ctest -R test_dcu_polling_loop` 绿灯通过。

### Task 5: 双缓冲 SQLite 事务批量刷盘与 RSD 查询 (TDD 单元测试)
*   **测试夹具**：`tests/test_dcu_storage.c`
*   **测试用例 A**：连续调用 `dcu_storage_write_telemetry` 写入 10 条数据。确认在主动触发 `dcu_storage_flush` 之前，SQLite 物理表中记录为 0；在 flush 之后，10 条数据通过一个事务原子性落盘。
*   **测试用例 B**：模拟历史日冻结 `50040200` 数据存入。执行带 RSD 的查询请求，传入 `RSD 时间区间` 过滤条件，确认返回的 A-XDR 行数组结构和数值正确。
*   **成功准则**：`ctest -R test_dcu_storage` 绿灯通过。

### Task 6: 掉电停电保护与主循环集成 (TDD 集成测试)
*   **测试夹具**：`tests/test_dcu_integration.c`
*   **测试用例 A**：模拟接收 `SIGTERM` / `停电中断信号`。确认系统立即触发 `dcu_storage_flush` 强行落盘，干净关闭数据库连接，退出 libuv 主循环。再次打开数据库，验证数据的完整性。
*   **成功准则**：`ctest -R test_dcu_integration` 绿灯通过。
