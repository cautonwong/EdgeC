# DL/T 698.45 集中器（`cmd/dcu`）生产化设计与实施计划

> 日期：2026-11-21　状态：执行中（operator 授权先行 P0；评审门保留）
> 读者：产品、架构、C 开发、协议与驱动、验证
> 关联：`docs/design/DCU_698_OBJECT_MODEL.md`、`docs/design/DCU_698_OI_CATALOG.md`、`docs/plans/2026-11-21-terminal-common-components-plan.md`、`docs/plans/2026-11-21-distribution-terminal-plan.md`、`docs/plans/2026-11-21-protocol-coverage.md`
> 用法：第 8 节的任务表就是派活单位，一任务一 issue 一 PR。第 9 节是无人值守执行协议。第 11 节的开放问题需要人拍板。

## How to read this

一个任务行就是一个工作单元。每行都写了类型、负责角色、估时、依赖、交付物和出口证据。出口证据必须是能复现的东西：测试输出、报文抓包、验收报告、提交 SHA。测试通过只是必要条件。本计划的完成定义是仓库口径：静态分析、测试、运行时验证证据三者齐全。

任务类型只有五种。`defect` 修既有错误，`feature` 加或改行为，`refactor` 不改行为只改结构，`docs` 与 `test` 只改文档或测试。重构与功能变更不混在同一个任务里，既有缺陷修复单独立项。这是 `AGENTS.md` 的约束，也是本计划拆任务的第一原则。

依赖写成 `D0-04` 这种任务号。`blocked` 表示依赖的外部计划尚未合并，任务不启动，也不允许在本产品内另写一套替代实现。

698 对象模型的全集在 `docs/design/DCU_698_OBJECT_MODEL.md`。26 个类的属性、方法与 513 个 OI 的完整清单在 `docs/design/DCU_698_OI_CATALOG.md`，由 `scripts/extract_dcu_698_catalog.py` 从 `docs/698_parsed.md` 生成。`docs/698_parsed.md` 是 `docs/698.pdf` 的抽取件，标准原文以 PDF 为准。本计划只写实施顺序与证据，不重复模型细节。

## 1. 结论

`cmd/dcu` 不是空壳。它有 29 个 C 源文件、28 个头文件、18 个 DCU 单元测试文件和两套产品验收任务，覆盖档案、轮询、SQLite 缓冲写、DataHub 影子、698 对象路由、南向驱动适配、部分事件与业务算法。问题不在“没有代码”，而在“代码已经够多，但生产路径还混着演示行为，698 对象模型只覆盖一小块，北向只能读四个 OI”。

本计划把 698 对象模型做全。26 个接口类全部进入范围，513 个 OI 全部进入目录，按阶段实现，不留“后续再看”的黑洞。实现顺序按业务价值与依赖排，不按类号排。

三件事先做：

1. 把演示行为从生产路径里拿掉，先把产品变成可长期运行的真实终端。
2. 用一张静态对象表替换手写的 `if (oi == ...)`，把 698 的 GET、SET、ACTION、Record、Report 五条路径接到同一张表上。
3. 按阶段补齐 26 个类的属性、方法与 OI，并把每个类的行为变成验收证据。

第一个可交付批次是 P0 加 P1A，约 29.5 人日：

- P0 去掉演示行为，修 stdout 缓冲规则，补基线证据，修文档漂移。
- P1A 建立对象模型生成链路、对象表与北向标量路径，先接 Class 1 到 10 的首批 OI。
- P1B 是 B1 到 B5 的协议栈缺口，16 人日，依赖 D1-12 的端口 ADR。它不阻塞 P1A，但阻塞 P2 到 P4 的结构属性任务。

P2 到 P5 覆盖其余类。P6 做产品化验证与验收，与 P2 到 P5 并行。P2 到 P4 的迁移任务依赖 `docs/plans/2026-11-21-terminal-common-components-plan.md` 里的 `edge_acq`、`edge_event`、`edge_time`、`edge_control`。这些组件没合并前，迁移任务保持 blocked。本产品不重写第二套调度、事件与控制组件。

首版不做的是“实现等级为 optional 的类方法”，不是“类本身不建模”。Class 2、12、17 的属性进对象表，方法按设计文档返回 DAR=5，等项目规范确认后再开。这样主站永远能看到对象存在，而不是对象未定义。

## 2. 目标与范围

### 2.1 产品目标

DCU 是台区配电智能融合终端，对下通过 RS485、载波或微功率无线采集电表，对上作为 DL/T 698.45 服务端接受配网主站的 GET、SET、ACTION 与 Record 请求，并主动上报事件。它同时承担台区线损、相位识别、停电上报、负荷控制与本地边缘计算。

### 2.2 本计划的范围

- `cmd/dcu` 的生产行为、698 对象模型、北向服务路径、冻结与事件、控制与安全。
- 26 个接口类的属性、方法与 OI 覆盖。
- `cmd/dcu` 的单元测试、验收任务、证据与文档。
- 与 `modules/edge_acq`、`modules/edge_event`、`modules/edge_time`、`modules/edge_control` 的端口对接。组件本身由公共组件计划负责。

### 2.3 范围边界

| 边界 | 说明 |
| --- | --- |
| 对象模型全量建模 | 26 个类全部进对象表，513 个 OI 全部进目录 |
| 实现等级分档 | `full`、`read`、`control`、`optional` 四档，见设计文档第 6 节 |
| 不猜报文 | 参数结构与编码以 `docs/698.pdf` 7.3、7.4、附录 A、附录 H 为准，`docs/698_parsed.md` 只作检索 |
| 不重写公共组件 | 调度、事件、时间、控制由公共组件计划负责，DCU 只做消费者 |
| 不做实时现场总线 | EtherCAT、Profinet、EtherNet/IP CIP Motion 不在范围 |
| 不做 IEC 61850 | 变电场景不在 DCU 范围 |
| 不做五防与操作票 | 首版只做选择执行、闭锁与审计 |

## 3. 现状与证据

### 3.1 代码盘点

| 文件 | 职责 | 现状 |
| --- | --- | --- |
| `cmd/dcu/main.c` | 组合根与 libuv 装配 | 可运行，stdout 用 `_IONBF`，配置只走环境变量 |
| `cmd/dcu/dcu_node.{h,c}` | 聚合根，持有档案、轮询、存储、事件对象 | 451 行，装配完整 |
| `cmd/dcu/dcu_inventory.{h,c}` | 档案增删查、通道协议、DB 加载 | 有测试 `test_dcu_inventory` |
| `cmd/dcu/dcu_poller.{h,c}` | 通道级串行链、限流、熔断、任务与方案 | 有 8 个测试 |
| `cmd/dcu/dcu_storage.{h,c}` | SQLite WAL、缓存合并、冻结查询 | 有测试 `test_dcu_storage` |
| `cmd/dcu/dcu_router.{h,c}` | 698 对象字典与属性读写 | 664 行，注册 12 个 OI |
| `cmd/dcu/dcu_north_adapter.c` | 698 从站装配与产品取数端口 | 只解析 4 个 OI，ACTION 恒失败 |
| `cmd/dcu/dcu_north_source.c` | 推送型北向的 metric 名与值 | 只覆盖 16 个实时 OAD |
| `cmd/dcu/dcu_driver_adapter.{h,c}` | OAD 与 DLT645、698、Modbus 的映射与量纲 | 三套硬编码点表 |
| `cmd/dcu/dcu_event_monitor.c` | 事件检测与冻结管线 | 3000 与 3005 两个事件 |
| `cmd/dcu/dcu_soe_event_engine.c` | SOE 环形缓冲 | 有测试 |
| `cmd/dcu/dcu_outage_detector.c` | 停电捕获与 last gasp | 有测试 |
| `cmd/dcu/dcu_dlt698_mgmt_engine.c` | DFU、费率、会话鉴权 | 有测试 |
| `cmd/dcu/dcu_heartbeat.c` | 周期刷盘与演示触发 | 混入演示行为 |

测试证据：`tests/CMakeLists.txt` 注册了 18 个 DCU 测试文件，包括 `test_dcu_poller`、`test_dcu_router`、`test_dcu_storage`、`test_dcu_node`、`test_dcu_business_engines`、`test_dcu_phase1_delivery`、`test_dcu_scale_benchmark` 与 `test_dlt698_conformance_suite`。验收证据：`acceptance/dcu/tasks/dcu_acceptance.yaml` 验南向采集，`acceptance/dcu/tasks/dcu_northbound.yaml` 验北向从站。

### 3.2 已确认的缺陷与漂移

| 编号 | 问题 | 证据 |
| --- | --- | --- |
| D-01 | 生产路径每 5 秒触发一次日冻结，每 200 块表随机拉合闸一次，默认跑 3 个周期后自杀 | `cmd/dcu/dcu_heartbeat.c:20`、`:43`、`:50`、`:71`、`:97`、`:109` |
| D-02 | 北向取数只支持 OI 2000、2001、2004、6000，其余 OI 一律不应答 | `cmd/dcu/dcu_north_adapter.c:41`、`:48`、`:56` |
| D-03 | 北向 ACTION 对所有 OMD 返回失败，主站看到 DAR=2 | `cmd/dcu/dcu_north_adapter.c:74` |
| D-04 | 对象字典只有 12 个 OI，Class 10 只有 6000、6012、6002、6003 | `cmd/dcu/dcu_router.c:512` 到 `:610` |
| D-05 | Class 9 只有 5004 只读，没有 5000 到 5011，也没有冻结方法 | `cmd/dcu/dcu_router.c:217` |
| D-06 | 产品 stdout 用 `_IONBF`，仓库规则要求 `_IOLBF`，且守卫只扫 iEMS | `cmd/dcu/main.c:80`、`ci/scripts/check_iems_stdout_line_buffered.py` |
| D-07 | `cmd/dcu/README.md` 仍写 4 文件架构，实际是 29 个 C 文件 | `cmd/dcu/README.md` 第 5 节 |
| D-08 | `docs/design/NORTHBOUND_ARCHITECTURE.md` 4.4 节仍写“DCU 无北向从站、OI 8000 返回 -4”，与当前代码不一致 | `docs/design/NORTHBOUND_ARCHITECTURE.md` 4.4 |
| D-09 | 所谓 698 一致性套件只断言常量，不解析真实帧 | `tests/test_dlt698_conformance_suite.c` |
| D-10 | 采集规则按协议写死三套点表，没有 601E/601F 的动态规则 | `cmd/dcu/dcu_driver_adapter.c` 第 2 到 4 节 |
| D-11 | 验收只覆盖南向抄读与少量北向读，不覆盖写、动作、记录、上报、冻结、事件与重启 | `acceptance/dcu/` |
| D-12 | `docs/698_parsed.md` 有 79 行 OI 的 IC 列与范围推断不一致。这是 OCR 列错位，标准本身明确 | `docs/design/DCU_698_OI_CATALOG.md` 首部 |

### 3.3 现状判定

南向采集与本地存储可用，轮询链、熔断、通道独立推进都有测试。698 对象模型是“有骨架、缺器官”。北向服务端“能连上、能读四个 OI、不能写、不能动作、不能查记录”。生产运行“能跑，但会自己触发冻结和跳闸，还会自己退出”。P0 与 P1 先把这三件事纠正，P2 到 P5 把 26 个类补齐，P6 把测试、验证与验收补齐。

## 4. 架构原则与本计划的遵守方式

以下原则来自 `AGENTS.md`、`docs/design/DOMAIN_CONTEXT_MAP.md`、`docs/design/NORTHBOUND_ARCHITECTURE.md`、`docs/design/slave_plane_architecture.md` 与 ADR-0007、0010、0011。

| 原则 | 本计划怎么遵守 |
| --- | --- |
| 依赖方向是 `cmd` 依赖 `modules` 与 `adapters`，`modules` 只依赖 `foundation` 的端口头 | DCU 只做装配与产品领域。调度、事件、时间、控制不写进 `cmd/dcu` |
| DataHub 归 infra，组件不得 include `datahub.h` | 现有 `cmd/dcu` 是组合根，可以持有 DataHub。新组件一律走端口 |
| 编解码器是 `foundation/src/protocol/` 的纯函数，驱动在 `adapters/io_drivers/` | 698 帧编解码继续在 foundation。产品只做 OAD 与业务映射 |
| 同一协议族的量纲与帧规则只能有一处来源 | D2-06 把三套点表收敛成一张 601F 规则表，并加 DLT645 与 DLT698 对拍测试 |
| 构建期静态表，运行期只读 | 698 对象表、点表、规则表都是 `const` 静态数组，由目录生成或代码生成产出 |
| 组件不用动态内存，调用方分配存储 | 新对象表与端口实现全部用静态数组或调用方提供的缓冲 |
| 每修一个错误，同一次改动加一条守卫并登记 `run_guard_selftest.py` | D0-03 修 stdout 守卫。D0-02 加“生产模式不产生演示冻结与控制”的测试 |
| 完成定义是静态分析、测试、运行时证据三者齐全 | 每个任务卡的出口证据按第 9.4 节打包 |
| spec 经人工评审后才打 ready 标签 | 本文件是草案。评审通过后，第 8 节的任务才建 issue 并打 `ready` |

