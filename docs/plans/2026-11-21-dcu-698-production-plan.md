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
| 同一协议族的量纲与帧规则只能有一处来源 | D2-07 把三套点表收敛成一张 601F 规则表，并加 DLT645 与 DLT698 对拍测试 |
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

冻结不是心跳的副产物。触发只有三个来源：时钟到达标准 OI 的冻结点（秒/分钟/小时/日/结算日/月/年）、主站 Class 9 方法 3（D3-03）、本地补冻。计划原写的 ACTION 8010 无标准依据，撤销。时间质量由 `edge_time` 给出，分 TRUSTED、DEGRADED、UNTRUSTED 三级。UNTRUSTED 时冻结照做，但记录打质量标记。零点对齐误差目标小于 50 毫秒。

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
| D1-06 | feature | 北向 ACTION 与 SET 接对象表方法。先做 OI 8000 跳合闸；强制冻结改走 Class 9 方法 3（8010 无标准依据，撤销），参数按标准表 A.9 解 | D | 3 | D1-03、D1-13、D1-14 | 方法处理器与 DAR 映射 | 单元测试覆盖合法参数、缺参、越权、目标离线；验收发送真实 ACTION 帧并收到正确应答 |
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

### P2 Class 10 采集监控与调度迁移（20 人日，第 9 到 11 周；2026-11-22 起改为 DCU 内先行）

前提（2026-11-22 用户拍板）：`COM-C1-01` 尚未合并，为交付速度，本阶段改为在 `cmd/dcu` 内经消费者定义端口先实现，复用现有 `dcu_poller`、`dcu_storage`、`dcu_inventory`；`edge_acq` 合并后再迁移，迁移任务登记在 §12.6。禁止把 DCU 私有接口暴露给其他产品。

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

### P3 Class 9、7、24、8 与时间、上报（18 人日，第 12 到 14 周；2026-11-22 起改为 DCU 内先行）

前提（2026-11-22 用户拍板）：`edge_time` 与 `edge_event` 未合并，本阶段改为在 `cmd/dcu` 内经现有 `dcu_event_monitor`、`dcu_storage` 与端口先实现；两个组件合并后迁移，迁移任务登记在 §12.6。

| ID | 类型 | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| D3-01 | refactor | 用 `edge_time` 替换心跳里的时间与冻结触发，删掉演示冻结 | C | 2 | COM-C2-01、D0-02 | 时间端口接入 | 时间质量分级用例通过，UNTRUSTED 时冻结带质量标记 |
| D3-02 | feature | Class 9 冻结调度器，覆盖时钟驱动的 5000–5007（8 个 OI）标准默认节奏（周期=1，秒/分钟/小时/日/结算日/月/年；属性 3 的主站可配周期/深度归 D3-03），零点对齐误差小于 50 毫秒；方法 3 触发归 D3-03；计划原写的 ACTION 8010 与“曲线冻结”无标准依据，撤销；切换类 5008–5011 无触发源且 65 项超驱动会话 1024 上限，只留对象模型不进 ACL（对象 66、ACL 60、cap 60），§12.5 记缺口 | C | 3 | D3-01、D1-02 | `dcu_698_freeze.c` | 模拟时钟跨零点，验收读 5004 与 5005，时间戳与源 OAD 一致 |
| D3-03 | feature | Class 9 方法 3、4、5、7、8 与关联对象属性表的增删（属性 3 是普通 GET array；RSD 只用于属性 2 记录，不造假选择器） | C | 2 | D3-02、D1-15 | 冻结方法处理器 | 添加与删除冻结对象属性后，Record 返回列变化 |
| D3-04 | refactor | SOE 迁移到 `edge_event`，有界缓冲、排序、溢出计数、重启不丢 | C | 2 | COM-C2-02 | 事件存储 | 排序、溢出、重启三类用例通过 |
| D3-05 | feature | Class 7 与 Class 24 的属性和方法接对象表，覆盖 3000 到 3320 的事件 OI | C | 3 | D3-04、D1-02、D1-15 | `dcu_698_event.c` | GET、GET Record、添加与删除关联对象属性各一例 |
| D3-06 | feature | Class 8 参数变量，按白名单实现日期时间、通信地址、表号、客户编号、地理位置、互感器变比、限值 | C | 2 | D1-02、D1-14 | `dcu_698_param.c` | SET 后回读一致，越权写返回 DAR=3 |
| D3-07 | feature | 3011 停电与 3106 停上电事件接入事件引擎与主动上报 | D | 2 | D3-05 | 事件对象 | 验收断电后收到主动上报 |
| D3-08 | test | Class 9、7、24、8 的单元测试与验收判据，每类至少四例 | Q | 2 | D3-02 到 D3-06 | 测试文件 | `test_dcu_698_freeze`、`test_dcu_698_event`、`test_dcu_698_param` 全绿 |

### P4 Class 13、23、19、20、21、18、22、25、26（24 人日，第 15 到 18 周；2026-11-22 起控制类改为 DCU 内先行）

