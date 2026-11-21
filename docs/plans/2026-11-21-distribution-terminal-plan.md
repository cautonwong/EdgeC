# 配网终端开发方案与计划（FTU 与 DTU）

> 日期：2026-11-21　状态：草案，待评审后派活
> 读者：产品、架构、C 开发、验证、前端、硬件
> 用法：§7 的任务表就是派活单位，一任务一 issue 一 PR；§8 定跟踪口径。

## How to read this

一个任务行就是一个工作单元。每行都写了负责角色、估时、依赖、交付物和出口证据。一个任务就是一个工作单元，每个工作单元都写明检查它的证据。出口证据必须是能复现的东西：测试输出、报文抓包、验收报告、PR 链接或提交 SHA。没有证据就不算完成。

测试通过只是必要条件。本程序的完成定义是仓库口径：静态分析、测试、运行时验证证据三者齐全（`AGENTS.md` 完成定义）。落到每个 PR 上，就是单元、实机与性能三类证据都齐了才算验过。Tests alone are not sufficient verification. A PR is verified only when its unit, live, and perf boxes are all checked.

实现开始后，每个任务开一个 issue，实现时按 `multi-phase-plan` 的 PR 结构补上逐 PR 的单元、实机与性能验证块。当前文档没有 PR 段，因为代码还不存在。

## Program checklist

- [ ] 评审本方案，§10 的开放问题 1、2、3、6 给出结论。
- [ ] 按 §7 给 M0 到 M5 建 GitHub Milestone。
- [ ] 按 §8.1 给每个任务建 issue，挂里程碑与标签，确认依赖顺序。
- [ ] 每周一按 §7 认领任务，周五按 §8.3 交证据并更新进度表。
- [ ] 每个 PR 合入后，在目标分支重跑 `run_guard_selftest.py` 与 `test.sh`。
- [ ] 阶段判据未过，不启动下一阶段的新任务（可并行的除外，见 §8.5）。

## 1. 结论

配网终端是 EdgeC 组件库上的第四个产品装配，覆盖两类终端：FTU（馈线终端，柱上开关）与 DTU（站所终端，环网柜与开闭所）。两者共用一个产品壳 `cmd/dist_terminal`，角色由配置决定；行为差异收敛在领域模型与点表模板，不复制两套代码。

约 60% 的能力已在仓库里：IEC104 编解码与主从双角色驱动、从站平面、驱动框架、DataHub、SnF、SQLite、App 平台。缺的是四个可复用组件和一个薄产品壳：

- `adapters/iec104_transport`：IEC104 从站，从 `cmd/iems` 下沉。
- `modules/edge_event`：有界事件缓冲（SOE）与持久化端口。
- `modules/edge_fault`：故障检测与故障记录。
- `modules/edge_fa`：馈线自动化状态机。
- `cmd/dist_terminal`：只做装配。

首版不做 IEC 61850、不做就地型 FA、不做 DL/T 698 安全层。开放项集中在 §10。

## 2. 终端形态：FTU 与 DTU

### 2.1 对比

行业常规对比如下，具体点数与功能以项目招标技术规范为准。

| 维度 | FTU（馈线终端） | DTU（站所终端） |
| --- | --- | --- |
| 安装位置 | 柱上开关、馈线分段点（户外） | 环网柜、开闭所、配电室（柜内） |
| 控制对象 | 1 台柱上开关，常带重合闸 | 多个间隔的开关柜 |
| 遥信 | 开关位置、储能、远方/就地、气压或 SF6 | 各间隔位置、保护动作信号、柜门、温湿度 |
| 遥测 | 三相电流、电压、零序 | 母线电压、各间隔电流、功率、零序 |
| 遥控 | 分闸、合闸、复归 | 各间隔分合、复归 |
| 遥调 | 少见（定值远方修改，视装置） | 有（保护定值、档位） |
| 典型点数 | 20 到 60 | 100 到 300 |
| FA 角色 | 就地判据参与，集中型执行端，重合闸配合 | 集中型执行端，多间隔隔离与恢复 |
| 通信 | 无线公网或光纤 | 光纤为主 |
| 环境 | 户外，宽温，EMC 严 | 柜内，温控较好 |