## 5. 目标设计

### 5.1 数据形状

对象模型、采集计划、冻结、事件、控制是五类状态。每一类用一个结构表达，不用散落的全局变量。完整定义在 `docs/design/DCU_698_OBJECT_MODEL.md` 第 4 节。

**698 对象表。** 一行描述一个 OI 实例。`scope` 区分共享对象、按表对象与单例对象。读写与方法是函数指针，找不到就是不支持。

```c
typedef enum { DCU_OBJ_SHARED, DCU_OBJ_PER_METER, DCU_OBJ_SINGLETON } dcu_obj_scope_t;

typedef struct {
    uint16_t          oi;
    uint8_t           class_id;
    dcu_obj_scope_t   scope;
    dcu_attr_read_fn  read;
    dcu_attr_write_fn write;
    dcu_method_fn     method;
    const void       *instance;
    uint8_t           attr_mask[32];   /* 属性号 0..255 位图 */
    uint8_t           method_mask[32]; /* 方法号 0..255 位图 */
} dcu_698_object_t;
```

**采集计划。** 对应 6013 任务单元与 6015 普通采集方案。`meter_mask` 指向档案槽位，`oads` 是要抄的列。调度器只认这个结构，不认协议。

```c
typedef struct {
    uint32_t plan_id;
    uint32_t period_s;
    uint8_t  scheme_type;
    uint16_t scheme_id;
    uint8_t  priority;
    uint32_t meter_mask[DCU_METER_MASK_WORDS];
    uint8_t  oad_count;
    uint32_t oads[DCU_MAX_PLAN_OADS];
} dcu_acq_plan_t;
```

**冻结记录。** 对应 5000 到 5011。时间戳只有一个来源，就是 OAD 2021 冻结时间。

```c
typedef struct {
    uint32_t freeze_oad;
    uint64_t freeze_ts_ms;
    char     meter_addr[DCU_ADDR_LEN];
    uint8_t  value_count;
    struct { uint32_t src_oad; double value; } values[DCU_MAX_FREEZE_COLS];
} dcu_freeze_record_t;
```

**事件记录。** 对应 Class 7 与 Class 24。发生时间、结束时间、快照列都在一条记录里。

```c
typedef struct {
    uint16_t oi;
    uint8_t  event_id;
    char     meter_addr[DCU_ADDR_LEN];
    uint64_t start_ms;
    uint64_t end_ms;
    uint8_t  snapshot_count;
    struct { uint32_t oad; double value; } snapshot[DCU_MAX_EVENT_SNAPSHOT];
} dcu_event_record_t;
```

**控制操作。** 对应 Class 13。选择与执行是两次状态转移，超时与闭锁是显式状态。

```c
typedef enum { DCU_OP_SELECTED, DCU_OP_EXECUTED, DCU_OP_REJECTED, DCU_OP_EXPIRED } dcu_op_state_t;

typedef struct {
    uint32_t        op_id;
    char            meter_addr[DCU_ADDR_LEN];
    uint8_t         relay_index;
    uint8_t         action;
    dcu_op_state_t  state;
    uint64_t        select_deadline_ms;
    uint8_t         auth_level;
} dcu_control_op_t;
```

### 5.2 对象模型替掉手写分支

现状是 `dcu_router.c` 里逐个 `obj->oi = ...` 注册，`dcu_north_adapter.c` 里再按 OI 写一次 `if`。两处各写一遍，必然漂移。目标是把“哪个 OI 支持什么”收敛到 `g_dcu_698_objects` 一张表。`dcu_router` 变成查表门面，北向适配器变成表驱动的 `resolve` 与 `accept`。

类处理器按接口类分组，不按 OI 分组。新增同类 OI 只加一行表，不写新代码。完整分组见设计文档 4.4 节。

### 5.3 北向五条路径

`bus_orchestrator` 的从站平面负责帧与会话。产品只实现 `edge_slave_source_ops_t`：

| 路径 | 入口 | 落点 |
| --- | --- | --- |
| GET 实时值 | `resolve` | 对象表 `read`，DataHub 影子或聚合根 |
| GET Record | `resolve` 的 Record 分支 | `dcu_storage_query_freeze`，按 RSD 过滤 |
| SET 参数 | `write` | 对象表 `write`，写档案、方案、通信参数 |
| ACTION 控制或冻结 | `accept_action` | 对象表 `method`，控制状态机或冻结引擎 |
| REPORT 主动上报 | 产品事件源 | `northbound_mgr` 推送族或从站主动帧 |

### 5.4 采集计划与 601F 规则

计划归 `dcu_acq_plan_t`。6013 与 6015 的增删改查只改计划。映射归 601F 规则表，规则行是 `(oad, protocol, protocol_id, scale, length)`，DLT645 有主用与备用 DI 两行。一个 OAD 在三协议下的量纲只在这张表定义。

### 5.5 冻结与时间

冻结不是心跳的副产物。触发只有三个来源：时钟到达日冻结或月冻结点、主站 ACTION 8010、本地补冻。时间质量由 `edge_time` 给出，分 TRUSTED、DEGRADED、UNTRUSTED 三级。UNTRUSTED 时冻结照做，但记录打质量标记。零点对齐误差目标小于 50 毫秒。

### 5.6 事件与 SOE

事件分两层。检测层看 DataHub 的影子值，生成 `dcu_event_record_t`。存储层用 `edge_event` 的有界环形缓冲加持久化端口。上报层按 601D 方案选择通道与重试次数。Class 7 与 Class 24 共用存储与上报，但 Class 24 的记录数是四张表的组合结构，不是单个计数。

### 5.7 控制与安全

控制走 `edge_control` 的状态机。选择与执行两阶段，选择有截止时间，执行前校验授权、闭锁与目标表在线。操作记录进审计表。ESAM 会话与 Class 21 鉴权由 `edge_infra_security` 与现有 ESAM 驱动承担，控制模块只认授权结果。Class 20 的对象列表是访问控制的事实来源。

### 5.8 文件职责

目标是在现有文件基础上按类组新增文件，单文件不超过 600 行：

- `cmd/dcu/dcu_698_object_model.{h,c}`，静态对象表与查表。
- `cmd/dcu/dcu_698_var.c`，Class 1 到 6。
- `cmd/dcu/dcu_698_event.c`，Class 7 与 24。
- `cmd/dcu/dcu_698_param.c`，Class 8。
- `cmd/dcu/dcu_698_freeze.c`，Class 9。
- `cmd/dcu/dcu_698_acq.c`，Class 10。
- `cmd/dcu/dcu_698_set.c`，Class 11。
- `cmd/dcu/dcu_698_pulse.c`，Class 12。
- `cmd/dcu/dcu_698_control.c`，Class 13。
- `cmd/dcu/dcu_698_stat.c`，Class 14 到 16。
- `cmd/dcu/dcu_698_display.c`，Class 17。
- `cmd/dcu/dcu_698_file.c`，Class 18。
- `cmd/dcu/dcu_698_devmgr.c`，Class 19。
- `cmd/dcu/dcu_698_appconn.c`，Class 20。
- `cmd/dcu/dcu_698_esam.c`，Class 21。
- `cmd/dcu/dcu_698_io.c`，Class 22。
- `cmd/dcu/dcu_698_group.c`，Class 23。
- `cmd/dcu/dcu_698_comm.c`，Class 25 与 26。
- `cmd/dcu/dcu_698_rules.{h,c}`，601F 规则表与 OAD 换算。

其余文件保持职责，逐步瘦身：

- `dcu_router.c` 只做查表门面，不再注册对象。
- `dcu_north_adapter.c` 只做端口适配，不再写 OI 分支。
- `dcu_driver_adapter.c` 只保留协议元数据登记，点表搬去规则表。
- `dcu_heartbeat.c` 改名或瘦身为 `dcu_runtime_health.c`，只做刷盘与自检。

## 6. 698 覆盖矩阵

26 个类全部进入范围。`full` 表示首版必须实现读写或方法，`read` 表示只读，`control` 表示鉴权后执行，`optional` 表示属性进表、方法可返回 DAR=5。完整属性与方法清单见 `docs/design/DCU_698_OI_CATALOG.md`。

| Class | 类名 | OI 行数 | OI 范围 | 首版等级 | 实现阶段 | 实现文件 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 电能量 | 69 | 0000..0503 | read | P1 | `dcu_698_var.c` |
| 2 | 最大需量 | 82 | 1010..2141 | optional | P5 | `dcu_698_var.c` |
| 3 | 分相变量 | 6 | 2000..200C | read | P1 | `dcu_698_var.c` |
| 4 | 功率 | 7 | 2004..200A | read | P1 | `dcu_698_var.c` |
| 5 | 谐波变量 | 2 | 200D..200E | read | P5 | `dcu_698_var.c` |
| 6 | 数据变量 | 45 | 200F..2506 | read | P1 | `dcu_698_var.c` |
| 7 | 事件对象 | 92 | 3006..3320 | full | P3 | `dcu_698_event.c` |
| 8 | 参数变量 | 68 | 3300..4520 | full | P3 | `dcu_698_param.c` |
| 9 | 冻结数据 | 13 | 5000..5011 | full | P3 | `dcu_698_freeze.c` |
| 10 | 采集监控 | 26 | 6000..6042 | full | P2 | `dcu_698_acq.c` |
| 11 | 集合 | 8 | 7000..7101 | read | P5 | `dcu_698_set.c` |
| 12 | 脉冲计量 | 8 | 2401..2408 | optional | P5 | `dcu_698_pulse.c` |
| 13 | 负荷控制 | 22 | 8000..8110 | control | P4 | `dcu_698_control.c` |
| 14 | 区间统计 | 5 | 2100..2104 | read | P5 | `dcu_698_stat.c` |
| 15 | 累加平均 | 5 | 2110..2114 | read | P5 | `dcu_698_stat.c` |
| 16 | 极值工具 | 5 | 2120..2124 | read | P5 | `dcu_698_stat.c` |
| 17 | 显示 | 2 | F300..F301 | optional | P5 | `dcu_698_display.c` |
| 18 | 文件传输 | 3 | F000..F002 | full | P4 | `dcu_698_file.c` |
| 19 | 设备管理 | 4 | 4300..4309 | full | P4 | `dcu_698_devmgr.c` |
| 20 | 应用连接 | 1 | 4400 | full | P4 | `dcu_698_appconn.c` |
| 21 | ESAM | 1 | F101 | full | P4 | `dcu_698_esam.c` |
| 22 | 输入输出设备 | 14 | F200..F210 | read | P4 | `dcu_698_io.c` |
| 23 | 总加组 | 8 | 2301..2308 | full | P4 | `dcu_698_group.c` |
| 24 | 分项事件 | 7 | 3000..300B | full | P3 | `dcu_698_event.c` |
| 25 | 无线公网通信 | 2 | 4500..4501 | full | P4 | `dcu_698_comm.c` |
| 26 | 以太网通信 | 8 | 4510..4517 | full | P4 | `dcu_698_comm.c` |

OI 行数来自当前目录，含 OCR 噪声。D1-00 与 D1-01 先核对并冻结目录，后续任务以冻结后的计数为准。

### 6.1 类到任务的追踪

每个类至少有一个实现任务、一个单元测试、一个验收判据。下表是追踪矩阵，调度器按它判断“这个类是否有人做”。

| Class | 实现任务 | 单元测试 | 验收判据 |
| --- | --- | --- | --- |
| 1 | D1-04、D3-02 | `test_dcu_698_var` | `dcu-northbound` GET 0010 |
| 2 | D5-01 | `test_dcu_698_var` | GET 1010 |
| 3 | D1-04 | `test_dcu_698_var` | GET 2000、2001 |
| 4 | D1-04 | `test_dcu_698_var` | GET 2004 |
| 5 | D5-02 | `test_dcu_698_var` | GET 200D |
| 6 | D1-04 | `test_dcu_698_var` | GET 200F |
| 7 | D3-05 | `test_dcu_698_event` | GET Record 3001 |
| 8 | D3-06 | `test_dcu_698_param` | SET 4000 后回读 |
| 9 | D3-02 | `test_dcu_698_freeze` | GET Record 5004 |
| 10 | D2-02 到 D2-08 | `test_dcu_698_acq` | 6000 加表后能抄到 |
| 11 | D5-03 | `test_dcu_698_set` | GET 7000 |
| 12 | D5-04 | `test_dcu_698_pulse` | GET 2401 |
| 13 | D4-02、D4-03 | `test_dcu_698_control` | ACTION 8000 |
| 14 | D5-05 | `test_dcu_698_stat` | GET 2100 |
| 15 | D5-05 | `test_dcu_698_stat` | GET 2110 |
| 16 | D5-05 | `test_dcu_698_stat` | GET 2120 |
| 17 | D5-06 | `test_dcu_698_display` | GET F300，方法 DAR=5 |
| 18 | D4-06 | `test_dcu_698_file` | 分块上传与校验 |
| 19 | D4-04 | `test_dcu_698_devmgr` | 对时与重启 |
| 20 | D4-05 | `test_dcu_698_appconn` | 未授权对象 DAR=3 |
| 21 | D4-07 | `test_dcu_698_esam` | 安全会话 |
| 22 | D4-08 | `test_dcu_698_io` | GET F201 |
| 23 | D4-09 | `test_dcu_698_group` | GET 2301 |
| 24 | D3-05 | `test_dcu_698_event` | GET Record 3000 |
| 25 | D4-10 | `test_dcu_698_comm` | SET 4500 后回读 |
| 26 | D4-10 | `test_dcu_698_comm` | SET 4510 后回读 |