前提（2026-11-22 用户拍板）：`edge_control` 未合并，控制类任务改为在 `cmd/dcu` 内经 `dcu_node` 聚合根与现有控制端口先实现，`edge_control` 合并后迁移，迁移任务登记在 §12.6；其余任务按 D1 完成后可开工。

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
docs/evidence/DCU-<ID>/spec.md        # 五栏规格：业务规则/不变量/可观测输出/验收判据/非目标
docs/evidence/DCU-<ID>/summary.md     # 改了什么、为什么、完成定义逐项勾选
docs/evidence/DCU-<ID>/tests.log      # 构建与目标测试输出
docs/evidence/DCU-<ID>/runtime.log    # 验收或模拟主站抓包
docs/evidence/DCU-<ID>/diffstat.txt   # git show --stat
docs/evidence/DCU-<ID>/comments.md    # /no-comments 审查结果：删除、保留、HOLD
```

每个单元先写 `spec.md` 五栏规格；verifier 的 `verify.md` 至少独立推导一条期望值并写推导过程。规则见 `AGENTS.md` 的「验证强度」。

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

9. `date_time_s` 的时区口径（2026-11-22 定）。表 49 只定义公历字段，没有时区。产品用设备本地公历（`localtime_r`/`mktime`），与现场主站的北京时间一致；冻结记录与 Class 24 A6/A14 共用 `dcu_698_put_date_time_s` / `dcu_698_date_time_s_to_epoch` 这一处来源。设备部署时必须配置 TZ，写入产品运维要求。改用 UTC 只需改这一对函数，不动调用方。

10. 未结束事件的结束时间编码（2026-11-22 定）。表 194 的 A6 记录单元没有 NULL 槽位，A14 才显式允许 NULL。A6 未结束记录用表 49 的非法值 `year=FFFFH, 其余=FFH`，不编 NULL；A14 仍编 NULL。有发生时间但不在可编区间时拒绝整条记录，不编 0 或单调时标。

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

> 最后更新：2026-11-27
> 状态枚举：未开始、进行中、待验收、已合并、阻塞、失败。
> 一个任务只有构建、目标测试、验收证据齐全才记为已合并。合并后回填分支与 HEAD。

### 12.1 当前队列

按依赖与风险排序。同一批内可并行，但一个任务的验收证据（构建、测试、守卫、`/no-comments`）不齐不合入。

当前在飞（2026-11-23 夜间）：

D2-05a、D2-05a-r、D2-06、D2-06-r 与 D2-07 片 1 已合并 `feat/dcu`。D2-06 是 601C/601D 管理平面；执行/SnF 停 ADR-0017，归 D2-06b/D3-05b。D2-06-r 是行为不变重构：上报方案 store 自持静态存储并由 `dcu_scheme_store_init` 显式绑定，事务助手收敛到内部头单一来源。D2-07 三片已全部合并 `feat/dcu`；片 1 把三张静态点表逐值迁入 `dcu_rule` 表，快照内嵌 `dcu_node`；片 2 的 601E/601F 管理面按主站 698 CSD OAD 寻址，相别行的内部影子 tag 不外露；片 3 的 645 备用 DI 回退由台架按 DI 失败用例验证，连续 3 次验收通过。D2-08 已合并（6034/6035 任务监控真实；6032/6033 因相序/中继无来源 DAR 2，另立）。夜间队列 D2-05a-r、D2-06、D2-07、D2-08 全部合并；D3-02 已合并（Class 9 对象/ACL/Record、调度器/记录、ms 边界对齐、测试时钟跨零点验收；属性 3 的周期/深度配置归 D3-03）；D3-03 切片 1 已合并（属性 3 配置、方法 3/4/5/7/8、GetRecord 列随配置变化；D3-03b 缺口见 §12.5）。D3-03b 已合并（每 OAD 独立周期、方法 3 延时、深度裁剪、上限拒绝与截断诊断；§12.5 已收口）。本轮 feat/dcu 已由 edgec-verifier 合入 main（`bf42d8d2`，main tip `7f227146`），feat/dcu 已 ff 到 `7f227146`；按 operator 指示（2026-11-24）恢复夜间模式，自 `7f227146` 继续派活。D0-13 已合并：产品自退定时器改为选择加入、验收壳转发平台信号回收子进程、拆除前停心跳定时器（详见 §12.2）。D4-02a（Class 13 只读与 DAR 诚实）与 D0-14（验收首读等待）已合并；D3-08（Class 9/7/24/8 测试矩阵）与 D1-21（ACL 上限移除：224/4096/10240）已合并。Class 13 控制执行待真实继电器/功控来源（§12.5）。另合入 `dcu/correct-guards`（/correct 流程）：已实现态文档引用守卫、`-Werror=implicit-*`、AGENTS 规则；守卫在工作树路径含 `guards` 时有子串误报，已记 §12.5。D1-02 设计文档经五轮独立评审批准（§12.2）。D3-01 用现有时间端口并记质量缺口；D3-05b 仍受 ADR-0017 阻塞。D2-05a 的 ACTION Data 参数与 A2 Data 结构按标准 H.5.4 用 `5C choice`。RSD Selector5 的裸 choice 是另一种协议 SEQUENCE 上下文，产品未实现，另立任务。D2-05 拆成 D2-05a 管理平面与 D2-05b 执行，D2-05b 等 D3-02/D3-05b 与 raw I/O 扩展；事件上送归 D3-02/D3-05b。dcu-dev 实现，dcu-verifier 验证并合入 `feat/dcu`；main 合并由 operator 指示后向 edgec-verifier 提请求（本轮已获指示）。`feat/dcu` 基于 main `51afa566`，PR #578 的后置缺陷修复已进 main；main 其后由其他会话推进到 `076d7ebf`，与本队列无关。

下一批（按 §12.6 迁移登记，先在 `cmd/dcu` 内实现，公共组件到位后迁走）：

1. D2-05 事件采集方案与透明方案（6016 到 601B）、D2-06 上报方案（601C/601D）、D2-07 规则表（601E/601F）、D2-08 采集状态与任务监控（6032 到 6035、6040 到 6042）。
2. D3-02 事件记录来源、D3-05b 事件上报接线（依赖 D1-16 队列为空的前置）。
3. P4：Class 13、21、22、25、26（含 D4 系列），先做只读与 DAR 诚实，再做写入。
4. P5：Class 2/5/11/12/14/15/16/17 与逐类验收判据。
5. P6：覆盖率、性能、长稳、安全与实机台架排期。
6. D1-09、D1-02 架构评审签字、D1-12 ADR 状态、10 个非 DCU 固定 `/tmp` 测试路径。

队列调整理由：B1 到 B5 已由 D1-12 到 D1-17 解除；B6 不再阻塞，因为 P2 到 P5 的产品逻辑先在 `cmd/dcu` 内按端口实现，迁移登记见 §12.6。这样并行度取决于类族边界，而不是公共组件的排期。

### 12.2 任务状态

> 谱系说明（2026-11-27）：表中「已合并 feat/dcu」指已合入集成分支并经批次验收。2026-11-26 夜间批次（merge `656731ab`）经 edgec-verifier 验收后进入 main；远端以单进口提交 `a253c4db` 把它带入新谱系，随后 PR #591 的 CI 修复与 dependabot 更新进入 `origin/main`（`8d9bc384`）。批次验收明细见 `.audit/dcu-plan-night.tsv` 的 `batch-accepted` 行；旧本地历史保留在 `backup/local-main-656731ab`。2026-11-27 起 `feat/dcu` 重建到 `8d9bc384`（旧 tip 归档在 `archive/feat-dcu-656731ab`）并已 ff 到 main；新任务从最新 `feat/dcu` 开 worktree。

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
| D1-01 | 已合并 | `dcu/D1-01-catalog-classes` | `ceb6a576` | `docs/evidence/DCU-D1-01/` | 合并提交 `ceb6a576`；79 行冲突 75 条按标准定类、4 条保留产品类并记录；生成器对无决策/再 OCR/类不符变红；核对表生成并校验 |
| D1-02 | 待验收 | `main` | `8d8089bf` | `docs/design/DCU_698_OBJECT_MODEL.md` | 设计文档已提交，待架构评审 |
| D1-03 | 已合并 | `dcu/D1-03-object-model` | `81730385` | `docs/evidence/DCU-D1-03/` | router 664→76 行；256 位 mask，门面走预检；object_model 709 行待 D1-04 拆；合并后 26/26 测试、守卫、验收全过 |
| D1-04 | 已合并 | `dcu/D1-04-var-handlers` | `100f9a41` | `docs/evidence/DCU-D1-04/` | 27/27 目标测试、守卫、验收 2+4 通过；200A/200F 与 attr2/attr4 类型对齐；`/no-comments` 删 5 条并修 `IC_VARIABLE` 过期示例；704 行待后续拆 |
| D1-05 | 已合并 | `dcu/D1-05-resolve-table` | `ac7b8dfe` | `docs/evidence/DCU-D1-05/` | 合并提交 `ac7b8dfe`；`dcu_698_value_for_oad` 成为 OAD→类型唯一来源；验收扩到 2000/2001/2004/6000/6012/5004 七例；顺带修 5004 计数回调的 ~900KB 越界（ctx 二次取址） |
| D1-06 | 部分合并 | `dcu/D1-06-action` | `9a7fb7f9` | `docs/evidence/DCU-D1-06/` | 合并提交 `9a7fb7f9`；127–131 参数按表 A.9 解码，错形状回 DAR 7；129–131 因产品没有继电器 OAD→电表地址映射，诚实回 DAR 2（不假装执行）；127/128 无告警输出同回 2；继电器映射需产品拍板 |
| D1-07 | 已合并 | `dcu/D1-07-record-storage` | `2ee06750` | `docs/evidence/DCU-D1-07/` | 合并提交 `2ee06750`；5004 记录端口接存储，Selector1/2 与 RCSD 列编码，UTC 时间基；合并后 33/33、ASAN、守卫、验收通过（一次控制帧 flake，重跑通过） |
| D1-18d | 已合并 | `dcu/D1-18d-control-security` | `f0ec96fe` | `docs/evidence/DCU-D1-18d/` | 合并提交 `f0ec96fe`；8000 方法号归位，F100 reset 真实，F200 假配置撤下，A11/A12 SET 真实；DAR 5/4 统一；合并后 35/35、守卫、验收通过 |
| D2-02 | 已合并 | `dcu/D2-02-class10` | `42516ae6` | `docs/evidence/DCU-D2-02/` | 合并提交 `42516ae6`；6000 A2 数组、127-134 持久化、6001 单元、ACL 16→20；合并后 36/36、守卫、验收 2 任务通过；40 表全量 A2 仍受 2048 缓冲限回 DAR 2 |
| D2-03 | 已合并 | `dcu/D2-03-tasks` | `b4bb8829` | `docs/evidence/DCU-D2-03/` | 合并提交 `b4bb8829`；6012 M1/127/128/129/130 与 6013 A2 真实，严格 A-XDR 解码，SQLite 持久化与回滚；ACL 18 对象；验收 2 任务 3+18 例；ACTION 验收待 D1-20a 接线后启用 |
| D3-06 | 已合并 | `dcu/D3-06-params` | `74798b0d` | `docs/evidence/DCU-D3-06/` | 合并提交 `74798b0d`；4000 A2 走真实时钟端口（EPERM/EINVAL 回 DAR 2 且时间不变），A3/A4 与 4001–4021 持久化 + 审计并按 (oi,attr) 建键；ACL Class 8 行由 `DCU_698_PARAM_OIS` 生成；明确存储语义未接消费方；合并后 341/341、ASAN、守卫、验收 2/2（20 例） |
| D1-08 | 已合并 | `dcu/D1-08-normal-list` | `c291f3e7` | `docs/evidence/DCU-D1-08/` | 合并提交 `c291f3e7`；混合成败、空列表（表73 无下限）、重复 OAD、验收未知 OI 逐项 DAR；修正 codec 对 n==0 的旧拒绝 |
| D2-09 | 已合并 | `dcu/D2-09-search-meter` | `4838369a` | `docs/evidence/DCU-D2-09/` | 合并提交 `4838369a`；6002 改 Class 11（A2/A3/A6/A10 与 M127/128/129），6003 只作元素类型；无扫描驱动时 M127 回 DAR 2、状态恒空闲，删除假生命周期；对象 37、ACL 36；验收 000024；合并后 342/342、ASAN、守卫、验收 2/2（23 例） |
| D2-05a | 已合并 feat/dcu | `dcu/D2-05a-scheme-management` | `d540979f` | `docs/evidence/DCU-D2-05a/` | 6016-601B 管理平面、ACL 40→44、GET/ACTION、SQLite、DAR 诚实；ASAN leak 修复；验收 2/2（3+25）；Comment Sicko 两项 MUST KILL 已修，op-struct 拆分开放；D2-05b 执行延后；SRP 拆分见 D2-05a-r |
| D2-05a-r | 已合并 feat/dcu | `dcu/D2-05a-r-srp-split` | `0169815f` | `docs/evidence/DCU-D2-05a-r/` | 行为不变拆分：`dcu_698_axdr.c` 1775→1202，新增 `dcu_698_scheme_axdr.c` 517 与 `dcu_698_axdr_internal.h`；SRP、guard 105、ASAN 0 泄漏、验收 2/2 通过；合并后全量 ctest 343/343 |
| D2-06 | 已合并 feat/dcu | `dcu/D2-06-report-schemes` | `42596fb9` | `docs/evidence/DCU-D2-06/` | 601C/601D 管理平面、对象 45、ACL 44（上限 44）；精确字节、持久化、DAR；执行/SnF 停 ADR-0017；验收 2/2（3+26）；合并后 ctest 343/343、guard 105、fanalyzer 3 known/0 new、warning 0；显式 storage 与事务助手单一来源见 D2-06-r；ACL cap 由 D2-07/D2-08 提升 |
| D2-06-r | 已合并 feat/dcu | `dcu/D2-06-r-explicit-storage` | `3e74976f` | `docs/evidence/DCU-D2-06-r/` | 行为不变重构：删除 `dcu_scheme_store_storage()`，report store 自持 static 并由 `dcu_scheme_store_init` 显式 bind；`_exec/_rollback/_begin/_commit` 收敛到内部头 inline 单一来源；测试未改；合并后 clean build 0 warning、ctest 343/343、guard 105/105、验收 2/2（3+26）、fanalyzer 3 known/0 new |
| D2-07 | 已合并 feat/dcu | `dcu/D2-07-rules` | `9799bc05` | `docs/evidence/DCU-D2-07/` | 601E/601F 规则表：三张静态点表 46 行逐值迁入 `dcu_rule` 表（旧表对比 46/46）；601E/601F 管理面（对象 45→47、ACL 44→46、cap 44→46），CSD 按主站 698 OAD（相别行内部影子 tag 不外露）；645 备用 DI 一次重试，台架按 DI 失败验证（主用异常帧→备用成功，继承 scale，窗口严格配对、无第三次请求）；合并后 clean build 0 warning、ctest 344/344、guard 105/105、验收连续 3 次 2/2（3+29）、fanalyzer 3 known/0 new；本机 TSAN 不可运行（mmap_rnd_bits），缺口见 §12.5 |
| D2-08 | 已合并 feat/dcu | `dcu/D2-08-status` | `6eb8755f` | `docs/evidence/DCU-D2-08/` | 6032–6042 七项进对象/ACL（对象 47→54、ACL 46→53、cap 46→53）；6040/6041/6042 真实墙钟；6034/6035 任务执行状态机与按任务收发计数（完成等全部 pending、档案变更按新执行重开、提交失败不计发送）；6032/6033 因相序/中继无来源回 DAR 2（缺口见 §12.5）；合并后 clean build 0 warning、ctest 344/344、guard 105/105、验收 2/2（3+31）、fanalyzer 3 known/0 new |
| D3-02 | 已合并 feat/dcu | `dcu/D3-02-freeze` | `24386465` | `docs/evidence/DCU-D3-02/` | Class 9 冻结调度器：对象 66/ACL 60/cap 60（时钟驱动 5000–5007；切换类 5008–5011 不发布，GET A2 实测 DAR 5）；GET.Record OI→freeze_oad；单 uv_timer ms 边界对齐（align delta 0）、5000–5007 写 `dcu_freeze_history`、5005 每月 1 日结算上月、本地 tm 分/时边界、demo/测试时钟跨零点验收（32 条含 000032）；属性 3 的周期/深度配置归 D3-03；合并后 clean build 0 warning、ctest 345/345、guard 105/105、验收 2/2（3+32）、fanalyzer 3 known/0 new |
| D3-03 | 已合并 feat/dcu（切片 1，缺口由 D3-03b 补齐） | `dcu/D3-03-methods` | `af01d16c` | `docs/evidence/DCU-D3-03/` | 属性 3 的 `{周期, OAD, 深度}` 持久化与配置/默认区分；方法 3/4/5/7/8 与 A3 GET；ACL attr {2,3}/methods {3,4,5,7,8}；调度器消费 OAD 列表与周期；增删 OAD 改变 GetRecord 列（验收 000033）；事务失败回滚、方法 3 延时>0 当时回 DAR 5（D3-03b 已实现）、未绑定 DAR 2；合并后 clean build 0 warning、ctest 345/345、guard 105/105、验收 2/2（3+33）、fanalyzer 3 known/0 new |
| D3-03b | 已合并 feat/dcu | `dcu/D3-03b-attr3` | `20cea519` | `docs/evidence/DCU-D3-03b/` | 属性 3 完整语义：每 (OI, 条目) 独立周期与边界（`last_fire[8][8]`，period=0 不自动）；方法 3 延时秒经固定 8 槽 pending 池由现有 uv 定时器驱动（delay=0 立即，池满 DAR 2，写失败保留并限速告警后重试）；按 (meter, freeze_oad, src_oad) 在写入事务内保留最新 depth 条（depth=0 按 1）；方法 4/7 按“现有 + 本帧去重的新 OAD”计数，超 8 整单回滚回 DAR 3（DAR 8 无 foundation 映射且 `edge_slave_plane.h` 不在白名单）；OAD 唯一走 upsert；历史超限行加载置 `truncated` 并 WARN（恰 9 行不再静默）；新增回归 `test_freeze_per_entry_periods`、`test_freeze_delayed_trigger`、`test_freeze_depth_trim`、`test_freeze_pending_retries_after_failure` 与容量用例；合并后 clean build 0 warning、ctest 352/352、guard 108/108、验收 2/2（3+33）、fanalyzer 3 known/0 new |
| D0-13 | 已合并 feat/dcu | `dcu/D0-13-selfexit` | `02fd2958` | `docs/evidence/DCU-D0-13/` | 产品 60 秒无条件自退定时器改为选择加入：`DCU_SELF_EXIT_SECONDS`（秒），未设/0/非法/负数/前导空白/溢出均为 0，生产与 keep-alive 不再按时间自退；装配移入 `dcu_runtime_self_exit.c`，`main.c` 保持 225/225 组合根基线；验收壳 `dcu_product.py` 转发 SIGTERM/SIGINT 并回收 DCU 子进程（此前平台杀壳后孤儿常驻），新增 `check_acceptance_wrapper_signals.py` 正反夹具并登记 guard；拆除前 `dcu_heartbeat_stop` 停并关闭两个心跳定时器，消除 5 秒 tick 落进 storage 销毁窗口的 SIGABRT；新增 `test_dcu_runtime_self_exit` 8 例与 `test_dcu_heartbeat_stop` 2 例；合并后 clean build 0 warning、ctest 354/354、guard 110/110、验收 r2/r3 连续 2/2（3+33，r1 为已知电压首读 0.0 台架抖动）、fanalyzer 3 known/0 new |
| D4-02a | 已合并 feat/dcu | `dcu/D4-02a-class13` | `e34d9487` | `docs/evidence/DCU-D4-02a/` | Class 13 只读切片：8000 加 A2/A4/A5，对象/ACL 60/60 不变，GET 因无继电器/功控来源回 DAR 2；8001/8100/8101/8102/8103/810E/810F/8110 只进对象模型（66→74），因 ACL 60 项与 1024 字节会话上限不进 Class 20，主站访问回 DAR 3；类定义未暴露→DAR 5、未定义→4、未知 OI→6；方法 127–131 维持诚实拒绝；新增 `test_class13_readonly_dar` 与验收 000034；合并后 clean build 0 warning、ctest 354/354、guard 110/110、验收 2/2（3+34）、fanalyzer 3 known/0 new |
| D0-14 | 已合并 feat/dcu | `dcu/D0-14-accept-stability` | `68a19e71` | `docs/evidence/DCU-D0-14/` | 北向验收首读电压 0.0 抖动的根因是用例在南向首轮抄表完成前断言 DataHub 影子；新增 `_wait_gauge`（100ms 重发 GET、15s deadline、超时带最后值失败）并改造电压、混合列表、电流/功率用例；期望值与容差不放宽，无 skip/retry/固定 sleep；合并后连续 3 次验收 2/2（3+34，64.5/55.2/49.8s）且无 DCU 残留；ctest -R dcu 37/37、guard 110/110、warning 0 |
| D3-08 | 已合并 feat/dcu | `dcu/D3-08-class-tests` | `9ddcdcd1` | `docs/evidence/DCU-D3-08/` | Class 9 12 例（新增属性 3 GET array 编码、方法 4 形状 DAR 7、方法 5 未知 OAD DAR 6、方法 8 非 NULL DAR 7）；Class 24 既有 4 例映射（A11/A12 线上类型、A2/A3/A5/A6/A8/A14、记录与状态时标、SET 回读）；Class 7 无产品实例，钉住 3009/300C/3011/3013 与 300B 回 DAR 6，不造事件；Class 8 复用 9 例（SET 回读、DAR 4/5/6/7、时间非法值不变、无时钟 DAR 2、持久化审计），越权 DAR 3 由 000016 覆盖；验收映射 000032/33、000011/12/13/17、000022；合并后 clean build 0 warning、ctest 354/354、guard 110/110、验收 2/2（3+34）、fanalyzer 3 known/0 new；Class 7 对象缺口记 §12.5 |
| D1-21 | 已合并 feat/dcu | `dcu/D1-21-acl-cap` | `47ad9f47` | `docs/evidence/DCU-D1-21/` | operator 2026-11-24 决定移除 ACL 60 项上限：目标集 201 项（现有 60 + Class 13 非 8000 + Class 22/25/26 + P5 类），60 项列表实测 1970 B，224 项投影 7354 B；`EDGE_SLAVE_ACL_MAX_OBJECTS` 60→224、`EDGE_SLAVE_AXDR_MAX` 2048→10240、`DRIVER_PRIV_SIZE` 1024→4096、manager storage 147456、north adapter 163840；A-XDR 长格式长度编解码两侧、ACL count 改 uint16；foundation 头登记 `core_readonly_whitelist`；io_drivers RAM 实测 147987、预算 163840；6000 A2 因 10240 缓冲返回 40 表完整配置（旧超缓冲 DAR 2 由短缓冲单测继续钉住）；本轮合入 47ad9f47 后合并树 clean build 暴露一条 test 的隐含声明，热修 b2b55ca3 补 `dcu_698_object_internal.h`；合并树 clean build 0 warning、ctest 354/354、guard 110/110、验收安静窗口连续 2 次 2/2（3+34，37.8/48.9s，无残留）、fanalyzer 3 known/0 new |
| D0-16 | 已合并 feat/dcu | `dcu/D0-16-monitor-count` | `dbae60d6` | `docs/evidence/DCU-D0-16/` | 6034/6035 任务监控成功计数的相位缺陷：成功只在 `task_due` 为真的结算 tick 计入，跨 tick 回调的成功被丢弃（一次执行 total=40 而 success=2/20/30）。改为按 `note_visit` 写入的 `visit_new_mask` 归属当前执行（`monitor.status==1`、任务 NORMAL、表 mask 允许），每表每执行计一次；档案重开清 mask/success；失败路径审计无对称丢失；sent/recv 不受影响。新增 `test_success_counts_across_non_due_tick`（到期 tick 提交、非到期 tick 回调，旧码 0、新码 1/2）。合并后 clean build 0 warning、ctest 354/354、guard 110/110、验收安静窗口连续 2 次 2/2（3+34，37.8/48.9s，无残留）、fanalyzer 3 known/0 new |
| D0-15 | 已合并 feat/dcu | `dcu/D0-15-task-monitor` | `33d63e22` | `docs/evidence/DCU-D0-15/` | 先定位到 6034/6035 成功计数的产品相位缺陷（由 D0-16 修复），按 §9.5 停止；随后重写 `test_task_monitor_matches_bench_failures`：以 start 变化等“新执行”，失败执行 20s、恢复执行 60s，单测 `@pytest.mark.timeout(120)`（实测最坏约 90s），断言阈值未放宽；000032 前移避开重试洪峰，000032/000033 对暂时性 OSError/FrameError 有界重试；台架独占窗口连续 3 次 2/2（71.3/44.0/48.9s），合并树再连续 2 次 2/2（48.8/40.5s，无残留）；ctest -R dcu 44/44、guard 110、clean build 0 warning |
| D0-18 | 已合并 feat/dcu | `dcu/D0-18-execution-rootcause` | `2a8d90cb`（经 96389382） | `docs/evidence/DCU-D0-18/` | 验收重复帧/超长的根因：`_chain_load_pending` 每 tick 按 task_due 重装，失败慢趟下同表重抄 4~8 遍（sent 497~2327）。修为一趟一表（visited 守门）、装载先于 note_visit、表级成功位图；新增 `test_execution_polls_each_meter_once`；非 coverage 3 连绿（39.6/78.2/59.2s）、ctest 45/45、ASAN 45/45、guard 118/118、fanalyzer 3 known/0 new。后续对抗评审把该修法推进为显式状态机（见 D0-20） |
| D0-19 | 已合并 feat/dcu | `dcu/D0-18-execution-rootcause` | `734ff37a`（经 2a8d90cb） | `docs/evidence/DCU-D0-19/` | CI 口径（COVERAGE=ON）下 000031 固定窗口命中执行中段：改为完成锚定（等 start 变化，再等该 start 的 end 非空且计数到位），恢复段接受任意“恢复后已完成 success==40”的执行；000029 按 bus `at` 聚簇还原单次执行；`@timeout(240)` 仅时间预算，断言不变；coverage 3 连绿（44.3~53.5s）、release 1 绿、ctest 45/45、guard 284/284 |
| D0-20 | 已合并 feat/dcu | `dcu/D0-18-execution-rootcause` | `2a8d90cb` | `docs/evidence/DCU-D0-20/` | 对抗评审 F1/F2/F3 + 重设计：每任务 IDLE/ACTIVE/DONE、begin 快照 target 位图（succeeded⊆settled⊆target，success≤total 无条件）、完成条件为 target 全部 settled、单一资格谓词贯穿 load/note/记账/完成、exec_gen 阻止旧访问结算新执行；熔断 CLOSED/OPEN/HALF_OPEN + 30s 冷却 + 601E 重配复位；F6 真因是 `IO_EVENT_RX` 载荷双重 unref（引擎回调内释放 + io 句柄返回后释放），回调栈内同步派发的 TX 缓冲被释放后覆盖，同帧写到多通道 socket——契约改为借用语义（`io_executor.h`），master/slave 去掉回调内 unref，passthrough 异步写前 ref；回归 `test_rx_payload_outlives_engine_callback` 修复前红/修复后绿；修复后 bus 跨通道重复帧 0（reads 6329→2624，distinct 600=40 表×15 tag）、验收 coverage 安静窗口 3 连绿（3+35，22.9/22.7/22.7s）、ctest 368/368、ASAN 受影响集 19/19、guard 284/284、fanalyzer 3 known/0 new；F4/F5 见 §12.5 |
| F5 | 已合并 feat/dcu | `dcu/F5-late-callback-write` | `08f59c6a` | `docs/evidence/DCU-F5/` | 档案 revision 成为访问与重试的边界：pump 在 revision 已变而 step 未观测时不再续发旧访问（迟到结果仍按快照落库）；bus_source 备用 DI 重试只在提交时 revision 不变时放行，slot 耗尽复用不再放行；读身份快照 address/slot 装载时冻结、tag/mask/exec 提交时写入；删除 `dcu_chain_t.depth` 死旋钮，在途读固定 1；新增 4 例（revision 交错、移位未删不记假失败、slot 复用不重试 3 红，revision 不变重试仍工作 1 正例）；verifier 复跑 27/27、`ctest -R 'dcu|dlt698'` 45/45、coverage 验收 3 连绿（2/2 任务，35 passed，22.80/22.97/22.94s）、ASAN 27/27、守卫自检全绿、warning 0 |
| D5-05 | 已合并 feat/dcu | `dcu/D5-05-class14-16-stats` | `a61e7e61` | `docs/evidence/DCU-D5-05/` | Class 14/15/16 的 15 个 OI（2100–2104/2110–2114/2120–2124）空掩码对象行 + 只读 ACL（属性 1..3、方法 1..4），Class 16 max_method 按表171 修正 2→4；GET 按类定义 DAR 5、类未定义 DAR 4、主站未进 ACL DAR 3、未知 OI DAR 6，方法与 SET 不伪造成功；对象 101、ACL 87（76 手写 + 11 参数宏），对象列表 3011 B、会话 1392 B，在 224/10240/4096 内；无统计生产者，计划原“GET 返回值”出口改为 DAR 5 诚实骨架，生产者缺口见 §12.5；verifier 红 5/45→绿 45/45、coverage 验收 3 连绿（2/2，35 passed，27.15/22.79/22.53s）、ASAN 目标集绿、守卫全绿、全量 ctest 367/367、warning 0、fanalyzer 3 known/0 new |
| F4 | 已合并 feat/dcu | `dcu/F4-pending-capacity` | `c6dcbccd`（merge 253ed19d） | `docs/evidence/DCU-F4/` | 单表任务并集超容量改为容量内分批，每任务每 CSD tag 每执行恰好一次；执行身份 {exec_gen, scheme_id, item_mask} 界定 served 与失败记录，6012 换方案只重开被改任务；熔断与通道成功统计在整表分批结束后结算一次，任一批失败即整表失败；新增 8 例（容量饿死、跨批共享、跨 step、整表失败、方案切换只重开该任务、无关任务 exec_gen 不变、跨代失败不污染、混合批次熔断）；verifier 复跑 34/34、45/45、coverage 验收 3 连绿（run1 台架未完成已归因）、ASAN 34/34、合并后守卫全绿、全量 ctest 367/367、fanalyzer 3 known/0 new、warning 0；HOLD：`dcu_poller_get_scheme` 可变指针（F4R 已覆盖 upsert_scheme 路径，生产 store 只改未引用方案） |
| D5-06 | 已合并 feat/dcu | `dcu/D5-06-class17-display` | `af263dc7`（merge f25d390f） | `docs/evidence/DCU-D5-06/` | Class 17 显示：F300/F301 空掩码单例对象行 + 只读 ACL（属性 2..4、方法 1..6，A1 不广告）；无显示配置来源，GET 按类定义 DAR 5、类未定义 DAR 4、主站未进 ACL DAR 3、未知 OI DAR 6，方法与 SET 不伪造成功；对象 103、ACL 89（78 手写 + 11 参数宏），对象列表 3141 B、会话 1424 B，在 224/10240/4096 内；计划原“GET F300 空轮显表”出口改为 DAR 5 诚实骨架，显示配置 producer 缺口见 §12.5；verifier 红 5/45→绿 45/45、coverage 验收 3 连绿（2/2，35 passed，22.76/22.48/22.69s）、ASAN 目标集绿、合并后守卫全绿、全量 ctest 367/367、warning 0、fanalyzer 3 known/0 new |
| D5-03 | 已合并 feat/dcu | `dcu/D5-03-class11-collection` | `092304cd`（merge 0066b073） | `docs/evidence/DCU-D5-03/` | Class 11 集合：7000（OI_FILE_SET）无属性、方法 127–129；7010（OI_SCRIPT_SET）A2 只读、方法 127–130；per-OI `k_oi_max_method` 记录实例方法，Class 11 类上限仍为 2，不误广告其他集合 OI；无文件/脚本集合存储，7010 A2 与 7000 方法按类定义 DAR 5、类未定义 DAR 4、主站未进 ACL DAR 3、未知 OI DAR 6，不伪造集合；对象 105、ACL 91（80 手写 + 11 参数宏），对象列表 3211 B、会话 1456 B，在 224/10240/4096 内；计划原“GET 7000 返回集合元素”按标准改为 7010 A2 + 7000 方法 DAR 5 骨架，集合 producer 缺口见 §12.5；verifier 红 5/45→绿 45/45、coverage 验收 3 连绿（2/2，35 passed，22.60/22.67/22.62s）、ASAN 目标集绿、合并后守卫全绿、全量 ctest 367/367、warning 0、fanalyzer 3 known/0 new |
| F4R | 已合并 feat/dcu | `dcu/F4-scheme-identity` | `60a13c85`（merge 168ab797） | `docs/evidence/DCU-F4R/` | 方案内容代际 `content_gen` 纳入执行身份；同 scheme_id 下 CSD 内容变化时递增代际并只重开引用该方案的 ACTIVE 任务，内容相同不重开；旧 served 位图不跨版本；新增 `test_scheme_content_change_reopens_referencing_execution`；verifier 红 1/35→绿 35/35、45/45、coverage 验收 3 连绿（2/2，35 passed，22.85/22.80/22.79s）；残留 `dcu_poller_get_scheme` 可变指针与 ms_count 边界记 HOLD |
| DCU-MUTATION-F4F5 | 已合并 feat/dcu | `dcu/mutation-guards-invariants` | `c8bf2eff`（merge caaf28be） | `docs/evidence/DCU-MUTATION-F4F5/` | 四条 F4/F5 不变量进 `ci/mutation_guards.tsv`：batch continuation、served identity、pump revision fence、spare retry revision；每条变异后目标测试红、恢复后绿；`run_mutation_guards.py --selftest` 绿，4/4 guards sensitive；产品零改动 |
| F5R | 已合并 feat/dcu | `dcu/F5-device-identity` | `9635814b`（merge 9ab59c23） | `docs/evidence/DCU-F5R/` | 事件监控、日志、冻结与 record reading 统一用稳定 slot_id，经 `dcu_inventory_by_slot` 解析；删除重排后事件/冻结断面/读数不再落到顶替下标的表；`dcu_node_record_reading` 端口参数改为 slot_id；新增两个移位回归；ASAN 目标集无泄漏（`9635814b` 释放 stage 描述符）；verifier 红 3/4+1/2→绿 4/4+2/2、45/45、coverage 验收 3 连绿（2/2，35 passed，22.75/31.94/22.57s）、合并树守卫/全量 ctest 367/367/fanalyzer 3 known/0 new/warning 0；残留 heartbeat demo 下标、var 快照元素序号、slot 复用状态见 §12.5 |
| D5-04 | 已合并 feat/dcu | `dcu/D5-04-class12-partial` | `7c9accd0`（merge efd887a0） | `docs/evidence/DCU-D5-04/` | Class 12 脉冲计量：2401 单例空掩码对象行；ACL 属性 1..15 只读、方法 1..4；16/17 按类定义表157 视为已定义但 ACL 不广告（表157/158 冲突 HOLD），18/19 超 ACL 属性 schema 不广告；GET 已广告属性 DAR 5、A20 DAR 4、未知 OI DAR 6、主站 ACL DAR 3；对象 106、ACL 92（81 手写 + 11 参数宏），对象列表 3336 B、会话 1472 B，在 224/10240/4096 内；无脉冲 producer，计划原“GET 2401 返回值”改为 DAR 5 骨架，2402-2408 与 producer 缺口见 §12.5；verifier 红 5/45→绿 45/45、coverage 验收 3 连绿（2/2，35 passed，23.19/22.28/17.60s）、ASAN 目标集绿、合并后守卫全绿、全量 ctest 367/367、warning 0、fanalyzer 3 known/0 new |
| P4-RELAY | 已合并 feat/dcu | `dcu/P4-relay-map` | `4125fdd2`（merge 5bcfdeda） | `docs/evidence/DCU-P4-RELAY/` | OI 8000 方法 129–131 的继电器 OAD 经产品点位表接口解析：`dcu_relay_map_t`（固定 32 项）由 `dcu_node` 聚合根拥有，`dcu_node_set_relay` 注入，节点销毁解绑；空表回 DAR 2 不变，命中后真实 DO 未接线仍回 DAR 5，不伪造成功；变异 A（命中改回 DAR 2）2 例红、变异 B（空表改 DAR 5）5 例红；verifier 绿 45/45（105.90s）、验收 3 连绿（2/2，35 passed，18.23/22.74/23.09s，Release build-cov 未开 COVERAGE 旗标）、合并树守卫全绿、全量 ctest 367/367、ASAN 47/47+5/5、fanalyzer 3 known/0 new、warning 0；点位表数值与真实 DO 执行缺口见 §12.5 |
| F4-ADDTASK | 已合并 feat/dcu | `dcu/F4-addtask-targeted` | `93877f9e`（merge 77dd8e1e） | `docs/evidence/DCU-F4-ADDTASK/` | `dcu_poller_add_task_ex` 不再调用 `_poller_reopen_active`：新增任务只追加槽位，旧 ACTIVE 任务的 exec_gen、visited/settled 与在途批次保留，新任务按自己的 due 首次 begin；链上 served_ident/exec_gen fence 不变；新增 `test_add_task_does_not_reopen_active_execution`（旧码 exec_gen `2 != 1`）；verifier 红 1/36→绿 36/36、45/45、验收 3 连绿（2/2，3+35 passed）、合并树 build/warnings/守卫全绿；全量 ctest 一条非 DCU app 管理墙钟用例在负载 24 下红、单独复跑 8.50s 绿，归因主机负载 |
| F5S | 已合并 feat/dcu | `dcu/F5-snapshot-identity` | `4848b34a`（merge 5b446fcc） | `docs/evidence/DCU-F5S/` | 事件 A6 关联数据快照按事件自己的稳定 slot 身份取表：`_capture_record_snapshots` 发生/恢复两处传 `ev->device_id`，编码走新 `dcu_698_var_encode_axdr_device`；OAD 低字节只作属性/元素选择，北向 GET 旧编码器语义不变；原型单一来源 `dcu_698_var.h`（返工删除两处本地声明）；测试断言 D 表 1500/1510/1520（旧码从 slot 0 取到 1000，`1000 != 1500`）；verifier 红 1/2→绿 2/2、45/45、验收 3 连绿（2/2，3+35 passed）、对抗评审 NO FINDINGS（gdb 观察发生+恢复 device=slot，旧编码器在快照路径零命中）、ASAN targeted 2/2+47/47；恢复记录 start_time 的既有身份缺口见 §12.5 |
| F5-HEARTBEAT | 已合并 feat/dcu | `dcu/F5-heartbeat-slot` | `2c57aff2`（merge 7dbbeb54） | `docs/evidence/DCU-F5-HEARTBEAT/` | DCU_DEMO_MODE 心跳的冻结触发与演示跳闸改用 `inv->meters[i].slot_id`，删除重排后不再错表/丢冻结；修复为两处替换 + 一条 why 注释；新增 `test_demo_freeze_trigger_uses_slot_identity`（删 slot 0 后旧码 `0 != 3`），登记变异守卫 `DCU-F5-heartbeat-slot`（改回下标即红）；verifier 红 1/4→绿 4/4、45/45、验收 5 轮 4 绿（run2 零点对齐 208ms 与并发 ctest 同窗，已归因）；合并树全量 ctest 367/367、守卫自检全绿、变异守卫 1/1 sensitive、ASAN `test_dcu_runtime_health` 绿、fanalyzer 3 known/0 new |
| D4-10 | 已合并 feat/dcu | `dcu/D4-10-comm-params` | `4b242540`（merge a926964b，热修 98eb5115） | `docs/evidence/DCU-D4-10/` | Class 25/26（4500/4501、4510–4517）只读 DAR 骨架；唯一真实来源是平台第一个物理单播以太网口 MAC：`dcu_comm_info_scan(net_root)` 判据 type=1、无 wireless、无 bridge/、有 device 总线链接、I/G=0、非全零，4510 A5 编 octet-string(6)，容器 veth 不冒充；其余类定义属性 DAR 5、类未定义 4、未知 OI 6、未进 ACL 3；SET DAR 3、方法 1 DAR 5；对抗评审三条缺陷（组播、bridge、veth）以 sysfs 夹具修复，独立变异逐条证红（组播 2 例、wireless/bridge/device 各 1 例），恢复 53/53；对象 116、ACL 102（91 手写 + 11 参数宏），对象列表 3878 B、会话 1632 B；verifier 验收 3 连绿（2/2，3+35 passed）、合并后守卫/变异守卫/全量 ctest 367/367、ASAN 目标集绿、fanalyzer 3 known/0 new（热修 `best[256]` 消一条 format-truncation）、warning 0；多口序号排序与通信参数写/持久化缺口见 §12.5 |
| D1-10A | 已合并 feat/dcu | `dcu/D1-10a-oi-methods` | `414298de`（merge 632783b7） | `docs/evidence/DCU-D1-10A/` | 目录夹具新增 per-OI 附录 A 方法集合（31 OI/86 行）与 7.3 类表自身上限 `max_class_method`（JSON `oi_methods`、Markdown 附录 A 一节）；新增 `test_methods_stay_within_the_oi_definition` 拒收方法放错同类 OI（7000 广告 130 而 130 属 7010），按类 max 检查保留；登记变异守卫 `DCU-D1-10A-wrong-oi-method`；产品代码零改动；verifier 复跑 test 5/5、目录门禁一致、变异 red rc=8→green 1/1、合并树守卫与全量 ctest 见证据；7.3 内部跳号仍留 §12.5 |
| D1-10B | 已合并 feat/dcu | `dcu/D1-10B-line-window` | `24d8f39b`（merge 8127a473） | `docs/evidence/DCU-D1-10B/` | 目录生成器改为按 `附 录 A`/`附 录 B` 与 `表A.1`..`表A.14` 标题定位，缺/重复标题非零退出；140 行偏移下旧码静默 512 OI（漏 F300/F301），新码 514 OI 且真实输入生成物逐字节不变；新增 `tests/test_dcu_698_catalog_source_window.py` 6 例与变异守卫 `DCU-D1-10B-fixed-line-window`；产品 C 零改动；verifier 复跑 6/6、`--check`/目录门禁 0、变异 red rc=1→green 1/1、合并树守卫与 ctest 见证据 |
| D4-04 | 已合并 feat/dcu | `dcu/D4-04-class19` | `d4051496`（merge 88e1e550） | `docs/evidence/DCU-D4-04/` | Class 19（4300、4307–4309）空掩码对象行 + 只读 ACL（A1..A10、方法 1..6）；GET 类定义 DAR 5、类未定义 4、未知 OI 6、未进 ACL 3；方法 1..6 无处理器 DAR 5、SET DAR 3，不执行复位/初始化；对象 120、ACL 106（95 手写 + 11 参数宏），对象列表 4306 B、会话 1696 B；两轮对抗评审：①106 项列表 4328 B 写爆 4096 共享池并写坏下一 net_buf 节点 → 从站独立 10496×32 池 + codec 全路径 tailroom 守卫 + 泄漏修复；②列表尾未预留导致 10427 B 半帧被发送且重发 → item 预留 5 B、失败清长度、引擎只发已封口帧；verifier 骨架红 5/45→绿，越界轮红 25/26→绿 26/26，帧尾轮红 2/28→绿 28/28、45/45；变异守卫 `DCU-D4-04-list-trailer-reserve` 1/1；验收两轮各 3 连绿（2/2，3+35 passed）；合并树守卫 + 全量 ctest 367/367；127..129 ACL 容量与 write_async 契约缺口见 §12.5 |
| D4-07 | 已合并 feat/dcu | `dcu/D4-07-esam` | `b0cd3b74`（merge cd4adcd0） | `docs/evidence/DCU-D4-07/` | Class 21 只注册目录事实 F100（F101 属 Class 8，按目录纠正任务单口径）；F100 空掩码 + 只读 ACL（A1..A15、方法 1..8，9..12 因每对象方法槽 8 记 HOLD）；删除 698 面软件 ESAM 复位处理器 `dcu_698_class_security.c`，ACTION 不再改动会话/密钥/认证态；方法走 resolver DAR 5、主站未广告的 9..12 先 DAR 3；GET A1..A15 DAR 5，不返回序列号/版本/MAC/密钥；SET DAR 3；对象 120、ACL 107（96 手写 + 11 参数宏），对象列表 4455 B；verifier 红 6/45→绿 45/45、变异守卫 `DCU-D4-07-esam-attr15-advertised` 1/1、验收 3 连绿（2/2，3+35 passed）；对抗评审 F1 裁决为 ACL 优先的既有口径，DEVIATIONS 已写清主站/resolver 两层 DAR；合并树门禁见证据 |
| CLEANUP-CODEC | 已合并 feat/dcu | `dcu/cleanup-codec-tags` | `7f3bcf71`（merge a106bec7） | `docs/evidence/DCU-CLEANUP-CODEC/` | 行为等价重构：`dlt698_codec.c` 的 9 处已有常量的标签字面量换 `DLT698_TAG_*`（`18`/`5`/`28`、`0x1C`/`0x00`/`0x54`）；`0x19`（date_time）无常量，保留并原位注明；verifier 独立重建 `edge_codec` 对象 sha256 前后一致（`eaa6b7c4…`）、`ctest -R 'dlt698|driver_dlt698'` 7/7、diff 仅 9 行替换 + 1 行注释；合并树门禁见证据 |
| D4-09 | 已合并 feat/dcu | `dcu/D4-09-group` | `50ab80ca`（merge 814f1de9） | `docs/evidence/DCU-D4-09/` | Class 23 总加组（2301..2308）空掩码对象行 + 只读 ACL（A1..A16、方法 1..5；A17/A18 因 ACL 属性 schema 上限 16 记 HOLD）；GET 已广告属性 resolver DAR 5、A17/A18 5、A19 4、未知 OI 6，主站未广告一律 DAR 3；方法 1..5 DAR 5 绝不执行，SET DAR 3；不伪造总加值/滑差/轮次；对象 128、ACL 115（104 手写 + 11 参数宏），列表 5551 B、帧后 5573 B < 10432 usable；verifier 红 5/45→绿 45/45、变异守卫 `DCU-D4-09-group-attr16-advertised` 1/1、验收 3 连绿（2/2，3+35 passed）、对抗评审 NO FINDINGS（gdb 复核 5551 B/115 项与 DAR 矩阵）；合并树门禁见证据 |
| D4-06 | 已合并 feat/dcu | `dcu/D4-06-file` | `4ab86856`（merge e6bd6a94） | `docs/evidence/DCU-D4-06/` | Class 18 文件传输 F000/F001/F002 空掩码对象行 + 只读 ACL（F000 A1..A5、F001/F002 A1..A4、方法 1..8；F001 9/10 计入 resolver 定义但因方法槽 8 记 HOLD）；GET 已定义属性 resolver/主站 DAR 5、未定义 4/3、未知 OI 6/3；ACTION 1..8 DAR 5 绝不传输/校验/激活/回滚；SET DAR 3；对象 131、ACL 118（107 手写 + 11 参数宏），列表 5806 B、帧后 5828 B < 10432 usable；verifier 红 5/45→绿 45/45、变异守卫 `DCU-D4-06-file-oi-methods` 1/1、验收 3 连绿（2/2，3+35 passed）、对抗评审 NO FINDINGS（per-OI 例外无泄漏、DAR 矩阵与计数复核）；合并树门禁见证据 |
| D1-02 | 已评审通过（五轮独立评审） | `dcu/D1-02b-design-doc` | `5f7dd711` | `docs/evidence/DCU-D1-02/` | 设计文档按实现事实重写：已实现态 §3–§9（对象 74/ACL 60/14 个非 ACL、DAR 优先级含 ACL 3 优先与 SET/ACTION `-1` 区分、真实文件/函数落点、真实存储表与 store 归属、生命周期/实例绑定、D1-21 224/10240/4096 非设计常量）与目标态 §10 分离；26 类等级 + 落点/归属任务齐全；Class 5/8/11/13/20/21/22/24 归属按 A.7/A.9/目录校正；F200 对象行与 A4 SET DAR 5 如实记录；五轮评审的 13+14+4+4 项全部修正；未决：Class 12 表157/158 标准内部冲突待 PDF 所有者裁决，4000 A2 SET DAR 与验收条款不一致另立缺陷（§12.5） |
| D1-09 | 已合并 | `dcu/D1-09-name-source` | `f51fe322` | `docs/evidence/DCU-D1-09/` | 合并提交 `f51fe322`；名字随影子 tag 放进 `var_element_t {tag, name}`，`dcu_north_source.c` 只转发；删掉两个无对象行的死名（ZERO_SEQ_CURRENT、REACTIVE_POWER_TOTAL）；双向对拍测试；合并后 342/342、ASAN、守卫、验收 2/2（22 例） |
| D1-10 | 已合并 | `dcu/D1-10-catalog-consistency` | `5a4fda20` | `docs/evidence/DCU-D1-10/` | 合并提交 `5a4fda20`；生成 514 OI + 26 类夹具，表→目录严格对拍、mask 范围、OI 唯一；红色对照 F200 改类；合并后 29/29、目录门禁、守卫、验收全过 |
| D1-11 | 已合并 | `dcu/D1-11-acceptance` | `7f5ce300` | `docs/evidence/DCU-D1-11/` | 合并提交 `7f5ce300`；GET/SET/ACTION/Record/Class20 共 16 例独立验收；修复过期 ACL 对象列表（A11/A12/F200 A4），ACL 属性上限 8→12 |
| D0-12 | 已合并 | `dcu/D0-12-acceptance-gate` | `e8b242bc` | `docs/evidence/DCU-D0-12/` | 合并提交 `e8b242bc`；`acceptance/run.sh` 记录逐任务退出码，`check_acceptance_tasks.py` 按 tasks 目录核对；正反夹具进守卫自检 |
| D1-20a | 已合并 | `dcu/D1-20a-action-route` | `564ff4c5` | `docs/evidence/DCU-D1-20a/` | 合并提交 `564ff4c5`；非 8000 ACTION 走对象表，6012 M127 严格 A-XDR，短帧 DAR 7 不改状态；TimeTag 有值三处统一 DAR 7（不静默忽略）；000019 作北向 19 例；合并后 340/340、ASAN 12/12、守卫、验收 2/2 |
| D3-05a | 已合并 | `dcu/D3-05a-event-records` | `3e291b74` | `docs/evidence/DCU-D3-05a/` | 合并提交 `3e291b74`；A2 关联 OAD 数组、A3 四表计数、A4/A11/A12 真实；A5 配置与 A6/A14 记录因来源不成立撤 mask 回 DAR 5 |
| D3-04a | 已合并 | `dcu/D3-04a-event-wallclock` | `34271061` | `docs/evidence/DCU-D3-04a/` | 合并提交 `34271061`；date_time_s 设备本地公历单 helper（冻结与 A6/A14 共用，测试 `TZ=CST-8`）；A6 未结束编表 49 非法值 1C FF×7，A14 编 NULL；旧 event_history 行不渲染；合并后 340/340、ASAN 12/12、守卫、验收 2/2 |
| D5-01/02 | 部分合并 | `dcu/D5-01-02-readonly` | `febc5345` | `docs/evidence/DCU-D5-01-02/` | 合并提交 `febc5345`；类 2/5 无数据源，登记 1010–1013、200D/200E 空掩码 + 只读 ACL，定义未广告回 DAR 5、类外回 DAR 4、未知 OI 回 DAR 6；不广告任何值；需量/频谱 producer 列为后续 |
| D2-04 | 已合并 | `dcu/D2-04-schemes` | `dc1778e5` | `docs/evidence/DCU-D2-04/` | 合并提交 `dc1778e5`；6014 A2/M127-130 与 6015 A2 由 `dcu_scheme_config/csd/meta` 事务化落地；6014 走严格解码入口，套用 TimeTag 规则；ACL 上限改为 `EDGE_SLAVE_ACL_MAX_OBJECTS` 单一来源，31/32；000020 北向 21 例；合并后 342/342、ASAN、守卫、验收 2/2 |
| D1-12 | 已合并 | `dcu/D1-12-axdr-port-adr` | `076e0e04` | `docs/evidence/DCU-D1-12/` | ADR-0013 已合并，状态 Proposed；14 个初始化点全为 designated，无位置初始化；架构签字待补 |
| D1-13 | 已合并 | `dcu/D1-13-axdr-port` | `68f4de69` | `docs/evidence/DCU-D1-13/` | 合并提交 `68f4de69`；合并后 28/28、守卫、验收 2+4 全过；流式逐项编码，单缓冲复用，无 8×2048；`/no-comments` 删 23 行、DAR 字面量改常量 |
| D1-14 | 已合并 | `dcu/D1-14-set` | `b5b95f62` | `docs/evidence/DCU-D1-14/` | 合并提交 `b5b95f62`；SET-Request、`accept_axdr` 写入、SET-Response DAR；拒绝非标量回落假成功；LONG64 无 UB；DENIED 3 与 CLASS_MISMATCH 5 分开；合并后告警、28/28、ASAN、守卫、验收全过 |
| D1-14a | 已合并 | `dcu/D1-14a-be32` | `e2ce657d` | `docs/evidence/DCU-D1-14a/` | 合并提交 `e2ce657d`；`_read_be32` 统一 5 处大端读取；高字节 ≥0x80 用例；合并后 UBSAN 全量 332/332、0 runtime error |
| D1-15 | 已合并 | `dcu/D1-15a-record-codec` + `dcu/D1-15b-record-driver` | `7658967a` | `docs/evidence/DCU-D1-15a/`、`DCU-D1-15b/` | D1-15a codec 层 GetRecord/RSD/RCSD；D1-15b ADR-0014 记录端口、Selector1、驱动 GET [3]、分页；产品记录取数留 D1-07 |
| D1-15b | 已合并 | `dcu/D1-15b-record-driver` | `7658967a` | `docs/evidence/DCU-D1-15b/` | 合并提交 `7658967a`；端口三回调协议中立；行失败整条回 DAR；>255 回 DAR 2；不支持选择方法空 RCSD 限制已入 ADR |
| D1-16 | 已合并 | `dcu/D1-16-report` | `479017fd` | `docs/evidence/DCU-D1-16/` | 合并提交 `479017fd`；ADR-0017 上报端口、CHOICE[1] 列表、确认匹配、5 秒超时解锁；生产队列在 A3/记录列表就绪前保持空；34/34、守卫、验收通过 |
| D1-18e | 已合并 | `dcu/D1-18e-freeze` | `c8caeb8a` | `docs/evidence/DCU-D1-18e/` | 合并提交 `c8caeb8a`；5004 属性/方法位全部撤回，GetRecord 是唯一记录路径；验收 2 任务 3+17 例；方法 3 受 north adapter 非 8000 ACTION 路由限制仍不可达 |
| D0-12 | 待派 | - | - | - | 验收门禁假绿：任务加载失败（退出 4）仍输出“全部合格:1 个验收任务”并以 0 退出；需按 tasks 目录核对任务数与非零退出 |
| D1-18c | 已合并 | `dcu/D1-18c-acq-shapes` | `78fd6083` | `docs/evidence/DCU-D1-18c/` | 合并提交 `78fd6083`；6000/6002/6003 按标准形状撤 mask，6012 reset 真实，AddScheme 移到 127；DAR 5/4 规则统一；合并后目标测试重跑绿 |
| D0-11 | 已合并 | `dcu/D0-11-test-tmp` | `174d6bf4` | `docs/evidence/DCU-D0-11/` | 合并提交 `174d6bf4`；4 个 DCU 测试改每进程唯一路径，新增并发双跑守卫；合并后 36 目标测试与验收绿；另有 10 个非 DCU 测试固定路径待清理 |
| D1-17 | 已合并 | `dcu/D1-17-appconn` | `5494f506` | `docs/evidence/DCU-D1-17/` | 合并提交 `5494f506`；ADR-0015 会话与 ACL；CONNECT/RELEASE 无 CHOICE 解析；未授权 DAR 3；预连接窗口按产品列表；32/32、ASAN、守卫、验收通过 |
| D1-18 | 审计完成 | 只读 | `4f864e29` | `docs/design/DCU_698_OAD_STATUS.md` | 56 个已暴露位：0 real、37 partial、12 stub、3 unimplemented、4 masked-but-undefined；按 P0/P1 拆成 D1-18a 到 D1-18d |
| D1-18a | 已合并 | `dcu/D1-18a-variable-shapes` | `8acf47fe` | `docs/evidence/DCU-D1-18a/` | 合并提交 `8acf47fe`；Class 1/3/4 attr2 标准数组、attr3 Scaler_Unit、整数类型、元素索引选槽位；修正错误 tag 常量；合并后 32/32、ASAN、守卫、验收通过 |
| D1-18b | 已合并 | `dcu/D1-18b-event-class24` | `e5906b65` | `docs/evidence/DCU-D1-18b/` | 合并提交 `e5906b65`；3000/3005 attrs 收为 {4,11,12}，A11 enum、A12 bool 真实；未暴露的类定义属性回 DAR 5，越类回 DAR 4；reset 真实清记录 |
| D1-19 | 已合并 | `dcu/D1-19-split` | `7687612d` | `docs/evidence/DCU-D1-19/` | 合并提交 `7687612d`；对象模型拆为 object_model 279 行 + acq/event/control/security 四个类族文件；公开 API 不变；合并后 32/32、守卫、验收通过 |
| D6-08a | 已合并 | `dcu/D6-08a-tsan-lock` | `6509c68f` | `docs/evidence/DCU-D6-08a/` | 合并提交 `6509c68f`；SQLite 内部锁序对三个 storage 用例定向抑制，第一方 pthread 锁序 A/B 仍会报；TSAN 目标 32/32、0 行 |
| D2 到 D5 | 阻塞 | - | - | - | 依赖 COM 组件，见 11.3 B6 |
| D6-08 | 已完成 | `main` | `6509c68f` | `docs/evidence/DCU-D6-08/` | ASAN 334/334、UBSAN 334/334（0 runtime error）、TSAN 334/334（0 行）、fanalyzer 3 known 0 new |
| D6-09 | 部分合并 feat/dcu | `dcu/D6-09-coverage-floor` | `cfcfdd6d`（merge `acc14dd8`） | `docs/evidence/DCU-D6-09/` | Release/COVERAGE 构建 + ctest 380/380 + lcov 实测 `cmd/dcu` 6112/7039=86.8305%，新增下限 86.8（不高于实测）；`cmd` 拆分后 78.2% 仍高于 76.5%，无既有下限降低；`foundation/src/protocol` 无插桩（foundation 静态库未链接 `edge_common_interface`，0 个 `.gcno`），无法设下限，另立 D6-09b |
| D6 | 未开始 | - | - | - | 随各阶段并行；D6-08 已完成 |
| P3-4 | 已合并 feat/dcu | `dcu/P3-4-event-start-time` | `0fe4adca`（merge f02ed19e） | `docs/evidence/DCU-P3-4-EVENT-START/` | 恢复记录按 slot_id 匹配同表发生记录取 start_time；A14 仍每 OI；发生记录被窗口挤出时丢弃恢复并只更新 last_end_time；verifier 红 `2001000000000 != 1001000000000`→绿 45/45、验收 3 连绿、变异守卫 2 条（start-slot / archive-start）红/绿、ASAN 5/5、对抗评审两轮无功能性 finding 并补 archive/eviction 守卫；合并树 373/373、fanalyzer 5 known/0 new |
| P3-7 | 已合并 feat/dcu | `dcu/P3-7-date-time-tag` | `169c4946`（merge 4ce21c74） | `docs/evidence/DCU-P3-7-DATE-TIME-TAG/` | 新增 `DLT698_TAG_DATE_TIME = 25`；codec/axdr/class_event 的 date_time/date_time_s 标签字面量全部换常量；codec 对象 sha256 前后一致 `eaa6b7c4…`；45/45、验收 3 连绿；纯等价清理无对抗评审 |
| P3-9 | 已合并 feat/dcu | `dcu/P3-9-heartbeat-demo-slot` | `7877146e`（merge 04baf768） | `docs/evidence/DCU-P3-9-HEARTBEAT-DEMO-SLOT/` | demo 控制 tick 改为档案第一只表的稳定 slot_id，空档案跳过；首版按地址 10000001 被验收 3/3 红打回（台架表 000000000001），重做后红 `0 != 1`→绿 45/45、验收 3 连绿、变异守卫 1 条红/绿；合并树 373/373 |
| P3-8 | 已合并 feat/dcu | `dcu/P3-8-catalog-gaps` | `7da6226e`（merge eb3312e6） | `docs/evidence/DCU-P3-8-CATALOG-GAPS/` | 生成器按 7.3 真实方法集合校验（Class 7/24 无 3、Class 9 无 6），OI 方法 <127 必须属于类集合或 F001 {9,10} 例外；缺号/多号/重复/越界退 1；真实源生成物逐字节一致；新测试注册进 guard selftest；变异守卫 1 条红/绿；运行时 max 语义另立 |
| P3-1 | 已合并 feat/dcu | `dcu/P3-1-acl-cap` | `fb96cfef`（merge a33af605） | `docs/evidence/DCU-P3-1-ACL-CAP/` | 每对象 ACL 16→12 B、attr 1..18/5 B、方法 12/对象、会话 1024 B 方法池 fail closed；Class 18 F001 9/10、Class 19 4307-09 127-129、Class 21 F100 9-12、Class 23 2301-08 A17/A18 已广告并走 resolver DAR 5；预算会话 3752、对象列表 6014，224/10240/4096 未抬高；对抗评审抓出池溢出测试假绿，已修测试并补池界变异守卫；合并树 373/373、ASAN 32/32、fanalyzer 5 known/0 new |
| P3-2 | 已合并 feat/dcu | `dcu/P3-2-write-async` | `c741091e`（merge 894c038e） | `docs/evidence/DCU-P3-2-WRITE-ASYNC/` | 统一 write_async 契约：调用方返回后立即 unref；poll/UDP/HTTP 成功时自持引用到完成/失败/destroy；同步消费不持；rc<0 不接管；对抗评审抓出 HTTP 完成回调 destroy 的 UAF/二次释放与 CURLM_ADDED_ALREADY 正 rc，均已修并补测试/守卫；ASAN 17/17；合并树 373/373 |
| P3-5 | 已合并 feat/dcu | `dcu/P3-5-var-4510` | `607eedb4`（merge 9c45f835） | `docs/evidence/DCU-P3-5-VAR-4510/` | 标量端口 `dcu_value_t` 无 octet-string，4510 A5 按 DEVIATIONS 无来源口径回 DAR 5（旧 DAR 7）；主站 GET 仍 octet-string(6)+MAC；红 `-22 != -21`→绿 45/45；验收 3 连绿；变异守卫 1 条；合并树 373/373、ASAN 103 测试无报告 |
| P3-9 | 已合并 feat/dcu | `dcu/P3-9-runtime-methods` | `c0449106`（merge 21c84480） | `docs/evidence/DCU-P3-9-RUNTIME-METHODS/` | 运行时类方法集合改为真实 7.3 集合（Class 7/24 无 3、Class 9 无 6）；`absent_methods` 位图 + `dcu_698_class_defines_method`；未定义方法 resolver DAR 4，ACL 先 DAR 3；appconn 零 diff；红编译/断言→绿 45/45、object_model 26/26、catalog 5/5；变异守卫 1 条；合并树 373/373 |
| P3-6 | 已合并 feat/dcu | `dcu/P3-6-zero-seq-oi` | `cbc416b8`（merge 2415aebf） | `docs/evidence/DCU-P3-6-ZERO-SEQ-OI/` | 删除错误别名 `OAD_ZERO_SEQ_CURRENT` 与唯一种子行（标准 A.3/表204 的 OI 2002 是电压相角，全文无零序 OI；2001 A4 是零线电流）；pytest 守卫注册 guard selftest；删前红/删后绿；45/45；生成物零变化；验收 3 连绿 |

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
| 2026-11-22 | D1-17 | 已合并 | `docs/evidence/DCU-D1-17/` | 合并提交 `5494f506`；Class 20 会话与对象列表 ACL；合并后 32/32、ASAN、守卫、验收通过 |
| 2026-11-22 | D1-01 | 已合并 | `docs/evidence/DCU-D1-01/` | 合并提交 `ceb6a576`；75 条按标准、4 条保留产品类并记录；生成器与守卫自检覆盖无决策/再 OCR 两类回退 |
| 2026-11-22 | D1-18 | 审计完成 | `docs/design/DCU_698_OAD_STATUS.md` | 56 位中 0 real；变量类类型/缩放/数组形状、逐表寻址、事件/记录/采集类假语义是 P0 主因 |
| 2026-11-22 | D6-08a | 已合并 | `docs/evidence/DCU-D6-08a/` | 合并提交 `6509c68f`；SQLite 锁序定向抑制到 3 个用例；A/B 证明第一方锁序仍报；ASAN/UBSAN/fanalyzer 已清，全量 TSAN 在跑 |
| 2026-11-22 | D6-08 | 完成 | `docs/evidence/DCU-D6-08/` | 全量 ASAN/UBSAN/TSAN 各 334/334，UBSAN 0 runtime error，TSAN 0 ThreadSanitizer 行；fanalyzer 0 new |
| 2026-11-22 | D1-18a | 已合并 | `docs/evidence/DCU-D1-18a/` | 合并提交 `8acf47fe`；标准数组与 Scaler_Unit；元素索引选表槽位；修 tag 常量与南向相位键；验收改为解析 APDU 数组 |
| 2026-11-22 | D1-18b | 已合并 | `docs/evidence/DCU-D1-18b/` | 合并提交 `e5906b65`；Class 24 位表、DAR 5/4 规则、A11/A12 真实、旧行清理；合并后 32/32、守卫、验收通过 |
| 2026-11-26 | F5 | 已合并 feat/dcu | `docs/evidence/DCU-F5/` | 红先绿后独立复跑；coverage 验收 3 连绿（2/2，35 passed，22.80/22.97/22.94s）；ASAN 27/27；merge 后守卫自检全绿、warning 0；详见 §12.2 F5 |
| 2026-11-26 | D5-05 | 已合并 feat/dcu | `docs/evidence/DCU-D5-05/` | 红先绿后独立复跑（5 个测试文件红→绿）；coverage 验收 3 连绿（2/2，35 passed，27.15/22.79/22.53s）；ASAN 目标集绿；merge 后守卫全绿、全量 ctest 367/367、warning 0；详见 §12.2 D5-05 |
| 2026-11-26 | F4 | 已合并 feat/dcu | `docs/evidence/DCU-F4/` | 红先绿后独立复跑（两轮各 2 红→绿）；coverage 验收 run2/3/4 连续 3 轮 2/2、35 passed（run1 台架未完成已归因）；ASAN 34/34；合并后守卫全绿、全量 ctest 367/367、fanalyzer 3 known/0 new、warning 0；详见 §12.2 F4 |
| 2026-11-26 | D5-06 | 已合并 feat/dcu | `docs/evidence/DCU-D5-06/` | 红先绿后独立复跑（5 个测试文件红→绿）；coverage 验收 3 连绿（2/2，35 passed，22.76/22.48/22.69s）；ASAN 目标集绿；merge 后守卫全绿、全量 ctest 367/367、warning 0；详见 §12.2 D5-06 |
| 2026-11-26 | D5-03 | 已合并 feat/dcu | `docs/evidence/DCU-D5-03/` | 红先绿后独立复跑（5 个测试文件红→绿）；coverage 验收 3 连绿（2/2，35 passed，22.60/22.67/22.62s）；ASAN 目标集绿；merge 后守卫全绿、全量 ctest 367/367、warning 0；详见 §12.2 D5-03 |
| 2026-11-26 | F4R | 已合并 feat/dcu | `docs/evidence/DCU-F4R/` | 红 1/35→绿 35/35、45/45；coverage 验收 3 连绿（2/2，35 passed，22.85/22.80/22.79s）；详见 §12.2 F4R |
| 2026-11-26 | DCU-MUTATION-F4F5 | 已合并 feat/dcu | `docs/evidence/DCU-MUTATION-F4F5/` | runner selftest 绿；四条变异各先红后绿；产品源码零改动；详见 §12.2 DCU-MUTATION-F4F5 |
| 2026-11-26 | F5R | 已合并 feat/dcu | `docs/evidence/DCU-F5R/` | 红先绿后独立复跑（test_dcu_node 3/4→4/4，test_dcu_advanced_modules 1/2→2/2）；coverage 验收 3 连绿（2/2，35 passed，22.75/31.94/22.57s）；ASAN 泄漏修复 `9635814b`；合并树守卫/全量 ctest 367/367/fanalyzer 3 known/0 new/warning 0；详见 §12.2 F5R |
| 2026-11-26 | D5-04 | 已合并 feat/dcu | `docs/evidence/DCU-D5-04/` | 红先绿后独立复跑（5 个测试文件红→绿）；coverage 验收 3 连绿（2/2，35 passed，23.19/22.28/17.60s）；ASAN 目标集绿；merge 后守卫全绿、全量 ctest 367/367、warning 0；详见 §12.2 D5-04 |
| 2026-11-26 | P4-RELAY | 已合并 feat/dcu | `docs/evidence/DCU-P4-RELAY/` | 红：编译失败→绿 45/45；变异 A/B 各 2/5 例红证明行为敏感；验收 3 连绿（2/2，35 passed，18.23/22.74/23.09s）；合并树守卫全绿、全量 ctest 367/367、ASAN 47/47+5/5、fanalyzer 3 known/0 new、warning 0；详见 §12.2 P4-RELAY |
| 2026-11-26 | F4-ADDTASK | 已合并 feat/dcu | `docs/evidence/DCU-F4-ADDTASK/` | 红 1/36→绿 36/36、45/45；验收 3 连绿（2/2，3+35 passed）；合并树 build/warnings/守卫全绿；全量 ctest 一条非 DCU 时序用例在负载下红、单跑绿；详见 §12.2 F4-ADDTASK |
| 2026-11-26 | F5S | 已合并 feat/dcu | `docs/evidence/DCU-F5S/` | 红 `1000 != 1500`→绿 2/2、45/45；验收 3 连绿（2/2，3+35 passed）；对抗评审 NO FINDINGS（gdb 观察发生+恢复同一 slot）；ASAN 2/2+47/47；详见 §12.2 F5S |
| 2026-11-26 | F5-HEARTBEAT | 已合并 feat/dcu | `docs/evidence/DCU-F5-HEARTBEAT/` | 红 `0 != 3`→绿 4/4、45/45；验收 5 轮 4 绿（run2 零点对齐 208ms 与并发 ctest 同窗，已归因）；变异守卫 `DCU-F5-heartbeat-slot` 先红后绿；详见 §12.2 F5-HEARTBEAT |
| 2026-11-26 | D4-10 | 已合并 feat/dcu | `docs/evidence/DCU-D4-10/` | 两轮红绿（首版链接+计数红；修复轮 seam 链接红+行为红 4/53）；验收 3 连绿（2/2，3+35 passed）；合并树 367/367、守卫+变异守卫绿、ASAN 目标集绿、fanalyzer 热修后 3 known/0 new；详见 §12.2 D4-10 |
| 2026-11-26 | D1-10A | 已合并 feat/dcu | `docs/evidence/DCU-D1-10A/` | 测试/夹具红绿（测试提交构建红→5/5）；变异守卫 `DCU-D1-10A-wrong-oi-method` 独立复跑先红后绿；合并树守卫与 ctest 见证据；无产品代码改动；详见 §12.2 D1-10A |
| 2026-11-26 | D1-10B | 已合并 feat/dcu | `docs/evidence/DCU-D1-10B/` | 偏移 140 行下旧码 512 OI（漏 F300/F301）→新码 514 OI；Python 套件 6/6、变异守卫 `DCU-D1-10B-fixed-line-window` 先红后绿；合并树守卫与 ctest 见证据；无产品代码改动；详见 §12.2 D1-10B |
| 2026-11-26 | D4-04 | 已合并 feat/dcu | `docs/evidence/DCU-D4-04/` | 三红三绿（骨架 5/45；越界 25/26；帧尾 2/28）→ 28/28、45/45；变异守卫 1/1；验收两轮各 3 连绿（2/2，3+35 passed）；合并树 367/367；gdb 探针证明 4096 池拒绝且不越界、生产池封口正确；详见 §12.2 D4-04 |
| 2026-11-26 | D4-07 | 已合并 feat/dcu | `docs/evidence/DCU-D4-07/` | 红 6/45→绿 45/45；变异守卫 1/1；验收 3 连绿（2/2，3+35 passed）；对抗评审 DAR 两层口径裁决并写清；F101=Class 8 口径纠正；详见 §12.2 D4-07 |
| 2026-11-26 | CLEANUP-CODEC | 已合并 feat/dcu | `docs/evidence/DCU-CLEANUP-CODEC/` | 9 处标签字面量换常量，对象码 sha256 前后一致，targeted ctest 7/7；合并树门禁见证据；详见 §12.2 CLEANUP-CODEC |
| 2026-11-26 | D4-09 | 已合并 feat/dcu | `docs/evidence/DCU-D4-09/` | 红 5/45→绿 45/45；变异守卫 1/1；验收 3 连绿（2/2，3+35 passed）；对抗评审 NO FINDINGS；对象 128/ACL 115/列表 5551 B；详见 §12.2 D4-09 |
| 2026-11-26 | D4-06 | 已合并 feat/dcu | `docs/evidence/DCU-D4-06/` | 红 5/45→绿 45/45；变异守卫 1/1；验收 3 连绿（2/2，3+35 passed）；对抗评审 NO FINDINGS；对象 131/ACL 118/列表 5806 B；详见 §12.2 D4-06 |
| 2026-11-26 | 夜间批次收尾（12 单元及同批） | 已合并 main（经 `656731ab`） | `.audit/dcu-plan-night.tsv` 的 `batch-accepted` 行 | edgec-verifier 验收：Release 373/373、验收 11/11（19/19 任务，COVERAGE=ON）、守卫 287/287、mutation 15/15、formal 1073 属性/84083 状态；最终 tip ASAN 4 目标绿、无报告；UBSAN/trimmed/arm64 未跑（operator 提前停止） |
| 2026-11-27 | 谱系切换 | 已同步 | `origin/main` `8d9bc384`；进口提交 `a253c4db` | 本地 main 由 reflog 记录 reset 到 origin/main；DCU 路径与新 main 内容一致，全树仅 19 个非 DCU 文件不同；旧本地历史保留在 `backup/local-main-656731ab`；feat/dcu 已重建到 `8d9bc384`（旧 tip 归档在 `archive/feat-dcu-656731ab`）并已 ff 到 main；后续任务从最新 feat/dcu 开 worktree，不复用旧构建缓存；新 main 本地复验（edgec-verifier 报告）：构建警告 0、ctest 373/373、formal 1073/84083、守卫 287/287、验收 11/11、fanalyzer 5 known/0 new |
| 2026-11-27 | P3-4 | 已合并 feat/dcu | `docs/evidence/DCU-P3-4-EVENT-START/` | 红 `2001000000000 != 1001000000000`→绿 45/45；验收 3 连绿（2/2，35 passed，22.93/22.92/22.75s）；变异守卫 2 条红/绿；ASAN 5/5；对抗评审两轮无功能性 finding，补 archive/eviction 守卫；合并树 ctest 373/373、fanalyzer 5 known/0 new |
| 2026-11-27 | P3-7 | 已合并 feat/dcu | `docs/evidence/DCU-P3-7-DATE-TIME-TAG/` | codec 对象 sha256 `eaa6b7c4…` 前后一致；`cmd/dcu` 标签路径无裸 0x19/0x1C；45/45；验收 3 连绿（2/2，35 passed，22.66/22.69/22.58s） |
| 2026-11-27 | P3-9 | 已合并 feat/dcu | `docs/evidence/DCU-P3-9-HEARTBEAT-DEMO-SLOT/` | 首版 acceptance 3/3 红（trip 帧缺失）已打回；重做后红 `0 != 1`→绿 45/45；验收 3 连绿（2/2，35 passed，22.74/17.85/17.97s）；变异守卫红/绿 |
| 2026-11-27 | P3-8 | 已合并 feat/dcu | `docs/evidence/DCU-P3-8-CATALOG-GAPS/` | 红（旧码）三条断言退 1；绿 5/5、source_window 6/6、--check 0、catalog matches；生成物 sha256 前后一致；变异守卫红 rc=1/绿 rc=0；guard selftest 注册通过；工具单不改产品代码 |
| 2026-11-27 | P3-1 | 已合并 feat/dcu | `docs/evidence/DCU-P3-1-ACL-CAP/` | 红 5 个用例（四类 DAR `3 != 5`、对象列表 `5828 != 6014`）→绿 45/45；验收 3 连绿（2/2，35 passed，22.42/27.83/21.63s）；变异守卫 4 条（含池界）红/绿；对抗评审 1 finding（假绿测试）已修；OBJECT_MODEL/DEVIATIONS 同步 |
| 2026-11-27 | P3-2 | 已合并 feat/dcu | `docs/evidence/DCU-P3-2-WRITE-ASYNC/` | 红 4 条失败→绿 45/45；验收 3 连绿（2/2，35 passed，17.49/27.58/18.19s）；ASAN 17/17；变异守卫 3 条红/绿；对抗评审 2 finding（HTTP 回调 UAF、ADDED_ALREADY 正 rc）已修并对原 repro 复跑 |
| 2026-11-27 | P3-5 | 已合并 feat/dcu | `docs/evidence/DCU-P3-5-VAR-4510/` | 红 `-22 != -21`→绿 45/45；验收 3 连绿（2/2，35 passed，21.56/16.87/22.22s）；变异守卫红/绿；主站 GET 不回归 |
| 2026-11-27 | P3-9 | 已合并 feat/dcu | `docs/evidence/DCU-P3-9-RUNTIME-METHODS/` | 红（编译/断言）→绿 45/45；验收 3 连绿（2/2，35 passed，22.51/22.02/17.00s）；变异守卫红/绿；集合与 P3-8 CLASS_METHODS 对齐 |
| 2026-11-27 | P3-6 | 已合并 feat/dcu | `docs/evidence/DCU-P3-6-ZERO-SEQ-OI/` | pytest 删前红/删后绿；45/45；guard selftest 全绿；生成物 sha256 一致；验收 3 连绿（2/2，35 passed，22.43/17.15/17.27s） |
| 2026-11-27 | P3-10 | 已合并 feat/dcu | `docs/evidence/DCU-P3-10/` | 合并 `b25ff9af`；A6 事件记录序号严格单调递增不回绕；变异守卫 `DCU-P3-10-event-record-index` 红(rc=8)/绿(rc=0)；合并树 ctest 380/380、warning 0、ASAN 绿、守卫自检全绿 |
| 2026-11-27 | P3-11 | 已合并 feat/dcu | `docs/evidence/DCU-P3-11/` | 合并 `53a7b96b`；`bus_orchestrator_destroy` 完整释放 engine/session/timer/pool 并修 `_slave_detach` UAF；变异守卫 `DCU-P3-11-orch-destroy-teardown` 红(rc=8)/绿(rc=0)；ASAN/LSAN 0 泄漏、`uv_loop_close == 0` |
| 2026-11-27 | P3-12 | 已合并 feat/dcu | `docs/evidence/DCU-P3-12/` | 合并 `56c3ca51`；4000 A2 SET 失败 DAR 契约（0/2/7/3）在实现/单测/验收/文档四方对齐；变异守卫 `DCU-P3-12-4000-set-dar` 红(rc=8)/绿(rc=0)；ASAN 绿、守卫自检全绿 |
| 2026-11-27 | P3-13 | 已合并 feat/dcu | `docs/evidence/DCU-P3-13/` | 合并 `22fdfffc`；3006 注释订正、4 脚本 `0x20020200` 改为电压相角、`OAD_REACTIVE_ENERGY` 偏差登记、`test_dcu_698_rules.c` `strcpy→snprintf`；pytest 2/2、ctest 380/380、warning 0 |
| 2026-11-27 | D4-02b | 已合并 feat/dcu | `docs/evidence/DCU-D4-02b/` | 合并 `63bfc8a5`；A.9 8 个控制/参数 OI 只读进入 Class 20 ACL（126 对象 / 6150 B）；GET 诚实回 DAR 2、SET 回 DAR 3；变异守卫 `DCU-D4-02b-class13-acl` 红(rc=8)/绿(rc=0) |
| 2026-11-27 | FORMAL-01 | 已合并 feat/dcu | `docs/evidence/DCU-FORMAL-01/` | 合并 `461a0ee0`；建立 `formal/manifest/dcu.tsv` + `formal/cbmc/dlt698_decode.c`（2740 属性 0 失败、`cover` 满足、`fcs_reject` 通过）+ 负向变异脚本 `dlt698_guard_mutation.sh` 捕获越界 |
| 2026-11-27 | D3-08b | 已合并 feat/dcu | `docs/evidence/DCU-D3-08b/` | 合并 `4fe8beba`；注册 4 个真 Class 7 (`IC_EVENT`) 对象（`3009`/`300C`/`3011`/`3013`）空掩码骨架与只读 Class 20 ACL（130 对象触发两字节长格式 `0x81 0x82`、6435 B）；GET 诚实回 DAR 5、SET 回 DAR 3；变异守卫红/绿 |
| 2026-11-27 | D5-07 | 已合并 feat/dcu | `docs/evidence/DCU-D5-07/` | 合并 `9887fceb`；同步 `cmd/dcu/CONTEXT.md`、`CONTEXT-MAP.md`、`DOMAIN_CONTEXT_MAP.md`、`PUBLIC_STRUCT_AUDIT.md` 至当前代码事实；`check_doc_references.py` 与 `run_guard_selftest.py` 全绿 |
| 2026-11-27 | FORMAL-02 | 已合并 feat/dcu | `docs/evidence/DCU-FORMAL-02/` | 合并 `797f03a6`；扩展 `formal/manifest/dcu.tsv`，为 `dlt698_decode_get_record_request` 增 CBMC 边界证明（2734 属性 0 失败、`cover` 满足、`selector_reject` 通过）+ 负向变异脚本捕获 `_read_be32` 越界 |
| 2026-11-28 | D4-11 | 已合并 feat/dcu | `docs/evidence/DCU-D4-11/` | 新增 `tests/test_dcu_698_p4_matrix.c`，覆盖 P4 9 个接口类（13/18/19/20/21/22/23/25/26）各至少 4 项断言；合并树 ctest 381/381、warning 0 |
| 2026-11-28 | D6-01 | 已合并 feat/dcu | `docs/evidence/DCU-D6-01/` | 扩展 `tests/test_dcu_698_catalog.c` 为 26 类目录与 ACL 一致性矩阵（135 对象 + 130 ACL 对象零悬空 + 类外 DAR 4/未注册 DAR 6）；合并树 ctest 381/381；登记 2 条基线观察 |
| 2026-11-28 | FORMAL-03 | 已合并 feat/dcu | `docs/evidence/DCU-FORMAL-03/` | 记录 `dlt698_decode_data` 递归 `O(N^N)` 爆炸发现；转向并证明 `dlt698_encode_report_notification`（2728 属性 0 失败、`cover` 满足、`report_reject` 通过 + 负向变异脚本捕获越界写），并修复 `dlt698_codec.c:508-510` `& 0xFFu` 截断检查 |

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
| 3000/3005 属性号 | `class_id` 已改 24，属性位在 D1-18b 中按 Class 24 重排 | 主站按 Class 24 读会落到未定义属性；位号范围检查发现不了 | D1-18b 收口 |
| codec 魔数标签 | 已收口（2026-11-26 CLEANUP-CODEC；2026-11-27 P3-7）：新增 `DLT698_TAG_DATE_TIME = 25`，`foundation/src/protocol/dlt698_codec.c`、`cmd/dcu/dcu_698_axdr.c`、`cmd/dcu/dcu_698_class_event.c` 的 date_time/date_time_s 标签字面量全部换常量；codec 对象 sha256 前后一致 `eaa6b7c4…` | 可读性，易与新标签漂移 | 已完成；证据 `docs/evidence/DCU-P3-7-DATE-TIME-TAG/`；已合并 feat/dcu（merge `4ce21c74`） |
| A.7 方法上限按类聚合 | D1-10A + P3-8 生成器按真实 7.3 集合校验；2026-11-27 P3-9 运行时也用真实集合（Class 7/24 无 3、Class 9 无 6），未定义方法 resolver DAR 4、ACL 先 DAR 3 | 生成器/检查器与运行时方法集合均已收口 | 证据 `docs/evidence/DCU-P3-8-CATALOG-GAPS/`、`docs/evidence/DCU-P3-9-RUNTIME-METHODS/`；已合并 feat/dcu（merge `21c84480`） |
| 生成器行窗口 | 已修（2026-11-26，D1-10B）：`appendix_a_window` 按 `附 录 A`/`附 录 B` 标题定位附录 A，再按 `表A.1`..`表A.14`（除数据格式表 A.10）标题及“对象名称”表头校验；行号整体平移后解析结果不变，标题缺失或重复时生成器报错退出 | 原缺口已消除：不再静默丢行 | 证据 `docs/evidence/DCU-D1-10B/`；已合并 feat/dcu（merge `8127a473`） |
| test-platform 解析器 | `testplatform.protocol.dlt698.parse_get_response_list` 遇到逐项 DAR 抛错、拒绝 count 0；`get_request_list` 仍拒绝空列表 | 验收只能直读 APDU，平台侧无法表达合法空列表与混合结果 | 外部 pinned 仓库更新；本仓不改 |
| 继电器 OAD 映射与真实 DO 执行 | P4 已提供映射接口：`dcu_relay_map_t`（固定 32 项）由 `dcu_node` 拥有，`dcu_node_set_relay` 填表；空表 129–131 回 DAR 2，命中后因真实 DO 未接线仍回 DAR 5 | 点位表数值（继电器 OAD → 表地址/DO 通道）与跳合闸执行未接线 | 产品点位表拍板后填表；真实 DO 执行另立任务 |
| `OAD_ZERO_SEQ_CURRENT` 的 OI 编号 | 已收口（2026-11-27，`dcu/P3-6-zero-seq-oi`）：标准 A.3/表204 的 OI 2002 是电压相角，全文无零序电流 OI；2001 A4 是零线电流（IN=3I0）；删除宏与唯一种子行 | 不再有按零序电流误读的 OAD | 已完成；证据 `docs/evidence/DCU-P3-6-ZERO-SEQ-OI/`；已合并 feat/dcu（merge `2415aebf`）；相邻 3006 注释/脚本硬编码另立 |
| D2-05 执行路径 | 6016 到 601B 的管理平面可做，但事件上送依赖 D3-02/D3-05b 的事件记录与上报接线；透明转发依赖 raw I/O 的等待/比对与 ADR-0017 REPORT CHOICE[2]；6013 任务当前拒绝 scheme types 2-5 | D2-05 的“事件上送与透明转发各一例”无法在管理平面任务内完成 | 拆成 D2-05a（管理平面、ACL、GET/ACTION、持久化、DAR 诚实，夜间先做）与 D2-05b（执行，等 D3-02/D3-05b 与 raw I/O）；事件上送仍归 D3-02/D3-05b |
| MS 编码上下文 | ACTION Data 参数与 A2 Data 结构（6014/6015/6016 到 601B）按 H.5.4 编成 `5C choice`；RSD Selector5 的 MS 是协议 SEQUENCE 字段，标准示例（`docs/698_parsed.md` 1825-1833、10311-10317）为裸 choice，产品未实现 Selector5 | 两种上下文不可混用；把 ACTION Data 的 `5C` 当缺陷会破坏 H.5.4 标准帧 | 保持 ACTION Data 的 `5C`；日后实现 RSD Selector5 时另写 typed codec，不共用 `_decode_ms` |
| D2-06 执行面 | 601C/601D 管理平面可做；SnF/重试/CHOICE[2] 记录上报与生产来源依赖 ADR-0017 与 D3-05b | 验收“主站断开恢复、按序补齐”无法在管理平面完成 | D2-06a 管理平面已做；D2-06b/D3-05b 承接；D2-06-r 显式 storage 与事务助手；ACL cap 由 D2-07/D2-08 提升 |
| D2-07 本机 TSAN | 内核 `vm.mmap_rnd_bits=32` 且 `/proc/sys` 只读，TSAN 启动报 unexpected memory mapping；备用 DI 重试的线程证据来自 `uv_default_loop()` 与 `cmd/dcu` 无 pthread 的代码核对 | 重试路径缺本机 TSAN 运行时证据 | CI matrix 的 TSAN 补；`docs/evidence/DCU-D2-07/summary.md` 已记录 |
| D2-08 采集状态字段 | 6032/6033 缺相序检测器与中继拓扑，整项回 DAR 2；6034/6035 已实现任务执行状态与按任务收发计数 | 采集失败次数与状态集本单不可读；D2-08 验收改用 6035 成功数与任务状态 | 相序检测与中继拓扑另立任务 |
| D2-08 档案变更计数 | 执行中档案增删按新执行重开，success/sent/recv 清零；重开前在途读的响应仍给新执行记一次 recv，recv 可暂时大于 sent | 档案变更瞬间收发计数不完全配对；状态与成功数不受影响 | 档案变更少见；如需严格配对，给在途读加执行代际并在 note_rx 过滤 |
| D3-02 冻结字段 | 表 A.6 只列 OI；Class 9 属性 3 是 `array structure {冻结周期, 关联 OAD, 存储深度}`（`698_parsed.md` 4190–4215），prd 正确；计划原写的 ACTION 8010 与“曲线冻结”无标准依据；DLT698 驱动会话上限 `DRIVER_PRIV_SIZE` 1024 字节（ACL 对象 16 字节，60 项约 1000 字节，65 项必炸） | D3-02 用标准默认节奏（周期=1）作占位；主站可配周期/深度是 D3-03；手动触发走方法 3（D3-03）；5008/5009/500A/500B/5011 无切换触发源且不进 ACL/Class 20 列表（GET A2 实测 DAR 5）；结算日默认每月 1 日 00:00 结算上月（标准为每次月结算动作，产品结算日参数未实现） | D3-03 实现属性 3 后调度器改为消费配置；切换类如需发布仍需触发源；ACL 会话上限已由 D1-21 提升（224/4096/10240），不再是阻塞；结算日可配另立参数任务 |
| D3-02 秒冻结写放大 | 5001 秒冻结按标准每秒写 40 表 × 3 源 = 120 行；本单只用虚拟时钟跨 1 个零点、未做秒级长跑 | 生产启用 5001 会持续写放大 | P6 性能评估：批量/降采样/存储层合并；D3-02 证据已记录 |
| D3-03b 属性 3 完整语义 | 2026-11-24 由 D3-03b 收口：每 (OI, 条目) 独立周期、方法 3 延时触发、按深度裁剪、超上限 DAR 3 整单回滚、历史截断诊断 | 剩余限制：pending 池容量 8 是产品常量；同一 OAD 多方案（属性特征 0..7）未建模；属性 3 无 RSD（标准只对属性 2 记录定义）；历史超限行只诊断不迁移 | 更大池或按方案建模时另立任务 |
| D4-02a Class 13 只读边界 | 已收口（2026-11-27，D4-02b，`63bfc8a5`）：8001、8100–8103、810E–8110 已只读加入 Class 20 对象列表/ACL（126 对象 / 6150 B）；GET 诚实回 DAR 2、SET 回 DAR 3 | 真实继电器/功控执行来源与 129/130/131 执行仍未接线 | ACL/Class 20 广告已收口（证据 `docs/evidence/DCU-D4-02b/`）；真实控制执行另立任务 |
| D3-08 Class 7 事件对象缺口 | 部分收口（2026-11-27，D3-08b，`4fe8beba`）：4 个真 Class 7 (`IC_EVENT`) 对象（`3009`/`300C`/`3011`/`3013`）已建空掩码骨架并加入只读 Class 20 ACL（GET 诚实回 DAR 5，不再 DAR 6）；`300B` 经表 A.4 核实为 Class 24 (`IC_SUB_EVENT`) 留待后续 | 真实事件记录引擎与停电/停上电上报（D3-05b/D3-07）仍未接线 | 骨架与 ACL 已收口（证据 `docs/evidence/DCU-D3-08b/`）；真实事件引擎与上报待 ADR-0017 另立任务 |
| Class 12 表157/表158 冲突 | 标准内部 16/17 归属相反（`698_parsed.md:4365-4366` vs `:4423-4425`）；D5-04 按类定义表157 登记 2401，ACL 只放属性 1..15，16/17 不广告 | 16/17 的映射仍未由 PDF 所有者裁决 | HOLD；裁决后 16 可进 ACL，17 仍超 schema | PDF 所有者裁决后写死并加对拍 |
| Class 12 2402-2408 与脉冲 producer | 2401 对象行/ACL 已就绪（D5-04）；2402-2408 未注册，无脉冲输入/电能示值来源 | 除 2401 外的脉冲计量对象不可读；2401 值回 DAR 5 | 有脉冲 producer 后补对象行、mask、resolver 编码与验收值 | 另立任务 |
| 4000 A2 SET 失败 DAR | 已收口（2026-11-27，P3-12，`56c3ca51`）：实现、单测、北向验收 `D_00_00_02_000022` 与 `DCU_698_OBJECT_MODEL.md` §8.1 统一为 0/2/7/3 契约 | 文档/代码/合同已一致，不回退假 RAM 时钟 | 已完成；证据 `docs/evidence/DCU-P3-12/`；变异守卫 `DCU-P3-12-4000-set-dar` |
| check_composition_root 路径子串 | 已修（2026-11-25 /correct 轮）：12 个守卫的 tests/fixtures 豁免改为 `c_source_scan.rel_scan_parts`/`skip_subtree` 相对扫描目标判断，fixture 模式用 `guard_baseline.is_selftest_fixture`；新增 `check_guard_path_independence.py` 正反夹具并登记 guard selftest（286/286） | 守卫不再随检出目录名变化（旧码在 `tests` 路径下会静默跳过、在 `guards` 路径下假红） | 已收口；规则见 AGENTS.md 规则表 |
| F4 pending 容量饿死第二任务 | 已收口（2026-11-26，见 §12.2 F4）：容量内分批 + 执行身份；每任务每 tag 每执行恰好一次，6012 换方案只重开被改任务，熔断按整表访问结算 | 原缺口已消除；F4R 已补方案内容代际，`dcu_poller_get_scheme` 可变指针仍是 HOLD | 已完成；证据 `docs/evidence/DCU-F4/`、`docs/evidence/DCU-F4R/` |
| F5 删表后迟到回调写错地址 | 已收口（2026-11-26，见 §12.2 F5）：档案 revision 成为访问与重试边界，读身份按快照落库与发布；迟到回调不再跨 revision 续发，slot 复用不再触发重试 | 原缺口已消除 | 已完成；证据 `docs/evidence/DCU-F5/` |
| Class 14/15/16 统计生产者 | 对象行与只读 ACL 已就绪（D5-05），但无区间统计/累加平均/极值的运行期计算、存储与喂数；GET 回 DAR 5 | 统计值不可读 | 有数据源后补对象 mask、resolver 编码与验收值用例；另立任务 |
| Class 17 显示配置生产者 | F300/F301 对象行与只读 ACL 已就绪（D5-06），但无轮显表、屏序号、显示时间、显示参数来源；GET 回 DAR 5 | 显示配置不可读、不可配 | 有显示配置来源后补对象 mask、resolver 编码与验收值用例；另立任务 |
| Class 11 文件/脚本集合 producer | 7000/7010 对象行与只读 ACL 已就绪（D5-03），但无文件/脚本集合存储与元素来源（D4-06 未做）；7010 A2 与 7000 方法回 DAR 5 | 集合元素不可读、文件/脚本方法不可执行 | 有存储与 producer 后补对象 mask、resolver 编码与验收值用例；另立任务 |
| Class 25/26 通信参数生产者 | 对象行/只读 ACL 与 4510 A5 平台物理单播 MAC 已就绪（D4-10）；无 APN/ICCID/IMSI/SIM 号码/信号强度/端口配置来源，4511–4517 无逐口映射 | 除 4510 A5 外全部 DAR 5；写与持久化不可用 | 有平台配置或存储后补逐口映射、写与持久化；另立任务 |
| 事件快照 device_id 错位 | `dcu_698_var.c::_read_element` 把 OAD 低字节当 device_id 传 `read_shadow`，`_capture_record_snapshots` 用元素序号当设备号；F5R 测试把 snapshot_count 置 0 隔离 | 事件 A2/A6 关联数据快照不跟表 | F5S 已修：快照按 `ev->device_id` 经 `dcu_698_var_encode_axdr_device` 取表；本条仅留作历史 |
| 事件恢复记录 start_time 身份 | 已收口（2026-11-27，`dcu/P3-4-event-start-time`）：`dcu_event_record_t` 增加 `slot_id`，恢复记录按同 slot 的最新 active 记录取 `start_time`；发生记录被 10 条窗口挤出时不追加恢复记录，只更新 A14 `last_end_time`（不伪造起点）；A14 `last_start_time` 仍每 OI | 同一 OI 两只表交错不再串时间；淘汰分支是显式产品语义（丢弃而非伪造） | 已完成；证据 `docs/evidence/DCU-P3-4-EVENT-START/`；已合并 feat/dcu（merge `f02ed19e`） |
| Class 19 设备管理 producer | 4300/4307–4309 对象行与只读 ACL 已就绪（D4-04）；无设备身份/版本/复位/数据初始化/恢复出厂/事件初始化/需量初始化端口 | GET 与方法按类定义回 DAR 5 | 有设备身份与平台端口后补 mask、处理器与验收值；另立任务 |
| Class 19 子设备方法 127..129 | 已收口（2026-11-27，`dcu/P3-1-acl-cap`）：每对象方法上限 8→12、会话级 1024 B 方法池；4307..4309 方法 1..6+127..129 已广告 | ACTION 到 resolver 回 DAR 5（无来源）；不再 DAR 3 | 已完成；证据 `docs/evidence/DCU-P3-1-ACL-CAP/`；已合并 feat/dcu（merge `a33af605`） |
| 从站响应池与 write_async 引用契约 | 已收口（2026-11-27，`dcu/P3-2-write-async`）：`io_executor.h` 写明契约；poll/UDP/HTTP 成功时自持引用到完成/失败/destroy，调用方无论 rc 立即 unref；HTTP 完成回调 destroy 顺序与 CURLM_ADDED_ALREADY 已修 | slave 只用 poll 的路径安全；UDP/HTTP 引用不再漏/双释放；MQTT 仍按库拷贝假设，未验证 | 契约完成；证据 `docs/evidence/DCU-P3-2-WRITE-ASYNC/`；MQTT 真实端点另立 |
| Class 21 ESAM producer 与方法 9..12 | 已收口（2026-11-27，`dcu/P3-1-acl-cap`）：F100 方法 1..12 已广告 | 方法 9..12 ACTION 回 DAR 5（无 ESAM 来源）；A2/A3 序列号/版本与安全会话仍无来源 | 已完成；证据 `docs/evidence/DCU-P3-1-ACL-CAP/`；已合并 feat/dcu（merge `a33af605`） |
| Class 23 总加组 producer 与 A17/A18 | 已收口（2026-11-27，`dcu/P3-1-acl-cap`）：2301..2308 A1..A18 已广告 | A17/A18 GET 回 DAR 5（无来源）；SET 仍 DAR 3；总加值与滑差仍不可用 | 已完成；证据 `docs/evidence/DCU-P3-1-ACL-CAP/`；已合并 feat/dcu（merge `a33af605`） |
| Class 18 文件传输 producer 与 F001 9/10 | 已收口（2026-11-27，`dcu/P3-1-acl-cap`）：F001 方法 1..10 已广告 | F001 9/10 ACTION 回 DAR 5（无来源）；文件存储/DFU 激活/校验/回滚仍不可用 | 已完成；证据 `docs/evidence/DCU-P3-1-ACL-CAP/`；已合并 feat/dcu（merge `a33af605`） |
| heartbeat 演示冻结 device_id | 已收口（2026-11-26，`dcu/F5-heartbeat-slot`）：DCU_DEMO_MODE 下冻结触发与演示跳闸改用 `inv->meters[i].slot_id`，不再用循环下标；删除重排后冻结断面与跳闸按稳定身份归表 | 原 demo-only 错表/丢冻结已消除，生产不走 | 已完成；证据 `docs/evidence/DCU-F5-HEARTBEAT/` |
| heartbeat 演示控制 tick 目标 | 已收口（2026-11-27，`dcu/P3-9-heartbeat-demo-slot`）：`dcu_heartbeat_control_tick` 从档案第一只表取稳定 slot_id，空档案跳过；不再写死 slot 0 或地址 10000001 | demo-only；生产遥控路径未改 | 已完成；证据 `docs/evidence/DCU-P3-9-HEARTBEAT-DEMO-SLOT/`；已合并 feat/dcu（merge `04baf768`） |
| A6 记录序号回绕重复（pre-existing） | 已收口（2026-11-27，P3-10，`b25ff9af`）：`new_rec->index` 按已有窗口首项 `records[1].index + 1` 单调递增，12 次追加产出 `[12..3]` 无重复 | 原回绕重复已消除；跳过恢复不推进序号 | 已完成；证据 `docs/evidence/DCU-P3-10/`；变异守卫 `DCU-P3-10-event-record-index` |
| 从站响应池 32 项容量 | D4-04 独立池 32×10496；slave 只用 poll；IEMS_MAX_VBUS=32 多从站可能耗尽并静默丢响应 | DCU 单从站不受影响 | 多从站产品按总线数扩容或加背压；另立任务 |
| bus_orchestrator destroy 不释放 engine/session | 已收口（2026-11-27，P3-11，`53a7b96b`）：`bus_orchestrator_destroy` 逐路 detach engine、停关 `audit_timer`、unref `pool`/`slave_pool`，并修 `_slave_detach` 对调用方 `io` 的 UAF | ASAN/LSAN 0 泄漏、`uv_loop_close == 0` | 已完成；证据 `docs/evidence/DCU-P3-11/`；变异守卫 `DCU-P3-11-orch-destroy-teardown` |
| dcu_698_value_for_oad 的 4510 A5 标量路径 | 标量端口 `dcu_value_t` 无 octet-string，6 字节 MAC 不可承载；主站 GET 走 `resolve_axdr` 正常 | 标量路径旧回 DAR 7（类型不符） | 已收口（2026-11-27，P3-5）：按 DEVIATIONS 无来源口径回 DAR 5；证据 `docs/evidence/DCU-P3-5-VAR-4510/`；已合并 feat/dcu（merge `9c45f835`） |
| 3006 注释与 OAD 脚本硬编码（follow-up） | 已收口（2026-11-27，P3-13，`22fdfffc`）：`dcu_soe_event_engine.h:24` 注释订正为电能表断流事件；4 脚本 `0x20020200` 订正为电压相角；`OAD_REACTIVE_ENERGY` 偏差记入 `DCU_698_DEVIATIONS.md`；`test_dcu_698_rules.c` `strcpy→snprintf` | 文档/脚本误导与 `strcpy` 已消除，pytest 守卫防回归 | 已完成；证据 `docs/evidence/DCU-P3-13/` |
| `k_oi_max_attr` 缺 `0x4000`/`0x8001` 与变量类越界 DAR 偏差 | D6-01 发现：`dcu_698_object_model.c` `k_oi_max_attr` 漏列 `0x4000`/`0x8001`；`dcu_698_var.c` `dcu_698_value_for_oad` 对变量类类外属性返回 `-20`（DAR 6）而非 `-23`（DAR 4） | 不影响正常路径；类外越界读变量类回 DAR 6 而非 DAR 4 | 另立缺陷任务修复并补正反测试 |
| `dlt698_decode_data` 递归池 CBMC 不可解 | FORMAL-03 发现：`dlt698_decode_data` 对 `ARRAY/STRUCTURE` 递归调用并写 64 项 `_Thread_local` 池，CBMC 5.12 即使 `max_len=1` 也出现 `O(N^N)` SSA 爆炸 | 无法直接对 `dlt698_decode_data` 做有界符号证明 | 若需形式化验证，另立重构单将其改为显式栈有界迭代 |

| foundation/src/protocol 覆盖率插桩 | 现有 `-DCOVERAGE=ON` 只给 `edge_common_interface` 加 `--coverage`；`edge_codec` 等 foundation 静态库不链接该目标，lcov 采集为 0 行，`check_coverage.py` 视为 MISSING | 该目录无法设下限，覆盖率门禁对其无约束 | `DCU-D6-09b`：给 foundation 静态库加插桩并重定 ALL/foundation 基线；不得写 0 行下限或下调既有下限 |

### 12.6 COM 迁移登记（2026-11-22 解阻塞后）

用户拍板：P2 到 P5 先在 `cmd/dcu` 内实现，不等 COM 组件。以下迁移在对应组件合并后执行，迁移完成前 DCU 私有实现是唯一实现，不得复制到其他产品。

| 现在（DCU 内） | 迁移到 | 迁移判据 |
| --- | --- | --- |
| Class 10 采集方案、任务、档案查询复用 `dcu_poller` 与 `dcu_storage` | `edge_acq`（COM-C1-01） | 组件接口覆盖现有端口；DCU 全部单测与验收绿；`dcu_poller` 行数下降 |
| 事件记录、上报标识、时间质量复用 `dcu_event_monitor` 与 `dcu_storage` | `edge_event`（COM-C2-02）、`edge_time`（COM-C2-01） | SnF 与时间质量用例迁移后全绿 |
| Class 13 控制经 `dcu_node` 聚合根与现有控制端口 | `edge_control`（COM-C3-01） | 选择、执行、闭锁、记录四类用例迁移后全绿 |
| 域内端口头仍在 `foundation/include/`，适配器仍在 `adapters/` | 不变 | 依赖方向检查通过 |

### 12.7 2026-11-23 后置缺陷修复（PR #578 复查）

operator 授权把 #578 合并后发现并复现的 4 条缺陷并入 `feat/dcu`，再由 edgec-verifier 合入 main。四条都按 defect 处理，失败复现先于修复，证据在 `docs/evidence/DCU-FIX-578-<n>/`。

| 任务 | 状态 | 分支 | HEAD | 证据 | 备注 |
| --- | --- | --- | --- | --- | --- |
| DCU-FIX-578-1 | 已合并 main | `dcu/fix-578-1-getrecord-overflow` | `0163d3a4` | `docs/evidence/DCU-FIX-578-1/` | GetRecord 整页与 trailer 边界回 DAR 2，不再越界；ROAD 分支与注释审查的开放项见证据 |
| DCU-FIX-578-2 | 已合并 main | `dcu/fix-578-2-archive-rollback` | `f1f72145` | `docs/evidence/DCU-FIX-578-2/` | 6000 方法 129/130 落库失败恢复 RAM 快照，GET 6001 A2 返回旧值 |
| DCU-FIX-578-3 | 已合并 main | `dcu/fix-578-3-scheme-dar` | `fbee4b3e` | `docs/evidence/DCU-FIX-578-3/` | 6014 方法 128 被引用方案回 DAR 3，不存在仍 DAR 6 |
| DCU-FIX-578-4 | 已合并 main | `dcu/fix-578-4-event-acl-write` | `d2e9c8c0` | `docs/evidence/DCU-FIX-578-4/` | 3000 A6 广告只读，合法 SET 回 DAR 3；A8/A9 保留 DAR 5 可达 |

main 合并：edgec-verifier 本地合并 `51afa566`，未 push；main 随后到 `53f1764c`，fix wave 仍在历史里。合并结果上 edgec-verifier 另跑 full ctest 342/342、`app_platform_e2e` 1/1、guards 105；TSAN、覆盖率、性能未在合并结果重跑。

feat/dcu 验收：clean Release build 0 warning；`check_build_warnings.py` 0 new；全量 `ctest -E '^app_platform_e2e$'` 342/342；`ctest -R 'dcu|dlt698'` 39/39；`run_guard_selftest.py` 105 PASS；`run_fanalyzer.py` 3 known / 0 new；目标 ASAN 70/70；目标 UBSAN 70/70；`./acceptance/run.sh dcu` 2/2 任务（3 acceptance + 23 northbound）通过。证据包含 `runtime.log`。一次 `test_dcu_runtime_health_concurrent` 超时，单独重跑两次与全量重跑均通过，记为 flake。`./test.sh` 的 venv bootstrap 因访问 `files.pythonhosted.org` 超时被阻断，改用同一 build 的直接 ctest，不影响代码门禁。TSAN、覆盖率、性能门禁与 `app_platform_e2e` 属于 CI 矩阵，需在 main 合并结果上补齐。

下一批仍按 §12.1，不因本修复波改变顺序。注释审查的开放重构项见各证据 `comments.md`，作为后续 ticket。

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