### 2.2 同与不同

共用：点模型、四遥通道、SOE、104 从站、定值框架、事件存储、维护接口。

不同：间隔与开关的聚合形态、点表模板、FA 参与方式、重合闸时序。计划用同一套领域模型承载差异：`Terminal` 有 `role`，DTU 下挂多个 `Bay`，FTU 只有一个 `Switch`。差异不写进两套代码分支，写进配置与点表模板。

### 2.3 TTU

TTU（配变终端）不属于首版。台区监测与线损已在 `cmd/dcu` 的上下文里（`CONTEXT-MAP.md` §2.2）。配网终端后续可复用 `edge_event`、`edge_fault` 与点表框架，但聚合模型不同，单独立项。

## 3. 领域模型（DDD）

### 3.1 限界上下文

现有上下文按 `docs/design/DOMAIN_CONTEXT_MAP.md` 与 `CONTEXT-MAP.md`。本产品新增一个业务上下文，其余复用。

| 上下文 | 类型 | 代码落点 | 本产品的关系 |
| --- | --- | --- | --- |
| 配电自动化终端（新） | core | `cmd/dist_terminal`、`modules/edge_fault`、`modules/edge_fa` | 本产品主场 |
| 四遥测控（DCU 已有） | core | `cmd/dcu` | 共享术语与不变量：遥控双确认、遥信防抖 |
| 遥测与总线 | supporting（共享内核） | `modules/edge_core`、`modules/edge_io` | 经端口访问，不 include 内部头 |
| 北向与 SnF | supporting | `modules/edge_north`、`adapters/iec104_transport` | Customer-Supplier，本产品是下游 |
| 访问控制 | supporting | `modules/edge_mgmt`、`modules/edge_security_policy` | 遥控授权与审计 |
| 应用平台 | supporting | `modules/edge_app_host` | 可选分析应用 |

集成方式：南向驱动是 Upstream，经 ACL 把驱动报文翻成领域点值；北向是本产品对主站的 Customer-Supplier，点号到 IOA 的翻译只发生在适配器内，协议类型（COT、ASDU、TypeID）不进入领域层。

### 3.2 聚合与不变量

| 聚合根 | 实体与值对象 | 不变量 |
| --- | --- | --- |
| `Terminal` | role（FTU/DTU）、运行状态、时钟状态、配置代际 | role 运行期不可变；配置代际单调递增；非运行态禁止遥控 |
| `Bay`（仅 DTU） | id、名称、开关集合、点集合 | 间隔 id 唯一；开关数量固定；间隔删除前必须先停控制 |
| `Switch` | 位置（分/合/未知）、远方/就地、储能、闭锁 | 遥控必须选择后执行（SBO）；同一开关同一时刻只有一笔进行中操作；反馈超时置未知 |
| `Point` | id、kind（YC/YX/YK/YT）、scale、deadband、quality、soe | kind 不可变；quality 取值与转移受约束；soe 点必须带时标 |
| `EventLog` | seq、ts、point、old、new、quality、overflow 计数 | seq 单调；每条带时标；容量满时记溢出，不静默丢 |
| `FaultRecord` | 触发点、采样窗口、相别、峰值、定值命中、结论 | 关闭后不可变；记录必须能追溯到采样与定值 |
| `SettingGroup` | 组号、生效标记、点值集合、审计 | 同时只有一组生效；切换必须留审计 |
| `FAProcess` | 当前状态、故障实例、跳闸与重合次数 | 每条馈线同一时刻只有一个进行中流程；状态只前进，人工复归除外 |

### 3.3 领域事件

`SwitchPositionChanged`、`CommandSelected`、`CommandConfirmed`、`CommandRejected`、`FaultDetected`、`FaultCleared`、`RecloseAttempted`、`SettingGroupActivated`、`ClockSynced`、`EventOverflow`。

事件在两个地方消费：领域内驱动 `FAProcess` 前进；北向上送为 104 遥信变化或 SOE。事件与 104 的映射表在适配器里维护。

### 3.4 通用语言