## 7. 任务与验证总纲

### 7.1 任务估算汇总

| 阶段 | 内容 | 人日 | 周期 |
| --- | --- | --- | --- |
| P0 | 基线、生产化与文档 | 6.5 | 第 1 周 |
| P1 | 目录、对象模型、北向四类请求、协议栈缺口 | 39 | 第 1 到 8 周 |
| P2 | Class 10 采集监控与调度迁移 | 20 | 第 9 到 11 周 |
| P3 | Class 9、7、24、8 与时间、上报 | 18 | 第 12 到 14 周 |
| P4 | Class 13、23、19、20、21、18、22、25、26 | 24 | 第 15 到 18 周 |
| P5 | Class 2、5、11、12、14、15、16、17 与文档 | 7.5 | 第 19 周 |
| P6 | 产品化验证与验收 | 22.5 | 第 5 到 20 周，与 P1B 到 P5 并行 |
| 合计 | 26 个类全覆盖，测试、验证、验收齐全 | 137.5 | 20 周 |

P1 分两段。P1A 是目录、对象模型与标量北向，23 人日，可立即开工。P1B 是 B1 到 B5 的协议栈缺口，16 人日，依赖 D1-12 的端口 ADR。两人并行可把 20 周压到约 14 周。

### 7.2 验证层次

产品化不是“编译通过加几个单元测试”。每一层都有明确的命令、判据和证据，缺一层就不算完成。

| 层 | 内容 | 工具与命令 | 失败意味着什么 |
| --- | --- | --- | --- |
| L0 静态 | 编译告警、静态分析、架构边界、公开结构体、动态内存、栈用量、资源纪律 | `check_build_warnings.py`、`run_fanalyzer.py`、`check_component_boundary.py`、`check_plane_isolation.py`、`check_public_structs.py`、`check_no_dynamic_memory.py`、`check_stack_usage.py`、`check_resource_discipline.py` | 代码不能进主线 |
| L1 单元 | 每个类、每个处理器、每个方法、每个 DAR 分支 | `tests/test_dcu_*.c`、`tests/test_dcu_698_*.c`、`ctest -R dcu` | 行为不确定 |
| L2 集成 | 南向抄读、DataHub、存储、事件、控制、冻结串起来 | `tests/test_dcu_integration.c`、新增 `test_dcu_698_integration` | 单点通过但链路不通 |
| L3 协议一致性 | 真实帧向量的 GET、SET、ACTION、Record、Report、RSD、异常应答 | `tests/test_dlt698_*.c`、`tests/test_dcu_698_catalog.c` | 主站解错值或超时 |
| L4 验收 | 产品壳加模拟主站、模拟电表，按场景判定 | `./test.sh --omnis dcu`、`acceptance/dcu/` | 用户视角不成立 |
| L5 运行时 | ASAN、UBSAN、TSAN、掉电、长稳、资源泄漏 | `./test.sh --asan`、`./test.sh --ubsan`、`TSAN_OPTIONS`、掉电夹具、24h 与 72h soak | 现场会崩或会漏 |
| L6 性能 | 4096 表规模、P99、平均延迟、写放大 | `test_dcu_scale_benchmark`、`EDGE_PERF_GATE=1` | 现场抄读超时 |
| L7 安全 | 未授权读写、重放、MAC 失败、Class 20 ACL、控制闭锁 | `test_dcu_698_security`、验收安全场景 | 主站或被攻击者可越权 |
| L8 注释卫生 | 合入前跑 `/no-comments`（comment-sicko），删叙述性、复述代码、过期注释，保留非显然的 why 与标准规则 | `/no-comments`、`git diff --check` | 注释误导、历史噪声、过期符号 |

验证规则只有一句。一个 PR 只有单元、运行时、验收三类证据都齐了才算验过。测试通过不是完成，证据可复现才是完成。

### 7.3 测试环境与数据

| 环境 | 组成 | 用途 |
| --- | --- | --- |
| 单元 | `build/` 下的 cmocka 测试 | 处理器、状态机、编解码、DAR |
| 南向台架 | `acceptance/dcu/dcu_product.py`、`sim_meter_pro.py`、`/tmp/rs485_22..31.sock` 的 DLT645 从站 | 40 表采集、离线、超时、控制帧 |
| 北向台架 | `DCU_NORTH_SOCK` 的 698 从站、模拟主站脚本 | GET、SET、ACTION、Record、Report |
| 存储 | `EDGEC_DATA_DIR` 指向的临时目录与 SQLite | 冻结、档案、事件、掉电恢复 |
| 规模 | 40 表验收，4096 表基准 | 轮询延迟与内存 |
| 故障注入 | 环境变量或测试钩子控制离线、超时、队列满、DB 损坏、时钟跳变 | 负路径与恢复 |
| 帧向量 | `docs/698.pdf` 附录 H 与 7.3 的字节，`docs/698_parsed.md` 只作检索 | 协议一致性 |
| 目录 | `docs/design/DCU_698_OI_CATALOG.json` | 生成对象表测试用例 |

本计划以模拟电表与模拟主站为主。实机台架是发布前的独立验收，不在本计划的自动化范围，需要项目排期。模拟台架通过不等于实机通过，这一条写进 11.2 的风险。

测试数据不手抄。26 类的属性与方法用例从目录 JSON 生成，生成物是 `tests/dcu_698_catalog_vectors.h`。目录变，用例跟着变。手改生成物即失败。

### 7.4 26 类测试矩阵

每个类至少四类用例：正常值、边界值、未定义或越权、目标离线或资源不足。下表是验收口径，具体用例名在测试文件里。

| Class | 正常 | 边界 | 负路径 | 验收判据 |
| --- | --- | --- | --- | --- |
| 1 电能量 | GET 0010 | 高精度 long64 | 未知 OAD DAR=5 | S2 |
| 2 最大需量 | GET 1010 | 发生时间边界 | 无数据 DAR=2 | S2 |
| 3 分相变量 | GET 2000/2001 | 单相接线 | 缺相 DAR=2 | S2 |
| 4 功率 | GET 2004 | 负功率 | 类型不匹配 DAR=5 | S2 |
| 5 谐波 | GET 200D | 2 次与 31 次 | 未计算 DAR=2 | S2 |
| 6 数据变量 | GET 200F | 状态字全 1 | 未采集 DAR=2 | S2 |
| 7 事件对象 | GET 3006 | 记录表满 | 删除不存在属性 DAR=5 | S9 |
| 8 参数变量 | SET 4000 后回读 | 校时模式 255 | 越权 DAR=3 | S4 |
| 9 冻结 | GET Record 5004 | 时间范围空 | RSD 列不存在 DAR=5 | S6、S7 |
| 10 采集监控 | 6000 加表后抄到 | 档案满 | 重复地址 DAR=5 | S1、S11、S12 |
| 11 集合 | GET 7000 | 空集合 | 元素 OAD 未定义 DAR=5 | S2 |
| 12 脉冲计量 | GET 2401 | 脉冲满 | 方法 DAR=5 | S2 |
| 13 负荷控制 | ACTION 8000 跳闸 | 选择超时 | 未选择或越权 DAR=3 | S5、S14 |
| 14 区间统计 | GET 2100 | 区间跨日 | 无统计 DAR=2 | S2 |
| 15 累加平均 | GET 2110 | 除零保护 | 无数据 DAR=2 | S2 |
| 16 极值 | GET 2120 | 极值相等 | 无数据 DAR=2 | S2 |
| 17 显示 | GET F300 | 空轮显表 | 方法 DAR=5 | S2 |
| 18 文件传输 | 分块上传与校验 | 最后一块短包 | 校验失败回滚 | S15 |
| 19 设备管理 | ACTION 对时 | 恢复出厂保留 OAD | 未二次确认拒绝 | S4、S16 |
| 20 应用连接 | 授权对象可读 | 权限边界 | 未授权 DAR=3 | S14 |
| 21 ESAM | 安全会话 | 会话过期 | MAC 失败 DAR=3 | S15 |
| 22 输入输出设备 | GET F201 | 配置边界 | 写只读 DAR=3 | S2、S4 |
| 23 总加组 | GET 2301 | 8 组满配 | 组不存在 DAR=5 | S14 |
| 24 分项事件 | GET Record 3000 | 四张记录表 | 记录表满 | S9 |
| 25 无线公网 | SET 4500 后回读 | 信号强度 0 | 写只读 DAR=3 | S4 |
| 26 以太网 | SET 4510 后回读 | IP 边界 | MAC 只读 | S4 |

### 7.5 验收场景矩阵

验收场景是产品化判据，不是单元测试的别名。每个场景在 `acceptance/dcu/` 里有一条独立判据，失败能归因到场景。

| 编号 | 场景 | 判据 | 证据 |
| --- | --- | --- | --- |
| S1 | 南向 40 表采集 | 每表至少一次成功抄读，离线表不拖累在线表 | `runtime.log` 与 bus.json |
| S2 | 北向 GET 单 OAD 与 NormalList | 26 类首批 OI 全部返回正确类型与值 | 主站抓包 |
| S3 | 北向 GET 未定义 OAD | 返回 DAR=5，不超时 | 主站抓包 |
| S4 | 北向 SET 参数 | 写入后回读一致，持久化跨重启 | 抓包与 DB 查询 |
| S5 | 北向 ACTION 跳合闸 | 选择后执行，继电器帧真的出门 | 抓包与控制帧日志 |
| S6 | 北向 GET Record 与 RSD | 时间范围与列选择正确，空范围返回空记录 | 抓包 |
| S7 | 日冻结零点对齐 | 冻结时间戳与源 OAD 一致，误差小于 50 毫秒 | 模拟时钟日志 |
| S8 | 月冻结与结算日 | 5005、5006 生成正确 | DB 查询 |
| S9 | 事件记录与 SOE | 顺序、时标、快照一致，重启不丢 | 事件表与抓包 |
| S10 | 停电 3011 主动上报 | 断电后主站收到事件 | 抓包 |
| S11 | 档案动态增删 | 加表后能抄，删表后不再抄 | 抓包与 DB |
| S12 | 任务与方案变更 | 改周期与列选择立即生效 | 抓包 |
| S13 | 上报方案与 SnF | 断线缓存，恢复后按序补齐 | 抓包与 SnF 表 |
| S14 | 控制选择执行与闭锁 | 未选择、越权、闭锁均拒绝 | 抓包与审计表 |
| S15 | ESAM 安全会话与文件传输 | 带 MAC 请求成功，校验失败回滚 | 抓包与版本日志 |
| S16 | 掉电恢复 | 数据库可打开，最后断面不丢 | 掉电夹具日志 |
| S17 | 4096 表规模 | P99 与平均延迟在预算内 | 基准报告 |
| S18 | 24 小时长稳 | 内存、fd、Flash 写放大无异常增长 | soak 日志 |

### 7.6 门禁与阶段判据

| 阶段 | 必须通过 | 证据 |
| --- | --- | --- |
| P0 | 构建、`ctest -R dcu`、目录 `--check`、stdout 守卫 | `docs/evidence/DCU-P0/` |
| P1 | `ctest -R 'dcu|dlt698'`、对象表一致性、北向 S2 到 S6、A-XDR 结构属性、SET、Record、Report、Class 20 | `docs/evidence/DCU-P1/` |
| P2 | 南向 S1、档案 S11、任务 S12、601F 对拍 | `docs/evidence/DCU-P2/` |
| P3 | 冻结 S7、S8、事件 S9、S10、上报 S13 | `docs/evidence/DCU-P3/` |
| P4 | 控制 S5、S14、安全 S15、通信 S4 | `docs/evidence/DCU-P4/` |
| P5 | 剩余类 S2、S4、文档同步 | `docs/evidence/DCU-P5/` |
| P6 | S1 到 S18 全绿、ASAN/UBSAN/TSAN、覆盖率、性能、长稳 | `docs/evidence/DCU-P6/` |

### 7.7 发布判据

发布前逐条勾选，缺一条不发。

1. 26 个类全部进对象表，513 个 OI 全部进目录，79 行 OCR 冲突已按 `docs/698.pdf` 重新抽取并归零。
2. GET、SET、ACTION、Record、Report 五条路径各有验收抓包。结构属性走 A-XDR 通道，标量回落路径不变。
3. 从站端口扩展有 ADR，IEC104 与 Modbus 的标量路径回归通过。
4. 每个类至少一条单元用例与一条验收判据，矩阵见 7.4 与 7.5。
5. ASAN、UBSAN、TSAN 干净，`run_fanalyzer.py` 无新报告。
6. 覆盖率不低于目录下限，且只升不降。
7. 4096 表规模 P99 在预算内，24 小时长稳无泄漏。
8. 掉电恢复、数据库损坏恢复、时钟跳变三类故障有证据。
9. 未授权读写、重放、MAC 失败、控制闭锁四类安全用例通过。
10. 文档与代码一致，`README`、`CONTEXT.md`、`NORTHBOUND_ARCHITECTURE.md`、`PUBLIC_STRUCT_AUDIT.md` 已同步。
11. 发布报告在 `docs/evidence/DCU-P6/summary.md`，含每条判据的命令与结果。
12. 所有合入 PR 都有 `/no-comments` 记录，注释噪声与过期符号已清理。

## 8. 分阶段实施计划

角色：A 架构，C C 开发，D 驱动或协议，Q 验证。

### P0 基线、生产化与文档（6.5 人日，第 1 周，可立即开工）

| ID | 类型 | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| D0-01 | docs | 重写 `cmd/dcu/README.md`，目录与函数原型改为链接到设计文档与 `CONTEXT.md`，保留 698 业务背景 | A | 1 | 无 | README 与代码一致 | 文档评审通过，目录清单与 `CMakeLists.txt` 逐项对上 |
| D0-02 | defect | 把演示行为移出生产路径。冻结、随机跳闸、自动退出只在 `DCU_DEMO_MODE=1` 下启用。默认持续运行 | C | 1.5 | 无 | `dcu_heartbeat.c` 改动与新测试 | `test_dcu_runtime_health` 证明默认 3 个周期不冻结、不跳闸、不退出 |
| D0-03 | defect | `cmd/dcu/main.c` 改 `_IOLBF`。把 stdout 守卫从只扫 iEMS 扩到全部产品，并在 `run_guard_selftest.py` 登记正反夹具 | C | 0.5 | 无 | 守卫脚本与夹具 | 故意改回 `_IONBF` 时守卫变红，改回后变绿 |
| D0-04 | test | 记录基线：构建、`ctest -R dcu` 与 `ctest -R dlt698`、ASAN、UBSAN、`./test.sh --omnis dcu`、覆盖率 | Q | 1 | 无 | `docs/evidence/DCU-P0/` | 日志与 SHA 齐全，失败项逐条登记 |
| D0-05 | test | 用真实帧向量替换 `test_dlt698_conformance_suite.c` 的常量断言，向量从 `docs/698.pdf` 摘 | D | 1 | 无 | 新帧向量测试 | 每条向量有 PDF 页码与附录 H 表号，正反例都跑 |
| D0-05a | defect | 698 解码补 HCS 校验（5.1.5）；把 `test_reject_bad_hcs` 从 skip 改为断言 -1 | D | 1 | D0-05 | codec 修复与测试 | 坏 HCS 被拒；21 例 20 通过，仅剩 Record/RSD skip |
| D0-05b | defect | 对齐 DAR 4/5/6/7：适配器区分 OI 未定义、属性/方法不支持、实例不存在、类型不匹配 | D | 1 | D0-05 | driver/adapter 修复与测试 | 4/5/6/7 各一例，不再是统一 6 |
| D0-06 | docs | 更新 `docs/design/NORTHBOUND_ARCHITECTURE.md` 4.4 节与 `cmd/dcu/CONTEXT.md` 的状态，标注已实现与未实现 | A | 0.5 | D0-01 | 文档状态与代码一致 | 评审通过，文中每条结论有代码或测试引用 |
| D0-07 | test | 把目录生成加入 CI，`--check` 失败即红；生成物进 `docs/design/` | C | 1 | D1-00 | CI 步骤 | 故意改一句原文后 `--check` 变红 |

### P1 目录、对象模型、北向四类请求与协议栈缺口（39 人日，第 1 到 8 周，P1A 可立即开工，P1B 依赖 D1-12）

| ID | 类型 | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| D1-00 | feature | 对象目录生成器落地。核对 26 个类与 513 行 OI，输出 `DCU_698_OI_CATALOG.md` 与 `.json`，提供 `--check` | D | 2 | 无 | `scripts/extract_dcu_698_catalog.py` 与生成物 | 26 类齐全，`--check` 通过，故意改原文会红 |
| D1-01 | docs | 以 `docs/698.pdf` 为准重新抽取 79 行 IC 列错位，修正生成器与 override 表，写清每个 OI 的最终类。标准没有歧义，这是解析错误 | A+D | 2 | D1-00 | 核对表与设计文档更新 | 每个冲突行有 PDF 页码或附录 A 表号，重新抽取后冲突归零 |
| D1-02 | docs | 写 `docs/design/DCU_698_OBJECT_MODEL.md`，定对象表结构、类处理器、DAR、访问控制、存储映射 | A | 2 | D1-01 | 设计文档 | 评审通过，26 个类都有等级与落点 |
| D1-03 | feature | 新建 `dcu_698_object_model.{h,c}`，把 `dcu_router.c` 的 12 个 OI 迁进静态表。`dcu_router` 改查表门面。属性/方法掩码用 256 位位图，覆盖方法号 127 到 134，门面不绕过 mask | C | 3 | D1-02 | 新模块与迁移后的 router | `test_dcu_router` 全绿，新增 `test_dcu_698_object_model` 覆盖查表、缺失 OI、方法 127/128、属性 18、掩码拒绝 |
| D1-04 | feature | Class 1、3、4、6 的读取处理器，接 DataHub 影子与 `dcu_value_t` 类型标签 | C | 2 | D1-03 | `dcu_698_var.c` | 单元测试覆盖正常值、缺 OI、类型标签、质量码 |
| D1-05 | feature | 北向 `resolve` 改表驱动，支持首批 Class 1、3、4、6、9、10 的标量属性；复杂属性走 D1-13 的 A-XDR 通道 | C | 2 | D1-04、D1-13 | `dcu_north_adapter.c` 改动 | 验收 `dcu-northbound` 读 2000、2001、2004、6000、6012、5004 全通过 |
| D1-06 | feature | 北向 ACTION 与 SET 接对象表方法。先做 OI 8000 跳合闸与 8010 强制冻结，参数按标准表 A.9 解 | D | 3 | D1-03、D1-13、D1-14 | 方法处理器与 DAR 映射 | 单元测试覆盖合法参数、缺参、越权、目标离线；验收发送真实 ACTION 帧并收到正确应答 |
| D1-07 | feature | GET Record 接 `dcu_storage_query_freeze`，解析 RSD 的开始时间、结束时间与列选择 | C | 2 | D1-03、D1-13、D1-15 | Record 查询路径 | 验收按时间范围查 5004，返回列与 RSD 一致 |
| D1-08 | feature | GET NormalList 支持一次读多个 OAD，按对象表逐个取，任一失败按 DAR 返回 | D | 1.5 | D1-05 | 多 OAD 编码 | 单元测试覆盖全成功、部分失败、空列表 |
| D1-09 | refactor | `dcu_north_source.c` 的名字表从对象表派生，删掉重复 OAD 常量表 | C | 1 | D1-03 | 单一名字来源 | `test_dcu_north_source` 全绿，名字表与对象表对拍 |
| D1-10 | test | 对象表一致性测试，表里每个 OI 的类、属性位图、方法位图与目录一致 | Q | 1.5 | D1-03 | `test_dcu_698_catalog` | 目录里每个 OI 都能在表里找到，反之亦然 |
| D1-11 | test | 北向验收扩展到 GET、SET、ACTION、Record 四类，每条判据独立 | Q | 1 | D1-06、D1-07 | `acceptance/dcu/tasks/dcu_northbound.yaml` | `./test.sh --omnis dcu` 通过，失败可归因到单条判据 |
| D1-12 | docs | 写 ADR 与设计文档，定从站 A-XDR 端口 `resolve_axdr` 与 `accept_axdr` 的签名、缓冲上限、向后兼容与标量回落 | A | 2 | D1-02 | `docs/adr/0013-698-slave-axdr-port.md`、设计文档 4.5 节 | 评审通过，IEC104 与 Modbus 标量路径不变 |
| D1-13 | feature | 扩展 `edge_slave_source_ops_t`，实现 `resolve_axdr` 与 `accept_axdr`，批量属性与结构属性走调用方缓冲 | C | 3 | D1-12、D1-03 | `foundation/include/edge_slave_plane.h`、`cmd/dcu/dcu_north_adapter.c` | 标量回落用例通过，结构属性用例通过，无动态内存 |
| D1-14 | feature | 698 从站补 SET-Request 分支，解 A-XDR 参数，调 `accept_axdr`，按标准回 DAR | D | 2 | D1-12 | `adapters/io_drivers/src/driver_dlt698.c` | 合法写、缺参、错类型、越权四类用例通过 |
| D1-15 | feature | 698 编解码与从站补 GetRecord 与 RSD，支持时间范围与列选择，超长记录分页 | D | 4 | D1-12、D1-13 | `foundation/src/protocol/dlt698_codec.c`、`driver_dlt698.c` | 5004 按时间范围与列选择查询通过，空范围返回空记录 |
| D1-16 | feature | 补 REPORT.notification 端口回调、驱动编码与产品上报源 | D | 2 | D1-12、D1-13 | `edge_slave_plane.h`、`driver_dlt698.c`、`cmd/dcu/dcu_event_monitor.c` | 主动上报用例通过，断链后 SnF 补齐 |
| D1-17 | feature | 从站交换补跨帧会话状态与 Class 20 对象列表查询，支持应用连接与 ACL | D | 3 | D1-12 | `driver_dlt698.c`、`cmd/dcu/dcu_north_adapter.c` | 未授权 OAD 返回 DAR=3，授权后成功 |

### P2 Class 10 采集监控与调度迁移（20 人日，第 9 到 11 周，依赖 COM-C1-01）

前提：`docs/plans/2026-11-21-terminal-common-components-plan.md` 的 `COM-C1-01` 已合并，且 D1-13 的 A-XDR 端口已落地。未合并时本阶段全部 `blocked`，不得在 `cmd/dcu` 内另写调度器。

| ID | 类型 | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| D2-01 | refactor | `dcu_poller` 迁移到 `edge_acq`，删除重复调度循环，保持通道串行与熔断行为 | C | 3 | COM-C1-01 | 迁移后的 DCU 装配 | DCU 全部单元测试与验收绿，`dcu_poller.c` 行数下降，无第二套调度 |
| D2-02 | feature | Class 10 6000 与 6001 的 Add、AddBatch、Update、Delete、Clear 方法与档案持久化 | C | 3 | D2-01 | `dcu_698_acq.c` 档案方法 | 验收动态加表后能抄到，删表后不再抄 |
| D2-03 | feature | 6012 与 6013 任务增删改查，映射到 `dcu_acq_plan_t` | C | 2 | D2-01 | 任务方法处理器 | 验收改周期后轮询间隔变化 |
| D2-04 | feature | 6014 与 6015 普通采集方案，电表集合与 CSD 列选择映射到计划 | C | 2 | D2-03 | 方案处理器 | 验收只抄指定列，未选列不出现 |
| D2-05 | feature | 6016 与 6017 事件采集方案，6018 与 6019 透明方案，601A 与 601B 结果 | C | 2 | D2-03 | 事件与透明方案处理器 | 验收事件上送与透明转发各一例 |
| D2-06 | feature | 601C 与 601D 上报方案，配置通道、超时、重试与数据列表，接 SnF | C | 2 | D2-03、D1-16 | 上报方案处理器 | 模拟主站断开后恢复，数据按序补齐 |
| D2-07 | feature | 601E 与 601F 规则表，替换三套硬编码点表，支持主用与备用 DI | C | 3 | D2-01 | `dcu_698_rules.{h,c}` | 同一 OAD 在 DLT645 与 DLT698 的量纲对拍通过，备用 DI 回退用例通过 |
| D2-08 | feature | 6032 到 6035 采集状态与任务监控，6040 到 6042 三个时标 | C | 2 | D2-03 | 监控对象 | 验收读任务状态，成功数与失败数与实际一致 |
| D2-09 | feature | 6002 与 6003 搜表接入对象模型，搜表结果可召测 | D | 1 | D2-02 | 搜表路径 | 验收启动搜表并读结果 |

### P3 Class 9、7、24、8 与时间、上报（18 人日，第 12 到 14 周，依赖 COM-C2）

前提：`COM-C2-01` 的 `edge_time`、`COM-C2-02` 的 `edge_event`、D1-13 的 A-XDR 端口与 D1-15 的 Record 路径已合并。未合并时本阶段 `blocked`。