| 术语 | 代码名 | 含义 |
| --- | --- | --- |
| 遥测 | YC / `POINT_YC` | 模拟量测量 |
| 遥信 | YX / `POINT_YX` | 开关量状态 |
| 遥控 | YK / `POINT_YK` | 分合闸命令 |
| 遥调 | YT / `POINT_YT` | 定值或档位调整 |
| 总召唤 | GI | 主站索取全量数据，`C_IC_NA_1` |
| 传送原因 | COT | 104 报文的传送原因字节 |
| 信息对象地址 | IOA | 104 点地址 |
| 选择后执行 | SBO | 遥控两步确认 |
| 双点遥信 | DP | 用两位表示开关位置 |
| 事件顺序记录 | SOE | 带时标的状态变位记录 |
| 馈线自动化 | FA | 故障隔离与非故障区恢复 |
| 重合闸 | reclose | 跳闸后按延时自动重合 |
| 定值区 | setting group | 一组可切换的保护与运行定值 |
| 间隔 | bay | 站所内一个开关间隔 |

## 4. 组件地图

新增组件按仓库规则登记 `component.meta`：subdomain、ports、ROM/RAM/栈预算、owner、maturity。预算先给初值，实现后按归档尺寸回填，只降不涨。

| 组件 | 类型 | subdomain | 端口 | 初始预算 | 复用方 |
| --- | --- | --- | --- | --- | --- |
| `adapters/iec104_transport` | adapter | 支撑 | `iec104_server_ports` | ROM 16 KB | 本产品、iEMS |
| `modules/edge_event` | module | 核心 | `event_store` | ROM 12 KB | 本产品、DCU、IEMS |
| `modules/edge_fault` | module | 核心 | `fault_source`、`fault_sink` | ROM 20 KB | 本产品、TTU（后续） |
| `modules/edge_fa` | module | 核心 | `fa_control`、`fa_signal` | ROM 24 KB | 本产品（FTU/DTU） |
| `cmd/dist_terminal` | product | 无 | 无 | 进二进制尺寸预算 | 终端产品 |

直接复用：`modules/edge_io`（`bus_orchestrator_setup_slave` 与 `foundation/include/edge_slave_plane.h`）、`adapters/io_drivers` 的 Modbus 与 DLT645 驱动、`foundation/src/protocol` 的 IEC104、Modbus、DLT645 编解码、`modules/edge_north` 的 SnF、`modules/edge_core` 的 DataHub、`adapters/sqlite_adapters`、`adapters/rest_api`、`modules/edge_app_host`、`web/core`。

依赖方向不变：`cmd` 依赖 `modules` 与 `adapters`，`modules` 只依赖 `foundation` 的端口头，`adapters` 实现端口。新组件必须过 `check_component_metadata`、`check_component_boundary`、`check_plane_isolation`、`check_public_structs`、`check_no_dynamic_memory`、`check_caller_allocated_storage`、SRP 1500 行，目标进 `check_component_targets.sh`。

### 4.1 组件要点

`adapters/iec104_transport` 管 TCP 监听、连接生命周期、APCI 状态、总召唤、时钟同步、遥控选择与执行；点号翻译走 `iec104_server_ports` 回调。迁移路径是先让 `cmd/iems` 改用它，用例全绿后删除 `cmd/iems/iems_north_iec104.[ch]`。多客户端首版支持 2 路（主站加联调），APCI 状态按连接隔离。

`modules/edge_event` 是有界环形缓冲，记录 `{seq, ts, point, old, new, quality, source}`，溢出计数单独暴露。时间通过端口注入，测试用假时钟。

`modules/edge_fault` 吃采样窗口，输出故障事件与故障记录。阈值来自定值，不写死。

`modules/edge_fa` 是状态机：`正常 → 故障启动 → 判定 → 跳闸 → 重合 → 成功/隔离 → 恢复`。只发控制意图给 `fa_control` 端口，由产品决定走哪个驱动与总线。集中型模式下隔离与恢复由主站决策，终端执行并上报。