| ID | 类型 | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| D3-01 | refactor | 用 `edge_time` 替换心跳里的时间与冻结触发，删掉演示冻结 | C | 2 | COM-C2-01、D0-02 | 时间端口接入 | 时间质量分级用例通过，UNTRUSTED 时冻结带质量标记 |
| D3-02 | feature | Class 9 冻结调度器，覆盖 5000 到 5011，支持日、月、曲线冻结，零点对齐误差小于 50 毫秒，支持 ACTION 8010 | C | 3 | D3-01、D1-02 | `dcu_698_freeze.c` | 模拟时钟跨零点，验收读 5004 与 5005，时间戳与源 OAD 一致 |
| D3-03 | feature | Class 9 方法 3、4、5、7、8 与 RSD 查询，关联对象属性表的增删 | C | 2 | D3-02、D1-15 | 冻结方法处理器 | 添加与删除冻结对象属性后，Record 返回列变化 |
| D3-04 | refactor | SOE 迁移到 `edge_event`，有界缓冲、排序、溢出计数、重启不丢 | C | 2 | COM-C2-02 | 事件存储 | 排序、溢出、重启三类用例通过 |
| D3-05 | feature | Class 7 与 Class 24 的属性和方法接对象表，覆盖 3000 到 3320 的事件 OI | C | 3 | D3-04、D1-02、D1-15 | `dcu_698_event.c` | GET、GET Record、添加与删除关联对象属性各一例 |
| D3-06 | feature | Class 8 参数变量，按白名单实现日期时间、通信地址、表号、客户编号、地理位置、互感器变比、限值 | C | 2 | D1-02、D1-14 | `dcu_698_param.c` | SET 后回读一致，越权写返回 DAR=3 |
| D3-07 | feature | 3011 停电与 3106 停上电事件接入事件引擎与主动上报 | D | 2 | D3-05 | 事件对象 | 验收断电后收到主动上报 |
| D3-08 | test | Class 9、7、24、8 的单元测试与验收判据，每类至少四例 | Q | 2 | D3-02 到 D3-06 | 测试文件 | `test_dcu_698_freeze`、`test_dcu_698_event`、`test_dcu_698_param` 全绿 |

### P4 Class 13、23、19、20、21、18、22、25、26（24 人日，第 15 到 18 周，依赖 COM-C3）

前提：`COM-C3-01` 的 `edge_control`、D1-13 的 A-XDR 端口与 D1-17 的 Class 20 会话已合并。未合并时控制相关任务 `blocked`，其余任务按 D1 完成后可开工。

| ID | 类型 | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| D4-01 | refactor | 控制迁移到 `edge_control`，选择、执行、闭锁、超时与审计走统一状态机 | C | 3 | COM-C3-01 | 控制状态机接入 | 全分支单测与验收时序用例通过 |
| D4-02 | feature | Class 13 属性与控制方法，覆盖 8000 到 8110，接本地功控与远程拉合闸 | C | 3 | D4-01 | `dcu_698_control.c` | 验收选择后执行，未选择或越权被拒 |
| D4-03 | feature | Class 13 的控制投入、控制解除、越限告警状态 | C | 1 | D4-02 | 控制状态属性 | 验收投入与解除后状态变化 |
| D4-04 | feature | Class 19 设备管理，重启、数据初始化、恢复出厂、事件初始化、需量初始化 | D | 2 | D4-01 | `dcu_698_devmgr.c` | 验收重启与对时，恢复出厂需二次确认 |
| D4-05 | feature | Class 20 应用连接，对象列表与属性、方法访问权限 | D | 2 | D1-03、D1-17 | `dcu_698_appconn.c` | 未授权对象返回 DAR=3，授权后成功 |
| D4-06 | feature | Class 18 文件传输与 DFU 接入，分块、校验、激活、失败回滚 | D | 2 | D4-05 | `dcu_698_file.c` | 验收分块传输、校验、激活失败回滚 |
| D4-07 | feature | Class 21 ESAM 属性与方法，按项目规范裁剪密钥、证书、钱包 | D | 3 | D4-05 | `dcu_698_esam.c` | 验收建立安全会话并完成一次带 MAC 的请求 |
| D4-08 | feature | Class 22 输入输出设备，F200 到 F210 的接口配置读写 | D | 2 | D1-03 | `dcu_698_io.c` | 验收读写串口、GPIO、载波配置 |
| D4-09 | feature | Class 23 总加组，配置、总加值、滑差、功控与电控轮次 | C | 2 | D4-02 | `dcu_698_group.c` | 验收总加组配置与总加值，和 Class 13 联动 |
| D4-10 | feature | Class 25 与 26 通信参数，APN、IP、端口、MAC、信号强度 | D | 2 | D1-03 | `dcu_698_comm.c` | 验收读写 APN、IP、端口并持久化 |
| D4-11 | test | Class 13、23、19、20、21、18、22、25、26 的单元测试与验收判据 | Q | 2 | D4-02 到 D4-10 | 测试文件 | 每类至少四例，验收判据独立 |

### P5 Class 2、5、11、12、14、15、16、17 与文档（7.5 人日，第 19 周）

| ID | 类型 | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| D5-01 | feature | Class 2 最大需量只读属性 | C | 1 | D1-04 | `dcu_698_var.c` | GET 1010 返回电表需量 |
| D5-02 | feature | Class 5 谐波只读属性，接 `dcu_power_quality` 的 2 到 31 次结果 | C | 1 | D1-04 | `dcu_698_var.c` | GET 200D 与本地计算一致 |
| D5-03 | feature | Class 11 集合只读属性，7000 与 7010 | C | 1 | D1-03 | `dcu_698_set.c` | GET 7000 返回集合元素 |
| D5-04 | feature | Class 12 脉冲计量，属性只读，方法返回 DAR=5 | C | 1 | D1-03 | `dcu_698_pulse.c` | GET 2401，方法 DAR=5 |
| D5-05 | feature | Class 14、15、16 统计、平均、极值只读属性 | C | 2 | D1-04 | `dcu_698_stat.c` | GET 2100、2110、2120 |
| D5-06 | feature | Class 17 显示只读配置，方法返回 DAR=5 | C | 1 | D1-03 | `dcu_698_display.c` | GET F300，方法 DAR=5 |
| D5-07 | docs | 更新 `CONTEXT-MAP.md`、`DOMAIN_CONTEXT_MAP.md`、`cmd/dcu/CONTEXT.md`、`PUBLIC_STRUCT_AUDIT.md` 与本文状态 | A | 0.5 | P1 到 P5 | 文档同步 | 评审通过，文档与代码一致 |

### P6 产品化验证与验收（22.5 人日，第 5 到 20 周，与 P1B 到 P5 并行）

P6 不等到最后才做。每个阶段的代码一落地，对应的验证任务就开始。P6 的最终判据是第 7.7 节的发布判据。

| ID | 类型 | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| D6-01 | test | 全类一致性测试与 26 类测试矩阵，目录驱动生成用例，覆盖正常、边界、负路径 | Q | 3 | D1-03、D1-13、P2 到 P5 | `tests/test_dcu_698_catalog.c`、`tests/dcu_698_catalog_vectors.h` | 每个类至少四类用例，目录行全覆盖 |
| D6-02 | test | 协议一致性套件，真实帧向量覆盖 GET、SET、ACTION、Record、Report、RSD 与 DAR | Q | 3 | D1-06、D1-07、D1-08、D1-14、D1-15、D1-16 | `tests/test_dlt698_*.c` | 每条向量有原文行号，正反例都跑 |
| D6-03 | test | 南向集成与故障注入，40 表、离线、超时、重试、熔断、队列满、畸形帧 | Q | 2 | D2-01 | `acceptance/dcu/tests/test_dcu_acquisition.py` | S1 通过，离线表不拖累在线表 |
| D6-04 | test | 北向五路径验收矩阵 S2 到 S6，GET 单值与列表、SET、ACTION、Record | Q | 3 | D1-11、D1-13 到 D1-17 | `acceptance/dcu/tasks/dcu_northbound.yaml` | 每条场景独立判据，主站抓包留档 |
| D6-05 | test | 冻结、事件、上报验收矩阵 S7 到 S13，含 SnF 断线补齐 | Q | 3 | D3-08、D1-15、D1-16 | `acceptance/dcu/tasks/dcu_freeze_event.yaml` | 每条场景独立判据 |
| D6-06 | test | 控制、安全、掉电验收矩阵 S14 到 S16，含未授权、重放、MAC 失败、闭锁 | Q | 3 | D4-11、D1-17 | `acceptance/dcu/tasks/dcu_control_security.yaml` | 每条场景独立判据，审计表留档 |
| D6-07 | test | 规模、长稳与性能 S17 与 S18，4096 表 P99、24h soak、内存与 fd | Q | 2 | D2-01 | `tests/test_dcu_scale_benchmark.c`、soak 脚本 | 性能门禁通过，长稳无泄漏 |
| D6-08 | test | ASAN、UBSAN、TSAN 全量干净，`run_fanalyzer.py` 无新报告 | Q | 1.5 | P1 到 P5 | 扫描日志 | 三类日志无新告警，误报进基线 |
| D6-09 | test | 覆盖率按目录下限只升不降，更新 `ci/baseline/coverage_floor.txt` | Q | 1 | D6-01 | 覆盖率报告 | `check_coverage.py` 通过，dcu 目录不低于新下限 |
| D6-10 | docs | 回归归档与发布报告。把 S1 到 S18 的抓包与 golden 输出固化为回归夹具，逐条勾选 7.7 的发布判据 | A+Q | 1 | D6-01 到 D6-09 | `docs/evidence/DCU-P6/summary.md`、`tests/fixtures/dcu_698/` | 11 条发布判据全部有命令与结果，回归夹具可重跑 |

## 9. 无人值守派活协议

### 9.1 建 issue

评审通过后，第 8 节的每一行建一个 GitHub issue。标题用 `[DCU-<ID>] <任务>`。标签：

- `dcu`
- `p0` 到 `p6`
- `feature`、`refactor`、`defect`、`docs` 或 `test`
- `blocked` 表示依赖未合并
- `ready` 只在人评审通过后打。没有 `ready` 的 issue，执行者不得开工

issue 正文直接复制该任务行，再补四块：

```text
类覆盖：<Class 编号，或 none>
文件：<本任务会改哪些文件>
依赖：<任务号，或 none>
完成定义：构建干净 + 目标测试绿 + 运行时证据 + 守卫通过 + `/no-comments` 通过
```

### 9.2 分支、提交与 PR

- 分支：`dcu/<ID>-<slug>`，一任务一分支，禁止复用。
- 提交：`feat|fix|refactor|docs|test(DCU-<ID>): 一句话`。
- 一个 PR 只对应一个任务。重构与功能变更分开。既有缺陷修复单独 PR。
- PR 描述写清 `Why`、改了什么、证据路径、回滚方式。
- 合并前必须有人评审。AI 自评不算独立评审。

### 9.3 依赖与冲突域

调度器只按第 8 节的依赖启动任务。同一文件被两个任务改时，先合并依赖方，后启动被依赖方，或者把文件改动拆到两个 PR。文件冲突域按第 5.8 节划分：

| 冲突域 | 文件 | 串行要求 |
| --- | --- | --- |
| 目录与对象模型 | `scripts/extract_dcu_698_catalog.py`、`dcu_698_object_model.*`、`dcu_router.*` | D1-00 到 D1-03 串行，D1-04 之后可并行 |
| 北向 | `dcu_north_adapter.c`、`dcu_north_source.c` | D1-05 到 D1-09 串行 |
| 采集 | `dcu_poller.*`、`dcu_698_rules.*`、`dcu_698_acq.c` | D2-01 先合并，D2-02 到 D2-09 串行 |
| 冻结与事件 | `dcu_event_monitor.c`、`dcu_698_freeze.c`、`dcu_698_event.c` | D3-01 先合并，D3-02 到 D3-08 串行 |
| 控制与设备 | `dcu_load_control.*`、`dcu_698_control.c`、`dcu_698_devmgr.c`、`dcu_698_appconn.c` | D4-01 先合并，D4-02 到 D4-10 串行 |
| 变量与统计 | `dcu_698_var.c`、`dcu_698_stat.c` | D1-04 先合并，D5-01 到 D5-06 串行 |

### 9.4 证据包

每个任务提交时附带：

```text
docs/evidence/DCU-<ID>/summary.md     # 改了什么、为什么、完成定义逐项勾选
docs/evidence/DCU-<ID>/tests.log      # 构建与目标测试输出
docs/evidence/DCU-<ID>/runtime.log    # 验收或模拟主站抓包
docs/evidence/DCU-<ID>/diffstat.txt   # git show --stat
docs/evidence/DCU-<ID>/comments.md    # /no-comments 审查结果：删除、保留、HOLD
```

证据要求按任务类型：

- `defect` 必须有失败复现与修复后通过的对照。
- `feature` 必须有单元测试与一条端到端或验收证据。
- `refactor` 必须证明行为不变，原有测试全绿，必要时加特征测试。
- `docs` 必须有评审记录。
- `test` 必须有正反例，守卫类任务还要证明故意违规会变红。

### 9.5 停止条件

遇到以下任一情况，执行者停止并在 issue 留言，不得自行扩权：

1. 依赖任务未合并，或依赖的公共组件不存在。
2. 698 报文语义在 `docs/698.pdf` 找不到依据。
3. 需要改 `modules/edge_core` 或受保护的 foundation 头，且不在白名单。
4. 需要放宽测试、跳过门禁、删除失败用例。
5. 需要改公共组件、ADR 或跨产品契约。
6. 验收证据无法复现，或只能在特定机器上通过。

### 9.6 每日循环与合并门禁

1. 调度器读第 8 节，找 `ready` 且依赖已合并的任务。
2. 按 9.2 开工，先跑一次基线测试，记录起点 SHA。
3. 完成后按 9.4 打包证据，开 PR。
4. 独立评审 + `/no-comments`（comment-sicko）注释审查 + 目标分支重跑 `run_guard_selftest.py` 与 `./test.sh`。
5. 合并后重跑守卫。单批通过不等于合并后通过。
6. 每个阶段结束时按第 7.6 节的阶段判据检查，未过不启动下一阶段的新任务。

## 10. 里程碑与判据

| 里程碑 | 内容 | 人日 | 周期 | 判据 |
| --- | --- | --- | --- | --- |
| M0 | P0 完成，产品可持续运行 | 6.5 | 第 1 周 | 默认模式不冻结、不跳闸、不退出；stdout 守卫覆盖 DCU |
| M1 | P1 完成，对象模型、北向四类请求与协议栈缺口可用 | 39 | 第 1 到 8 周 | 26 类目录冻结；GET、SET、ACTION、Record 各有验收；A-XDR 端口、SET、Record、Report、Class 20 各有用例 |
| M2 | P2 完成，Class 10 档案与方案可用 | 20 | 第 9 到 11 周 | 动态加表、改周期、列选择、事件方案、透明方案、上报方案都有验收 |
| M3 | P3 完成，冻结、事件、参数可用 | 18 | 第 12 到 14 周 | 13 个冻结 OI、Class 7 与 24、Class 8 白名单都有证据 |
| M4 | P4 完成，控制、安全、设备与通信可用 | 24 | 第 15 到 18 周 | Class 13、23、19、20、21、18、22、25、26 各有验收 |
| M5 | P5 完成，剩余类与文档收尾 | 7.5 | 第 19 周 | 26 个类每类至少一条验收判据；文档与代码一致 |
| M6 | P6 完成，产品化验证与验收齐全 | 22.5 | 第 5 到 20 周 | S1 到 S18 全绿；ASAN、UBSAN、TSAN、覆盖率、性能、长稳、安全全过；发布报告归档 |

## 11. 风险与开放问题

### 11.1 需要人拍板

标准语义不需要拍板。`docs/698.pdf` 是规范性文件，7.3、7.4、附录 A、附录 H 把类、属性、方法、字节都写死了。需要人拍板的是产品范围，不是协议语义。

1. Class 12 脉冲计量与 Class 17 显示是否首版必须支持方法与写入。
2. Class 21 的密钥、证书、钱包方法是否首版开放，取决于项目安全认证要求。
3. Class 25 与 26 的多模块并存与主备通道切换，是否首版必需。
4. 冻结保留策略。日、月、曲线冻结各保留多久，决定 SQLite 容量与清理任务。
5. 多主站与备用通道是否首版必需。当前北向从站是单监听路径。
6. 实机台架是否纳入本次产品化验收。模拟台架不能替代硬件兼容性。
7. 5004 记录型对象的寻址。主站如何指定要读哪块表的冻结记录：OAD 元素索引选表实例、Class 20 会话区分客户机、还是返回台区聚合记录。已核 7.3.20：Class 20 的“当前连接的客户机地址”是主站侧身份，对象列表是访问权限，**不承担表计寻址**。建议默认用 OAD 的元素索引（`oad & 0xFF`）选表槽位，与 `DCU_OBJ_PER_METER` 的现有模型一致；索引越界回 DAR 6。该口径写入设计文档后再开 D1-07。
8. OI 8000 的继电器映射。表 A.9 的参数用“继电器 OAD”寻址，产品目前只有电表地址/槽位；需要产品定义继电器 OAD → 电表地址或 DO 通道，D1-06 的 129/130/131 才能执行。建议映射写进产品点位表，由 `dcu_node` 提供查询；在映射确认前保持 DAR 2，绝不按 OAD 猜继电器。

### 11.2 技术风险

| 风险 | 影响 | 应对 |
| --- | --- | --- |
| 公共组件未按期合并 | P2 到 P4 阻塞 | 先交付 P0 与 P1A，P2 到 P4 保持 blocked，不在本产品重写 |
| 从站端口只传标量 | Class 7、9、10、20 的属性是结构，无法通过现有 `resolve` 传给主站 | D1-12 到 D1-13 扩展 `resolve_axdr` 与 `accept_axdr`，走 A-XDR 通道 |
| 从站驱动缺 SET、Record、Report、Class 20 | 主站只能 GET 标量与 ACTION，写参数、查冻结、主动上报、应用连接都不可用 | D1-14 到 D1-17 补协议栈缺口，按标准 7.3 与附录 A、H 实现 |
| 目录 OCR 噪声 | 类归属错误 | D1-01 以 PDF 为准重新抽取，override 表留痕，生成物不改 |
| 对象表迁移引入回归 | 现有北向读失效 | D1-03 保留 `dcu_router` 门面，先迁移再删旧分支，原测试全绿 |
| 演示行为删除影响验收 | 验收依赖自动冻结与跳闸 | D0-02 用 `DCU_DEMO_MODE=1` 保留台架路径，验收显式开启 |
| 26 类一次铺开导致质量下降 | 只加表不加测试 | 每类必须同时有单元测试与验收判据，D1-10 与 D6-01 卡住 |
| SQLite 写放大与 Flash 寿命 | 长期运行磨损 | D6-06 用写放大指标与掉电测试证明，缓存合并继续保留 |
| 4096 表规模下的延迟 | 轮询超时与主站超时 | D6-07 用 P99 门禁，通道链与熔断保持现有设计。首版先测基线，再把阈值写进 `EDGE_PERF_P99_MS` 与 `EDGE_PERF_AVG_US`，只降不升 |
| 模拟台架通过不等于实机通过 | 现场硬件差异 | 实机验收独立排期，模拟台架只证明行为与协议，不替代硬件兼容性测试 |

### 11.3 阻塞项

阻塞项按“谁能解除”分类。B1 到 B5 是代码缺口，能在本仓库内解除。B6 是外部依赖。B7 是评审门。B8 是硬件排期。

| ID | 阻塞 | 证据 | 影响任务 | 解除条件 |
| --- | --- | --- | --- | --- |
| B1 | 从站端口 `resolve` 只返回标量，Class 7、9、10、20 的数组与结构传不出去 | `foundation/include/edge_slave_plane.h:36-50` 的 `edge_point_value_t` 只有 value、raw、width、type、quality、ts；`driver_dlt698.c:187` 的编码缓冲是 8 字节 | D1-05、D1-07、D2-02 到 D2-08、D3-02 到 D3-06、D4-05 | D1-12 出 ADR，D1-13 加 `resolve_axdr` 与 `accept_axdr` |
| B2 | 698 从站不处理 SET-Request | `driver_dlt698.c:257` 只进 `GET_REQ`，`:315` 只进 `ACTION_REQ`，`:360` 直接返回 -1；`DLT698_SVC_SET_REQ` 只有常量没有分支 | D1-06 的 SET 部分、D3-06、D4-10 | D1-14 补 `SET_REQ` 分支，调 `accept_axdr` |
| B3 | 698 编解码与从站不实现 GetRecord 与 RSD | `dlt698_codec.h:40` 注释说明 `[3]/[4] Record` 存在，但 `dlt698_decode_frame_ex` 只输出 `[1]/[2]`；驱动没有 Record 分支 | D1-07、D3-02、D3-03、D6-02 | D1-15 补 Record 编解码、RSD 解析与驱动分支 |
| B4 | 没有 REPORT.notification 路径 | `edge_slave_source_ops_t` 没有上报回调；`dlt698_codec.h:36` 只有 `DLT698_SVC_REPORT_RES` 常量，没有编码函数 | D2-06、D3-03、D3-07 | D1-16 加端口回调、驱动编码与产品上报源 |
| B5 | 从站交换无跨帧状态，Class 20 应用连接无法建立会话与 ACL | `driver_dlt698.c:223` 写明 `(void)session; /* 无跨帧状态 */` | D4-05、D4-07 | D1-17 加会话状态与 Class 20 对象列表查询 |
| B6 | `edge_acq`、`edge_event`、`edge_time`、`edge_control` 未实现 | `modules/` 下没有这四个组件；公共组件计划 COM-C1-01、COM-C2-01/02、COM-C3-01 | P2、P3、P4 的迁移任务 | 公共组件计划合并后，按依赖启动 |
| B7 | 规范要求人工评审后才打 `ready` | `AGENTS.md` 的“所有 spec 经人工评审后，才由评审人打 ready 标签” | 全部任务 | 评审通过，按 9.1 打 `ready` |
| B8 | 实机台架不在本计划自动化范围 | 第 7.3 节写明模拟台架为主 | 发布验收 | 项目排期实机台架，或明确接受模拟台架作为本次发布口径 |

B1 到 B5 都不是标准模糊。标准把结构写清楚了，缺的是代码里的端口与分支。这五项必须在 P1 内解决，否则 P2 到 P4 的属性类任务只能在标量上打转。

## 12. 执行状态

> 最后更新：2026-11-21
> 状态枚举：未开始、进行中、待验收、已合并、阻塞、失败。
> 一个任务只有构建、目标测试、验收证据齐全才记为已合并。合并后回填分支与 HEAD。

### 12.1 当前队列

按依赖与风险排序。前一批未合并，不启动下一批。

1. D0-02 去掉演示行为。已合并。
2. D0-03 stdout 行缓冲与守卫。已合并。
3. D1-03、D1-04 对象表与标量读取。已合并。D1-13 的端口需要对象表可调。
4. D1-13 到 D1-17 协议栈缺口。D1-12 已合并，端口实现依赖 D1-03。
5. D1-00、D1-01、D1-02 目录、OCR 核对、对象模型设计。D1-00 已合并并接入 CI；OCR 核对（D1-01）待做。
6. D1-05、D1-06、D1-07、D1-11 北向五类请求。
7. D0-01、D0-06 文档与 README。不阻塞代码，排在协议栈缺口之后。
8. D0-04、D0-05 基线与真实帧向量；D0-05a 修 HCS 校验，D0-05b 对齐 DAR。
9. P2 到 P5 在公共组件与端口依赖满足后启动。
10. P6 随各阶段并行，最后做发布判据。

队列调整理由：D1-12 是 B1 的唯一解除路径，且 D1-05、D1-06、D1-07 与 P2 到 P4 都依赖它。先做协议栈缺口，避免后续属性任务在标量上返工。

### 12.2 任务状态