`cmd/dist_terminal` 入口照 iEMS 风格：`main.c` 解析配置，装配函数按阶段创建聚合根、DataHub、总线、存储、北向、本地接口。点模型放产品本地（数字点号加静态表，运行期清单进 SQLite），不采用 DCU 的 OAD 原生寻址。

## 5. 现状限制

计划要正面处理，均按当前树核对：

- `cmd/iems/iems_north_iec104.c:52` 是产品本地实现且单客户端，主站与联调工具不能同时连。
- 104 无时钟同步：`foundation/include/iec_asdu.h:46` 只有 `IEC_TYPE_C_CS_NA_1` 枚举，没有处理分支。
- REST 控制写写死总线 0 与 ModbusRTU：`adapters/rest_api/src/rest_server.c:180-191`。
- 驱动会话私有区上限 1024 字节：`adapters/io_drivers/src/driver_iec104.c:75` 有断言。
- WASM 应用默认经 `edgeos-launcher` 启动，launcher 要求调用方 uid 0。
- `CONTEXT-MAP.md` §4.1 仍写「TagID 唯一真理源是 contracts」，与 ADR-0010（已接受，废除全局契约与 TagID）冲突。以 ADR-0010 为准，文档需在 M0 一并修正。

## 6. 协议方案

### 6.1 北向 IEC104 从站

- 端口默认 2404，t0/t1/t2/t3 与 k/w 可配。
- 支持 STARTDT/STOPDT、总召唤（站总召，QOI=20）、遥测 `M_ME_NC_1`、单点或双点遥信、遥控 `C_SC_NA_1`（含 SBO）、遥调 `C_SE_NC_1`、SOE 带 CP56Time2a、时钟同步 `C_CS_NA_1`。
- 选择确认、执行确认、否定确认（COT 44）分开记录并计数，联调时直接看计数定位。
- 待确认：双点遥信是否必需。建议首版带上，编解码已支持 `M_DP_NA_1`。

### 6.2 南向

- Modbus RTU/TCP：保护测控、开关柜、辅助装置。轮询计划进 SQLite，带上限、超时、重试与断路器（`edge_poll_policy_t` 已具备）。
- DL/T 645：电能表读取，用于电量与对时场景。
- 遥信去抖窗口进定值；SOE 时标取本机接收时刻，主站对时后一并校准。

### 6.3 安全

- 部署侧纵向加密认证装置或安全接入网关，终端预留端口与证书路径配置，不在首版实现加密协议。
- 遥控双校验：从站平面的写权限检查（`_slave_is_write_allowed`）加产品侧闭锁条件。
- WASM 分析应用跑在 app-host，受 launcher 的 uid、cgroup、seccomp 约束；部署文档必须写明 product 需要 root 启动。

## 7. 工作分解与排期

角色：A 架构，C C 开发，D 驱动开发，Q 验证，F 前端，H 硬件，P 产品。人日为估算，评审时按实际调整。

### M0 范围冻结（5 人日，第 1 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| DRT-M0-01 | 主站规约与终端形态决策 | A+P | 2 | 无 | `docs/adr/00xx-dist-terminal-scope.md` | 评审记录与结论 |
| DRT-M0-02 | FTU/DTU 点表与定值需求清单 | P+Q | 2 | 01 部分 | 两份点表模板与定值清单 | 评审通过，含点数上限 |
| DRT-M0-03 | 硬件基线与资源预算 | H+A | 1 | 无 | CPU/内存/串口/DI-DO/电源表 | 组件预算回填表 |
| DRT-M0-04 | 验收任务与模拟器清单 | Q | 1 | 01 | 任务清单与模拟器需求 | 四条验收任务命名冻结 |

出口：§10 的 1、2、3、6 有结论；`CONTEXT-MAP.md` 与 ADR-0010 的冲突修正单独立项。