| 任务 | 状态 | 分支 | HEAD | 证据 | 备注 |
| --- | --- | --- | --- | --- | --- |
| D0-01 | 已合并 | `dcu/D0-01-readme` | `ea775f3f` | `docs/evidence/DCU-D0-01/` | README 307→83 行，按代码现状重写；文档任务跳过 `/no-comments`；D1-13 合并后补 A-XDR 现状一行 |
| D0-02 | 已合并 | `dcu/D0-02-remove-demo` | `d0eb1f43` | `docs/evidence/DCU-D0-02/` | 构建、25/25 目标测试、验收 2+4、全部守卫通过 |
| D0-03 | 已合并 | `dcu/D0-03-stdout-guard` | `3bfc4237` | `docs/evidence/DCU-D0-03/` | 构建、25/25 目标测试、守卫正反夹具、验收 2+4 通过；5 个产品入口补 `_IOLBF`，组合根预算相应 +1 到 +2 |
| D0-04 | 已合并 | `dcu/D0-04-baseline` | `5c808baf` | `docs/evidence/DCU-P0/` | 合并提交 `5c808baf`；Release/UBSAN/验收/覆盖率通过；ASAN 发现测试帧生命周期缺陷（另立 D0-08）；覆盖率 `cmd` 依赖先跑 `app_platform_e2e`，复现顺序已记录 |
| D0-05 | 已合并 | `dcu/D0-05-frame-vectors` | `0f45d0e2` | `docs/evidence/DCU-D0-05/` | 26/26 目标测试、套件 21 例 19 过 2 skip；`/no-comments` 删 26 行；DAR 向量改 6；HCS 归 D0-05a，DAR 代码对齐归 D0-05b |
| D0-05a | 已合并 | `dcu/D0-05a-hcs` | `ff845789` | `docs/evidence/DCU-D0-05a/` | 27/27、套件 20 过 1 skip、守卫、验收通过；HCS 5.1.5 已校验 |
| D0-05b | 已合并 | `dcu/D0-05b-dar` | `731b1331` | `docs/evidence/DCU-D0-05b/` | 合并提交 `731b1331`；DAR 0/2/4/5/6/7 与失败码枚举；GET-Normal 缺对象回 DAR；合并后 28/28、ASAN 6/6、告警门禁、守卫、验收全过 |
| D0-06 | 已合并 | `dcu/D0-06-docs` | `50feb55e` | `docs/evidence/DCU-D0-06/` | 文档与代码对齐；`git diff --check`、目录检查、守卫通过；文档任务不做 `/no-comments` 代码注释审查 |
| D0-07 | 已合并 | `dcu/D0-07-catalog-ci` | `355c8251` | `docs/evidence/DCU-D0-07/` | 合并提交 `355c8251`；目录门禁进 CI 与守卫自检，parity 11 条；动态真实文档副本正反夹具；`/no-comments` 删 10 行 |
| D0-08 | 已合并 | `dcu/D0-08-asan-frame-lifetime` | `9e458af6` | `docs/evidence/DCU-D0-08/` | 合并提交 `9e458af6`；`decode_frame_borrows` 借用调用方帧，删本地 512 字节存储；ASAN 全量 330/330、目标 6/6；合并后 main 再跑 ASAN 6/6、守卫全绿 |
| D0-08a | 已合并 | `dcu/D0-08-frame-len` | `438ed230` | `docs/evidence/DCU-D0-08/` | 合并提交 `438ed230`；`len > UINT16_MAX` 拒绝，`(uint16_t)` 显式收窄；clean build 告警门禁 0 new |
| D0-09 | 已合并 | `dcu/D0-09-class-ids` | `6638ce23` | `docs/evidence/DCU-D0-09/` | 合并提交 `6638ce23`；F100→21、F200→22、3000/3005→24；生成器按 7.3 小节解析，F100 入目录（513→514）；快照改按处理器族分派；合并后告警、目录、28/28、守卫、验收全过 |
| D0-10 | 已合并 | `dcu/D0-10-atomic-bus-log` | `28b0363f` | `docs/evidence/DCU-D0-10/` | 合并提交 `28b0363f`；`dump_json_atomic`（同目录 mkstemp+fsync+replace）替换 4 处 `write_text`；并发读守卫；合并后验收连跑 2 次绿 |
| D1-00 | 已合并 | `main` | `355c8251` | `scripts/extract_dcu_698_catalog.py` | 生成器、目录与 `--check` 门禁均已入库；D0-07 接入 CI 与守卫自检 |
| D1-01 | 进行中 | `dcu/D1-01-catalog-classes` | `21ca75f0` | `docs/evidence/DCU-D1-01/` | dcu-dev 执行中；逐行用 7.3/7.4/附录 A 定类，冲突归零，生成器对未解释差异变红 |
| D1-02 | 待验收 | `main` | `8d8089bf` | `docs/design/DCU_698_OBJECT_MODEL.md` | 设计文档已提交，待架构评审 |
| D1-03 | 已合并 | `dcu/D1-03-object-model` | `81730385` | `docs/evidence/DCU-D1-03/` | router 664→76 行；256 位 mask，门面走预检；object_model 709 行待 D1-04 拆；合并后 26/26 测试、守卫、验收全过 |
| D1-04 | 已合并 | `dcu/D1-04-var-handlers` | `100f9a41` | `docs/evidence/DCU-D1-04/` | 27/27 目标测试、守卫、验收 2+4 通过；200A/200F 与 attr2/attr4 类型对齐；`/no-comments` 删 5 条并修 `IC_VARIABLE` 过期示例；704 行待后续拆 |
| D1-05 | 已合并 | `dcu/D1-05-resolve-table` | `ac7b8dfe` | `docs/evidence/DCU-D1-05/` | 合并提交 `ac7b8dfe`；`dcu_698_value_for_oad` 成为 OAD→类型唯一来源；验收扩到 2000/2001/2004/6000/6012/5004 七例；顺带修 5004 计数回调的 ~900KB 越界（ctx 二次取址） |
| D1-06 | 部分合并 | `dcu/D1-06-action` | `9a7fb7f9` | `docs/evidence/DCU-D1-06/` | 合并提交 `9a7fb7f9`；127–131 参数按表 A.9 解码，错形状回 DAR 7；129–131 因产品没有继电器 OAD→电表地址映射，诚实回 DAR 2（不假装执行）；127/128 无告警输出同回 2；继电器映射需产品拍板 |
| D1-07 | 未开始 | - | - | - | 设计口径已定（OAD 元素索引选表槽位，见设计文档 Class 9）；等 D1-17 合并后再开，避免与驱动/端口同文件冲突 |
| D1-08 | 已合并 | `dcu/D1-08-normal-list` | `c291f3e7` | `docs/evidence/DCU-D1-08/` | 合并提交 `c291f3e7`；混合成败、空列表（表73 无下限）、重复 OAD、验收未知 OI 逐项 DAR；修正 codec 对 n==0 的旧拒绝 |
| D1-09 | 未开始 | - | - | - | - |
| D1-10 | 已合并 | `dcu/D1-10-catalog-consistency` | `5a4fda20` | `docs/evidence/DCU-D1-10/` | 合并提交 `5a4fda20`；生成 514 OI + 26 类夹具，表→目录严格对拍、mask 范围、OI 唯一；红色对照 F200 改类；合并后 29/29、目录门禁、守卫、验收全过 |
| D1-11 | 阻塞 | - | - | - | 依赖 D1-06 的继电器映射与 D1-07 的记录取数；当前 ACTION 只有 DAR 失败路径 |
| D1-12 | 已合并 | `dcu/D1-12-axdr-port-adr` | `076e0e04` | `docs/evidence/DCU-D1-12/` | ADR-0013 已合并，状态 Proposed；14 个初始化点全为 designated，无位置初始化；架构签字待补 |
| D1-13 | 已合并 | `dcu/D1-13-axdr-port` | `68f4de69` | `docs/evidence/DCU-D1-13/` | 合并提交 `68f4de69`；合并后 28/28、守卫、验收 2+4 全过；流式逐项编码，单缓冲复用，无 8×2048；`/no-comments` 删 23 行、DAR 字面量改常量 |
| D1-14 | 已合并 | `dcu/D1-14-set` | `b5b95f62` | `docs/evidence/DCU-D1-14/` | 合并提交 `b5b95f62`；SET-Request、`accept_axdr` 写入、SET-Response DAR；拒绝非标量回落假成功；LONG64 无 UB；DENIED 3 与 CLASS_MISMATCH 5 分开；合并后告警、28/28、ASAN、守卫、验收全过 |
| D1-14a | 已合并 | `dcu/D1-14a-be32` | `e2ce657d` | `docs/evidence/DCU-D1-14a/` | 合并提交 `e2ce657d`；`_read_be32` 统一 5 处大端读取；高字节 ≥0x80 用例；合并后 UBSAN 全量 332/332、0 runtime error |
| D1-15 | 已合并 | `dcu/D1-15a-record-codec` + `dcu/D1-15b-record-driver` | `7658967a` | `docs/evidence/DCU-D1-15a/`、`DCU-D1-15b/` | D1-15a codec 层 GetRecord/RSD/RCSD；D1-15b ADR-0014 记录端口、Selector1、驱动 GET [3]、分页；产品记录取数留 D1-07 |
| D1-15b | 已合并 | `dcu/D1-15b-record-driver` | `7658967a` | `docs/evidence/DCU-D1-15b/` | 合并提交 `7658967a`；端口三回调协议中立；行失败整条回 DAR；>255 回 DAR 2；不支持选择方法空 RCSD 限制已入 ADR |
| D1-16 | 未开始 | - | - | - | - |
| D1-17 | 进行中 | `dcu/D1-17-appconn` | `21ca75f0` | `docs/evidence/DCU-D1-17/` | edgec-dev-0 执行中；先写 ADR-0015 会话端口，再接 4400 应用连接与对象列表 ACL |
| D2 到 D5 | 阻塞 | - | - | - | 依赖 COM 组件，见 11.3 B6 |
| D6 | 未开始 | - | - | - | 随各阶段并行 |

### 12.3 本轮验收记录

| 时间 | 任务 | 结果 | 证据 | 备注 |
| --- | --- | --- | --- | --- |
| 2026-11-21 | D0-02 | 待验收 | `/tmp/dcu-verify-D0-02.log` | 构建、19/19 目标测试、证据路径范围通过；发现 `runtime.log` 被 `.gitignore` 挡住未入库，`dcu_product.py` 的 `DCU_DEMO_MODE` 转发可被外层 0 覆盖；已退回补提交 |
| 2026-11-21 | D0-02 | 已派单 | - | dcu-dev 在 `/tmp/dcu-dev-worktree` 执行 |
| 2026-11-21 | D0-02 | 已合并 | `docs/evidence/DCU-D0-02/` | 合并提交 `d0eb1f43`；合并后 `ctest -R dcu` 25/25，`run_guard_selftest.py` 全绿，验收 `dcu-acceptance` 2 项、`dcu-northbound` 4 项通过 |
| 2026-11-21 | D0-03 | 已合并 | `docs/evidence/DCU-D0-03/` | 合并提交 `3bfc4237`；合并后 25/25 目标测试、守卫正反夹具、验收 2+4 全过；stdout 守卫扩到全部 `cmd/*/main.c` |
| 2026-11-21 | D1-12 | 已合并 | `docs/evidence/DCU-D1-12/` | 合并提交 `076e0e04`；ADR-0013 Proposed；独立核对 55 处引用、14 个 designated 初始化点、0 个位置初始化；守卫与目录检查通过 |
| 2026-11-21 | D1-03 | 待验收 | `/tmp/dcu-dev-worktree/docs/evidence/DCU-D1-03/` | 构建与 20/20 目标测试通过；`dcu_router.c` 664 到 96 行；review 发现 `uint16_t` mask 覆盖不了方法 127 到 134 与属性 18，门面绕过预检属 workaround；设计改为 256 位位图，已退回补实现 |
| 2026-11-21 | D0-05 | 已派单 | - | edgec-dev-0 在 `/tmp/dcu-docs-worktree` 执行；真实帧向量从 `docs/698.pdf` 摘 |
| 2026-11-21 | D1-03 | 已合并 | `docs/evidence/DCU-D1-03/` | 合并提交 `81730385`；26/26 目标测试、7/7 新测试、守卫、验收 2+4 全过；`dcu_router.c` 664→76；mask 改 256 位并删门面绕过 |
| 2026-11-21 | 清理 | 完成 | - | 删除 `dcu/D1-03-object-model` 分支；删除本会话 `/tmp` 日志与 scratch；删除被忽略的 `reports/`；工作树切到 `dcu/D1-04-var-handlers` |
| 2026-11-21 | D1-03 注释 | 已合并 | `docs/evidence/DCU-D1-03/comments.md` | `/no-comments` 删除 100 行注释、0 行代码；构建、26/26 测试、守卫、验收 2+4 通过；提交 `5e145714` |
| 2026-11-21 | D1-04 | 待验收 | `/tmp/dcu-dev-worktree/docs/evidence/DCU-D1-04/` | 构建与 21/21 通过；review 发现 `OAD_POWER_FACTOR_TOTAL` 指 2009、`OAD_GRID_FREQUENCY` 指 200A，标准应为 200A/200F；0010/0020 attr2 应为 0x06、attr4 为 0x15；已退回修宏与逐属性类型标签，并 rebase 到 5e145714 |
| 2026-11-21 | D0-05 | 已合并 | `docs/evidence/DCU-D0-05/` | 合并提交 `0f45d0e2`；26/26 目标测试、守卫、验收 2+4 通过；套件 21 例 19 过 2 skip；`/no-comments` 删 26 行；DAR 向量从 5 改 6，设计文档 DAR 表改为 4/5/6/7；HCS 立 D0-05a，DAR 代码对齐立 D0-05b |
| 2026-11-21 | D1-04 | 已合并 | `docs/evidence/DCU-D1-04/` | 合并提交 `100f9a41`；27/27 目标测试、守卫、验收 2+4 通过；200A/200F 与 attr2/attr4 对齐；`/no-comments` 删 5 条并修 `IC_VARIABLE` 过期示例 |
| 2026-11-21 | 派单 | D1-13、D0-05a | - | 两个独立工作树并行；D1-13 走端口与驱动 GET，D0-05a 走 codec HCS 与测试 |
| 2026-11-21 | D0-05a | 已合并 | `docs/evidence/DCU-D0-05a/` | 合并提交 `ff845789`；27/27、套件 20 过 1 skip、守卫、验收通过；HCS 范围 5+sa_len，先 HCS 后 FCS |
| 2026-11-21 | D0-06 | 已合并 | `docs/evidence/DCU-D0-06/` | 合并提交 `50feb55e`；北向与 DCU 上下文文档按代码现状重写；文档任务跳过 `/no-comments` 代码注释审查 |
| 2026-11-21 | D0-01 | 已合并 | `docs/evidence/DCU-D0-01/` | 合并提交 `ea775f3f`；README 307→83 行；文档任务跳过 `/no-comments`；D1-13 合并后补 A-XDR 一行 |
| 2026-11-21 | D1-13 | 已合并 | `docs/evidence/DCU-D1-13/` | 合并提交 `68f4de69`；合并后 28/28 目标测试、守卫全绿、验收 2+4 通过；流式逐项编码替代 8×2048；`/no-comments` 23 行，DAR 字面量改 `DLT698_DAR_*` |
| 2026-11-21 | D0-07 | 已合并 | `docs/evidence/DCU-D0-07/` | 合并提交 `355c8251`；合并后目录门禁 0、parity 11 条、守卫自检全绿、28/28、验收 2+4；临时真实文档副本改坏后拒收 |
| 2026-11-21 | D0-08 | 已合并 | `docs/evidence/DCU-D0-08/` | 合并提交 `9e458af6`；ASAN 全量 330/330、目标 6/6；合并后 main 的 ASAN 目标 6/6、release 28/28、守卫全绿；`/no-comments` 采纳 1 条 MUST KILL，helper 改名 `decode_frame_borrows` |
| 2026-11-21 | D0-08a | 已合并 | `docs/evidence/DCU-D0-08/` | 合并提交 `438ed230`；clean build 的 `-Wconversion` 归零；`check_build_warnings.py` 0 new |
| 2026-11-21 | D0-05b | 已合并 | `docs/evidence/DCU-D0-05b/` | 合并提交 `731b1331`；28/28、ASAN 6/6、告警门禁、守卫、验收 2+4；ADR-0013 2.1 记录失败码与 DAR 映射 |
| 2026-11-21 | D0-09 | 已合并 | `docs/evidence/DCU-D0-09/` | 合并提交 `6638ce23`；class_id 四处修正，F100 入目录，快照按处理器族；复核恢复了 2 条被过度删除的 why 注释 |
| 2026-11-21 | D1-14 | 已合并 | `docs/evidence/DCU-D1-14/` | 合并提交 `b5b95f62`；SET 分支、`EDGE_SLAVE_SRC_DENIED`、codec 增加 BOOLEAN/LONG64/FLOAT 解码；复核退回三处：非标量回落、LONG64 UB、DAR 混用 |
| 2026-11-21 | D1-10 | 已合并 | `docs/evidence/DCU-D1-10/` | 合并提交 `5a4fda20`；夹具随生成器 `--check` 一起校验；守卫自检临时根补 fixture；29/29 |
| 2026-11-22 | D1-15a | 已合并 | `docs/evidence/DCU-D1-15a/` | 合并提交 `87185420`；H.3.3 响应逐字节复现；RSD [1]/[3..10] 明确返回不支持；29/29、0 skip、ASAN、守卫、验收通过 |
| 2026-11-22 | D1-05 | 已合并 | `docs/evidence/DCU-D1-05/` | 合并提交 `ac7b8dfe`；单类型表；验收 7 例；修 `_node_count_freezes` 的 ctx 越界；30/30、全量 332/332、ASAN、守卫、验收通过 |
| 2026-11-22 | D1-15b | 已合并 | `docs/evidence/DCU-D1-15b/` | 合并提交 `7658967a`；ADR-0014；驱动 10/10、套件 30/30；验收一次 flake（空 bus.json），重跑通过，已立 D0-10 |
| 2026-11-22 | D1-08 | 已合并 | `docs/evidence/DCU-D1-08/` | 合并提交 `c291f3e7`；空列表按表73 无下限改为合法；验收 8 例；平台解析器不支持逐项 DAR/空列表，测试直读 APDU，平台更新另记 |
| 2026-11-22 | D0-10 | 已合并 | `docs/evidence/DCU-D0-10/` | 合并提交 `28b0363f`；原子写消除 `bus.json` 半截读；并发守卫；合并后验收 2 次绿；复核恢复了 7 条被删的运行约束 |
| 2026-11-22 | D1-06 | 部分合并 | `docs/evidence/DCU-D1-06/` | 合并提交 `9a7fb7f9`；A.9 参数解码与 DAR 7；129–131 因缺继电器映射回 DAR 2；codec 补 OAD 标签；验收 9 例；组合根台账修正 `57846715` |
| 2026-11-22 | D1-14a | 已合并 | `docs/evidence/DCU-D1-14a/` | 合并提交 `e2ce657d`；UBSAN 抓到的有符号左移 UB；合并后全量 UBSAN 332/332、0 runtime error；Release 30/30、守卫、验收通过 |