### M1 骨架与四遥闭环（30 人日，第 2 到 4 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| DRT-M1-01 | `iec104_server_ports` 端口头 | A | 2 | M0-01 | `foundation/include/iec104_server_ports.h` | 端口评审，无协议类型泄漏 |
| DRT-M1-02 | `adapters/iec104_transport` 组件 | C | 8 | 01 | 组件与 `component.meta` | 两路客户端用例；边界与预算门禁绿 |
| DRT-M1-03 | iEMS 迁移并删除本地实现 | C | 4 | 02 | `cmd/iems` 改用适配器 | iEMS 全量用例绿，`iems_north_iec104.[ch]` 删除 |
| DRT-M1-04 | 时钟同步与时间端口 | D | 3 | 02 | `C_CS_NA_1` 处理加 SNTP 主备 | 模拟主站对时用例 |
| DRT-M1-05 | 产品壳与装配 | C | 5 | 01 | `cmd/dist_terminal` 骨架 | 起停、配置加载、优雅退出用例 |
| DRT-M1-06 | 点模型与 FTU/DTU 点表模板 | C+Q | 4 | M0-02、05 | 点定义、映射到 IOA | 点表加载与翻译单测 |
| DRT-M1-07 | REST 控制写改为点路由 | C | 3 | 06 | 修掉 `rest_server.c:191` 写死路径 | 写不同总线点均命中正确驱动 |
| DRT-M1-08 | 四遥验收任务与模拟主站 | Q | 3 | 02、05 | `acceptance/dist_terminal/tasks/iec104_four_remote.yaml` | 任务绿，证据归档 |

出口：四遥验收任务绿；iEMS 无回归；`run_guard_selftest.py` 绿；`cmd/dist_terminal` 进产品构建与尺寸预算。

### M2 南向真实化与 SOE（30 人日，第 5 到 7 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| DRT-M2-01 | `modules/edge_event` 组件 | C | 6 | M1-06 | 环形缓冲、溢出计数、端口 | 假时钟排序与溢出用例 |
| DRT-M2-02 | 事件持久化适配器 | C | 3 | 01 | SQLite `event_store` 实现 | 重启后事件可查 |
| DRT-M2-03 | Modbus 轮询接入 | D | 6 | M1-05 | RTU/TCP 轮询与断路策略 | PTY 仿真用例，含超时与重试 |
| DRT-M2-04 | DL/T 645 读表 | D | 4 | 03 | 645 采集路径 | PTY 仿真用例 |
| DRT-M2-05 | 遥信去抖与 SOE 落库 | C | 5 | 01、03 | 去抖定值加事件写入 | 去抖窗口与顺序用例 |
| DRT-M2-06 | SNTP 客户端 | D | 3 | M1-04 | 时间源主备 | 断网回退与偏差告警用例 |
| DRT-M2-07 | 验收：SOE、时钟、重启不丢 | Q | 3 | 02、05、06 | 验收任务 | 任务绿，证据归档 |

出口：SOE 顺序与溢出可验证；主站对时生效；重启后事件不丢。

### M3 故障检测与记录（30 人日，第 8 到 10 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| DRT-M3-01 | `modules/edge_fault` 组件 | C | 8 | M2-05 | 过流与零序检测、记录组装 | 注入采样用例，含边界 |
| DRT-M3-02 | 定值区聚合与切换审计 | C | 5 | M1-06 | `SettingGroup` 与审计记录 | 切换与越权用例 |
| DRT-M3-03 | 故障记录存储与查询 API | C | 4 | 01 | SQLite 表与 REST 查询 | 重启后可查，字段对齐验收 |
| DRT-M3-04 | 故障事件 104 上送 | D | 4 | 01、M1-02 | 遥信变化与 SOE 上送 | 模拟主站收到故障事件 |
| DRT-M3-05 | 验收：故障注入 | Q | 4 | 03、04 | 验收任务 | 任务绿，记录与主站报文一致 |
| DRT-M3-06 | 故障场景与定值评审 | P+Q | 2 | 02 | 场景与定值矩阵 | 评审记录 |

出口：注入过流与零序能产出可追溯记录；定值切换有审计。

### M4 馈线自动化（40 人日，第 11 到 14 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| DRT-M4-01 | `modules/edge_fa` 状态机 | A+C | 10 | M3-01 | 状态机与端口 | 状态机单测覆盖全部分支 |
| DRT-M4-02 | 集中型 FA 与主站时序 | C | 8 | 01、M1-02 | 跳闸、隔离、恢复时序 | 模拟主站时序用例 |
| DRT-M4-03 | 执行闭锁与看门狗预算 | C+D | 6 | 02 | 闭锁条件与超时 | 拒动与超时用例 |
| DRT-M4-04 | 重合闸与 FTU 角色差异 | D | 6 | 01 | FTU 时序与配置 | FTU 场景用例 |
| DRT-M4-05 | 验收：两开关故障场景 | Q | 6 | 02、03、04 | 验收任务 | 任务绿，时序证据归档 |
| DRT-M4-06 | TTU 复用评估 | A | 4 | M3-01 | 评估结论 | 是否立项的结论 |

出口：仿真馈线两开关故障按主站指令完成隔离与恢复；异常路径有用例。

### M5 维护、部署与整体验收（30 人日，第 15 到 17 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| DRT-M5-01 | 本地维护页面与 API | F+C | 8 | M3-03 | `web/products/dist_terminal` 页面 | 页面可用，E2E 与截图 |
| DRT-M5-02 | 日志导出与诊断 | C | 4 | M2-02 | 导出接口与格式 | 诊断包可复现问题 |
| DRT-M5-03 | 升级与回滚 | C | 5 | M1-05 | 升级路径与回滚 | 升级失败回滚用例 |
| DRT-M5-04 | 安全接口预留 | A+D | 4 | M0-03 | 加密装置端口与配置 | 接口评审 |
| DRT-M5-05 | 部署与运维文档 | A+Q | 4 | 01..04 | 部署手册 | 评审通过 |
| DRT-M5-06 | 全量验收与覆盖率下限 | Q | 5 | 全部 | 验收报告与覆盖率台账 | `acceptance/run.sh` 绿，目录下限登记 |

出口：全产品验收绿；新增目录有覆盖率下限；部署文档可照做。

合计约 165 人日，约 17 周。M2 与 M1 后半段可并行；M5 文档从 M3 起穿插。

## 8. 派活与跟踪

### 8.1 一任务一 issue 一 PR

- issue 标题：`[DRT-M1-02] adapters/iec104_transport 组件`。
- 标签：阶段 `M1`，组件 `component:iec104-transport`，类型 `type:feature`、`type:refactor`、`type:test` 之一，triage 用 `needs-triage`，评审后换 `ready-for-agent` 或 `ready-for-human`。
- 一个 issue 对应一个 PR，一个 PR 只做一件事。重构、缺陷、功能不混在一个 PR（仓库规则）。
- 里程碑：GitHub Milestone 按 M0 到 M5 建，issue 归属里程碑。

### 8.2 完成定义

任务的完成按仓库口径：静态分析通过、测试通过、有运行时验证证据。具体到本产品：

- 单测与 `run_guard_selftest.py` 绿；新组件过边界与预算门禁。
- 涉及协议或时序的任务，必须有模拟对端产生的证据（报文或日志）。
- 涉及数据的任务，必须证明重启后不丢或可恢复。
- 合并后在目标分支重跑守卫，单批通过不算。

### 8.3 每周节奏

- 周一派活：按 §7 的表认领 issue，确认依赖已合并。
- 周五演示：每任务给证据链接（测试输出、报文抓包、验收报告）。
- 进度表每周五更新，字段固定：ID、状态、owner、PR、证据、阻塞、预计完成日。
- 阻塞超过一天，升级到架构负责人，不允许静默等待。

### 8.4 进度总览（模板）

| 阶段 | 任务数 | 完成 | 进行中 | 阻塞 | 里程碑判据 |
| --- | --- | --- | --- | --- | --- |
| M0 | 4 | 0 | 0 | 0 | 规约、点表、硬件、验收清单冻结 |
| M1 | 8 | 0 | 0 | 0 | 四遥验收绿，iEMS 无回归 |
| M2 | 7 | 0 | 0 | 0 | SOE、时钟、重启不丢 |
| M3 | 6 | 0 | 0 | 0 | 故障记录可追溯 |
| M4 | 6 | 0 | 0 | 0 | FA 场景绿 |
| M5 | 6 | 0 | 0 | 0 | 全量验收与部署文档 |

### 8.5 依赖图