### 12.4 并行工作树

| 工作树 | 分支 | 负责人 | 任务 | 文件边界 |
| --- | --- | --- | --- | --- |
| `/tmp/dcu-dev-worktree` | `dcu/D1-14-set` | dcu-dev | D1-14 SET-Request | `driver_dlt698.c`、`foundation/src/protocol/`、`cmd/dcu/dcu_698_axdr.c`、`tests/` |
| `/tmp/dcu-baseline-worktree` | `dcu/D1-10-catalog-consistency` | edgec-dev-0 | D1-10 目录一致性测试 | `scripts/extract_dcu_698_catalog.py`、`cmd/dcu/dcu_698_object_model.*`、`tests/` |
| `/tmp/dcu-docs-worktree` | detached | - | 空闲，D0-07 已完成 | - |

并行只跨文件边界。`driver_dlt698.c`、`dcu_698_object_model.c`、`dcu_router.c` 是串行冲突域，同一时刻只允许一个工作树改。

### 12.5 已知缺口（不阻塞本轮合并，需另立任务或产品拍板）

| 项 | 现状 | 影响 | 去向 |
| --- | --- | --- | --- |
| F200 属性语义 | 产品把 attr2 当波特率（uint32）、attr3 当校验位（uint8）；目录 Class 22 里 attr2 是设备对象列表（array）、attr3 是数量（unsigned） | 主站按标准读 F200 会拿到类型不符的值；D1-14 的合法写用例沿用了现有产品口径 | 产品点位表拍板后另立 defect；D1-10 只断言位号范围，不判定语义 |
| 3000/3005 属性号 | `class_id` 已改 24，但 `attr_mask` 仍用 Class 7 的属性号（2–6、8–10），Class 24 的属性定义不同 | 主站按 Class 24 读会落到未定义属性；位号范围检查发现不了 | P3 事件任务（D3-02 起）按 Class 24 重排并加验收 |
| codec 魔数标签 | `dlt698_codec.c` 既有编码器里仍有 `18`/`5`/`28` 字面量，未用 `DLT698_TAG_*` | 可读性，易与新标签漂移 | 随下一次 codec 改动清理，单独 commit |
| D1-01 OCR 冲突 | 目录仍有 79 行 IC 列错位；D0-09 只补了 F100 这一行 | 类归属可能错 | D1-01 按 PDF 逐行核对后归零 |
| A.7 方法上限按类聚合 | 夹具把某 OI 的 A.7 方法号并进该类的 `max_method`，范围检查对同类其它 OI 也放行 | 只能发现越界，发现不了“方法放错 OI” | 后续把夹具改成 per-OI 方法集合 |
| 生成器行窗口 | `parse_ois` 与 `parse_oi_methods` 都写死 6298..9450 | 原文行号变化会静默丢行 | 改为按附录标题定位 |
| test-platform 解析器 | `testplatform.protocol.dlt698.parse_get_response_list` 遇到逐项 DAR 抛错、拒绝 count 0；`get_request_list` 仍拒绝空列表 | 验收只能直读 APDU，平台侧无法表达合法空列表与混合结果 | 外部 pinned 仓库更新；本仓不改 |
| 继电器 OAD 映射 | A.9 的 129/130/131 用“继电器 OAD”寻址，产品只有 `dcu_inventory_by_slot()` 的槽位身份 | ACTION 129–131 只能回 DAR 2，无法真正跳合闸 | 产品点位表拍板：继电器 OAD → 电表地址/DO 通道；另立任务实现 |

## Appendix A. 698 覆盖明细

完整属性、方法与 OI 清单见 `docs/design/DCU_698_OI_CATALOG.md`。本计划的覆盖矩阵见第 6 节。两者不一致时，以目录为准，并开 D1-01 的核对任务。

目录生成：

```bash
python3 -I scripts/extract_dcu_698_catalog.py
python3 -I scripts/extract_dcu_698_catalog.py --check
```

`docs/698_CLASS_STUDY.md` 给类与业务域的对应，`docs/698_OAD_LOGICAL_RELATION.md` 给 6000、6012、5004、601D 的级联关系，`docs/698_OAD_STUDY_NOTES.md` 给 OI 与数据类型。三者不一致时，以 `docs/698.pdf` 原文为准，并在设计文档记录差异。

## Appendix B. 文件职责

| 文件 | 目标职责 |
| --- | --- |
| `cmd/dcu/main.c` | 只做组合根与生命周期 |
| `cmd/dcu/dcu_node.{h,c}` | 聚合根，持有档案、轮询、存储、事件 |
| `cmd/dcu/dcu_698_object_model.{h,c}` | 静态对象表与查表，新增 |
| `cmd/dcu/dcu_698_var.c` | Class 1 到 6 |
| `cmd/dcu/dcu_698_event.c` | Class 7 与 24 |
| `cmd/dcu/dcu_698_param.c` | Class 8 |
| `cmd/dcu/dcu_698_freeze.c` | Class 9 |
| `cmd/dcu/dcu_698_acq.c` | Class 10 |
| `cmd/dcu/dcu_698_set.c` | Class 11 |
| `cmd/dcu/dcu_698_pulse.c` | Class 12 |
| `cmd/dcu/dcu_698_control.c` | Class 13 |
| `cmd/dcu/dcu_698_stat.c` | Class 14 到 16 |
| `cmd/dcu/dcu_698_display.c` | Class 17 |
| `cmd/dcu/dcu_698_file.c` | Class 18 |
| `cmd/dcu/dcu_698_devmgr.c` | Class 19 |
| `cmd/dcu/dcu_698_appconn.c` | Class 20 |
| `cmd/dcu/dcu_698_esam.c` | Class 21 |
| `cmd/dcu/dcu_698_io.c` | Class 22 |
| `cmd/dcu/dcu_698_group.c` | Class 23 |
| `cmd/dcu/dcu_698_comm.c` | Class 25 与 26 |
| `cmd/dcu/dcu_698_rules.{h,c}` | 601F 规则表与 OAD 换算，新增 |
| `cmd/dcu/dcu_router.{h,c}` | 查表门面，保持现有调用方兼容 |
| `cmd/dcu/dcu_north_adapter.c` | 从站端口适配，表驱动 |
| `cmd/dcu/dcu_north_source.c` | 推送族名字与取值，名字从对象表派生 |
| `cmd/dcu/dcu_inventory.{h,c}` | 档案仓储 |
| `cmd/dcu/dcu_poller.{h,c}` | 采集执行，P2 后由 `edge_acq` 承担调度 |
| `cmd/dcu/dcu_storage.{h,c}` | SQLite 与冻结查询 |
| `cmd/dcu/dcu_event_monitor.c` | 事件检测，P3 后写 `edge_event` |
| `cmd/dcu/dcu_runtime_health.{h,c}` | 刷盘与自检，D0-02 后替换 heartbeat 的演示职责 |
| `cmd/dcu/dcu_driver_adapter.{h,c}` | 协议元数据登记，点表迁到规则表 |

## Appendix C. 验证命令

在仓库根执行：

```bash
# 目录
python3 -I scripts/extract_dcu_698_catalog.py --check

# 构建
cmake --build build -j"$(nproc)"

# DCU 与 698 目标测试
cd build && ctest --output-on-failure -R 'dcu|dlt698'

# 全量单元测试
./test.sh

# 产品验收
./test.sh --omnis dcu

# 内存与未定义行为
./test.sh --asan
./test.sh --ubsan

# 覆盖率
./test.sh --coverage
python3 ci/scripts/check_coverage.py --root .

# 守卫
python3 ci/scripts/run_guard_selftest.py --root .
python3 ci/scripts/check_component_boundary.py --root .
python3 ci/scripts/check_plane_isolation.py --root .
python3 ci/scripts/check_build_warnings.py --root .

# 静态分析
python3 ci/scripts/run_fanalyzer.py --root .

# 性能门禁
EDGE_PERF_GATE=1 ctest --output-on-failure -R 'dcu_scale_benchmark'
```

单个任务的出口证据至少包含：构建命令与结果、目标测试命令与结果、运行时或验收命令与结果、所改文件的 `git show --stat`。

## Appendix D. 参考

- `docs/698_parsed.md`、`docs/698_CLASS_STUDY.md`、`docs/698_OAD_LOGICAL_RELATION.md`、`docs/698_OAD_STUDY_NOTES.md`
- `docs/design/DCU_698_OBJECT_MODEL.md`、`docs/design/DCU_698_OI_CATALOG.md`、`scripts/extract_dcu_698_catalog.py`
- `docs/design/NORTHBOUND_ARCHITECTURE.md`、`docs/design/slave_plane_architecture.md`
- `docs/design/DOMAIN_CONTEXT_MAP.md`、`CONTEXT-MAP.md`、`cmd/dcu/CONTEXT.md`
- `docs/plans/2026-11-21-terminal-common-components-plan.md`、`docs/plans/2026-11-21-distribution-terminal-plan.md`、`docs/plans/2026-11-21-protocol-coverage.md`
- `docs/adr/0010-eliminate-global-contracts-and-tagid.md`、`docs/adr/0011-northbound-two-family-model.md`、`docs/adr/0007-storage-subsystem-hardening.md`
- `cmd/dcu/`、`foundation/include/dlt698_codec.h`、`foundation/include/edge_slave_plane.h`、`adapters/io_drivers/src/driver_dlt698.c`
- `tests/test_dcu_*.c`、`tests/test_dlt698_*.c`、`acceptance/dcu/`