```mermaid
flowchart LR
    M0[M0 范围冻结] --> M1[M1 骨架与四遥]
    M1 --> M2[M2 南向与 SOE]
    M2 --> M3[M3 故障检测]
    M3 --> M4[M4 馈线自动化]
    M4 --> M5[M5 维护与验收]
    M1 -.可并行.-> M2
```

## 9. 验证与验收

- 单元：状态机、事件缓冲、点表翻译要有行为测试；时间相关用注入时钟。
- 仿真：Modbus 从站、104 主站模拟器。`acceptance/iems/iec104_client.py` 已存在，先复用，稳定后提升为公共工具。
- 验收任务：`acceptance/dist_terminal/tasks/` 至少四条，四遥、SOE 与时钟、故障记录、FA。产品壳照 `acceptance/iems/iems_product.py` 四步：准备 SQLite、起从站、分配端口、exec 产品。
- 合并后跑 `run_guard_selftest.py` 与 `test.sh`；CI 的 ASAN/TSAN/UBSAN、arm64、trimmed 矩阵覆盖新组件，不新增豁免。
- 覆盖率：新增目录在 `ci/baseline/coverage_floor.txt` 登记下限，只升不降。

## 10. 风险与开放问题

需人拍板：

1. 主站规约：IEC104、Q/GDW 1376.1，或两者都要。决定适配器边界与验收模拟器。
2. 首版是否覆盖 TTU。本方案按不覆盖排期，M4 留了评估任务。
3. FA 模式：集中型、就地型、智能分布式。本方案按集中型排期。
4. DTU 是否需要 IEC 61850。需要则单独立项。
5. 安全接入装置型号与协议，决定预留端口形态。
6. 硬件基线：CPU 与内存、串口数量、DI/DO 数量、是否双电源。
7. 入网检测与型式试验条目，验收任务按检测项补。
8. 遥控是否要求 SBO 与预置/撤销。建议首版支持，编解码已有基础。
9. 双点遥信是否必需。建议首版带上。
10. 主站并发连接数。当前实现单客户端，适配器要一次定清 `max_clients`。

技术风险：

- `cmd/dcu` 点模型是 OAD 原生，照抄会把 DLT698 对象模型带进来。本方案采用产品本地数字点号。
- 驱动会话私有区 1024 字节，新增协议驱动状态要算清楚。
- 104 单客户端限制阻塞联调，M1 必须解决。
- WASM 应用需要 root 启动 product，部署与联调文档要写清。
- `CONTEXT-MAP.md` §4.1 与 ADR-0010 冲突，若不修正，后续实现会被文档误导。

## Close the program

- [ ] §7 每个任务都已关闭，且每个关闭都有证据链接。
- [ ] `acceptance/run.sh` 全产品绿，`acceptance/dist_terminal/` 四条任务证据归档。
- [ ] `run_guard_selftest.py` 与 `test.sh` 在 `main` 上绿。
- [ ] 新增目录的覆盖率下限已登记，且不低于本次实测。
- [ ] 部署手册经现场或台架照做一遍，偏差记录在附录或 issue 里。

## Appendix A. Links and reading list

- `docs/design/slave_plane_architecture.md`（从站平面）
- `docs/design/NORTHBOUND_ARCHITECTURE.md`（北向两族模型）
- `docs/adr/0010-eliminate-global-contracts-and-tagid.md`（废除全局契约与 TagID）
- `docs/design/DOMAIN_CONTEXT_MAP.md`、`CONTEXT-MAP.md`（上下文与聚合清单）
- `docs/design/app-platform/03-abi-compatibility.md`、`08-events.md`（应用平台）
- `cmd/dcu/CONTEXT.md`、`cmd/iems/CONTEXT.md`（现有产品上下文）
- `GRID_CORE_PRD.md`（一核多角色的产品设想）

## Appendix B. Prototype evidence

本轮没有跑原型。§10 的开放问题里，1、2、3、6 是采购与规范决策，不是实验能回答的；4 与 9 可以在 spec 阶段用模拟器验证，届时按 `playbooks/prototype.md` 记录分支、SHA 与产物。
