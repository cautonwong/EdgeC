# 配网终端开发方案与计划

> 日期：2026-11-21
> 状态：方案草案，待评审（按仓库选择流程，评审后再进入 `/grill-embedded` 与 `/to-spec-embedded`）
> 读者：产品、架构、验证与实现负责人

## 1. 结论

配网终端不是一个新固件，而是 EdgeC 组件库上的第四个产品装配。约 60% 的能力已经在仓库里（IEC104 编解码、从站平面、驱动框架、DataHub、SnF、SQLite、App 平台），真正缺的是三块可复用组件与一个薄产品壳：

1. `adapters/iec104_transport`：把 IEC104 从站从 `cmd/iems` 下沉为适配器，供所有产品复用。
2. `modules/edge_event`：有界事件缓冲（SOE）与持久化端口。
3. `modules/edge_fault` 与 `modules/edge_fa`：故障检测与馈线自动化状态机。

产品壳 `cmd/dist_terminal` 只做装配，FTU 与 DTU 用配置区分角色。首版不做 IEC 61850、不做就地型 FA、不做 DL/T 698 安全层，这些进入开放问题清单。

## 2. 目标与范围

### 2.1 做什么

- 面向配电自动化主站的终端：遥测、遥信、遥控、遥调（四遥），SOE，故障检测与记录，集中型馈线自动化配合。
- 北向对配电主站：IEC 60870-5-104 从站（DL/T 634.5104）。
- 南向：Modbus RTU/TCP 对保护测控与开关柜，DL/T 645 对电能表。
- 本地维护：REST/UDS 查询与调试、点表与定值管理、事件查看。
- 验收：在 `acceptance/dist_terminal/` 下用 test-platform 跑，主站与从站都是模拟器。

### 2.2 不做什么（首版）

- IEC 61850 MMS/GOOSE（站所终端需要时单独立项）。
- 就地型与智能分布式 FA（先做集中型，模式差异见 §7）。
- DL/T 698 的安全层与 ESAM（现有 698 编解码不含安全）。
- 纵向加密装置驱动（由部署侧接入，接口留出来）。
- 新的全局点表或契约编译器（ADR-0010 已废除）。

### 2.3 与现有产品的关系

| 产品 | 定位 | 复用到配网终端的部分 |
| --- | --- | --- |
| `cmd/dcu` | 目录名是集中器，而 `cmd/dcu/CONTEXT.md:3` 自述为「配电智能融合终端 / 低压能源控制器 (TTU)」 | 104 驱动、点表形态、验收壳 |
| `cmd/iems` | 工业能源管理 | IEC104 从站实现、控制路由、验收壳 |
| `cmd/smart_box` | 养老健康终端 | App 平台使用方式 |
| `cmd/dist_terminal`（新） | FTU/DTU 角色 | 以上组件的消费者 |

## 3. 现状盘点：可复用组件与限制

### 3.1 直接复用

| 能力 | 组件 | 证据 |
| --- | --- | --- |
| 从站平面 | `modules/edge_io`：`bus_orchestrator_setup_slave`；端口在 `foundation/include/edge_slave_plane.h` 的 `resolve/accept/close_link` | `docs/design/slave_plane_architecture.md` |
| IEC104 编解码 | `foundation/src/protocol/iec104_apci.c`、`iec_asdu_codec.c`、`iec101_link.c` | 状态机 + ASDU + 链路层 |
| IEC104 驱动 | `adapters/io_drivers/src/driver_iec104.c`，主站与从站双角色：`req_slave_exchange` 处理 STARTDT、总召唤、遥控遥调 | 文件头注释 |
| Modbus | `foundation/src/protocol/modbus_codec.c`、`adapters/io_drivers/src/modbus_{rtu,tcp}_driver.c` | RTU CRC / TCP MBAP |
| DL/T 645 | `foundation/src/protocol/dlt645_codec.c`、`driver_dlt645.c` | 帧编解码与写点 |
| DL/T 698 | `foundation/src/protocol/dlt698_codec.c` | 应用层编解码，无安全 |
| 数据通道 | `modules/edge_core` DataHub：publish、shadow、订阅、WAL、快照 | `modules/edge_core/include/datahub.h` |
| 断点续传 | `modules/edge_north`：`snf_manager.c`，`snf_store_port_t` 至少一次投递 | `modules/edge_north/include/snf_manager.h` |
| 本地存储 | `adapters/sqlite_adapters` | point 表、事件表、定值 |
| 应用宿主 | `modules/edge_app_host` + `sdk/wit/app.v1.wit`（6 个已实现导入） | 可选的线路损耗等分析应用 |
| 前端 | `web/core` + `web/products/*` 工作区 | 复用页面与 SDK |

### 3.2 现有限制（计划要正面处理）

- `cmd/iems/iems_north_iec104.c` 是产品本地实现，且**单客户端**（`iems_north_iec104.c:52` 拒绝第二路）。主站与本地调试工具不能同时连。
- 104 驱动**未实现时钟同步**：`IEC_TYPE_C_CS_NA_1` 只在枚举里（`foundation/include/iec_asdu.h:46`），没有处理分支。SOE 打时标前必须补。
- REST 写路径**写死总线 0 与 ModbusRTU**：`adapters/rest_api/src/rest_server.c:180-191`。配网终端的控制落点必须按点路由，这个限制先修。
- 驱动会话私有区上限 `DRIVER_PRIV_SIZE = 1024` 字节（`driver_iec104.c:75` 断言）。新增协议驱动必须在这个预算内。
- WASM 应用默认经 `edgeos-launcher` 启动，launcher 要求调用方 uid 0。部署与本地调试都要知道这一点。
- `modules/edge_io` 没有字面意义的 master link 文件，主站角色在 `engines/master_engine.c` 与 `bus_orchestrator_setup_master`。

## 4. 组件设计（先定组件，再定产品）

### 4.1 组件地图

新增组件按仓库规则登记 `component.meta`：subdomain、ports、ROM/RAM/栈预算、owner、maturity。预算先给初值，实现后按归档尺寸回填，只降不涨。

| 组件 | 类型 | subdomain | 端口 | 初始预算 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `adapters/iec104_transport` | adapter | 支撑 | `iec104_server_ports` | ROM 16 KB | 从 `cmd/iems` 下沉的 104 从站；支持多客户端配置 |
| `modules/edge_event` | module | 核心 | `event_store` | ROM 12 KB | 有界 SOE 环形缓冲、序号、溢出计数、持久化端口 |
| `modules/edge_fault` | module | 核心 | `fault_source`，`fault_sink` | ROM 20 KB | 过流、零序、突变检测与故障记录组装 |
| `modules/edge_fa` | module | 核心 | `fa_control`，`fa_signal` | ROM 24 KB | 馈线自动化状态机（集中型） |
| `cmd/dist_terminal` | product | — | — | 二进制预算进 `scripts/check_binary_sizes.sh` | 薄装配壳，角色由配置决定 |

依赖方向不变：`cmd` 依赖 `modules` 与 `adapters`，`modules` 只依赖 `foundation` 的端口头，`adapters` 实现端口。

### 4.2 `adapters/iec104_transport`

- 负责：TCP 监听、连接生命周期、APCI 状态、ASDU 编解码、总召唤、时钟同步、遥控选择/执行。
- 不负责：点号到业务点的翻译。翻译通过 `iec104_server_ports` 的 `resolve`/`accept` 回调交给产品，沿用 `edge_slave_plane.h` 的契约。
- 迁移路径：先让 `cmd/iems` 改用它，`cmd/iems` 的测试全绿后删除 `cmd/iems/iems_north_iec104.[ch]`（迁移调用方后删除旧实现，不留双路径）。
- 多客户端：配置项 `max_clients`，首版 2（主站加本地联调工具）。多客户端共享同一 SlaveEngine 时需要按客户端隔离 APCI 状态，这一点在组件设计阶段定稿。
- 时钟同步：接收 `C_CS_NA_1` 后调用时间端口；同时提供 SNTP 客户端（`adapters/linux_platform` 已有网络能力）作为主备。

### 4.3 `modules/edge_event`

- 数据结构：固定容量环形缓冲，记录 `{seq, ts, point, old, new, quality, source}`。溢出计数单独暴露，不静默丢。
- 端口：`event_store`（insert/fetch/mark_persisted/count）。
- 复用方：本产品做 SOE；`cmd/dcu` 的 `event_history` 与 IEMS 的事件表后续可迁移过来，迁移前不删旧表。
- 时间来源：注入 `now_ms`，测试用假时钟验证排序与溢出。

### 4.4 `modules/edge_fault` 与 `modules/edge_fa`

- `edge_fault`：吃采样窗口，输出故障事件与故障记录（启动时间、相别、峰值、持续时间、定值命中项）。检测阈值来自定值，不写死。
- `edge_fa`：状态机 `正常 → 故障启动 → 判定 → 跳闸 → 重合 → 成功/隔离 → 恢复`。只发控制意图给 `fa_control` 端口，由产品决定走哪个驱动与总线。集中型模式下，隔离与恢复的决策由主站下发，终端只做执行与状态上报。
- 两个组件分开的理由：故障检测对 TTU 与台区融合终端也有用，FA 编排只对 FTU/DTU 有意义，拆开后复用面不同。

### 4.5 `cmd/dist_terminal`

- 入口保持 iEMS 风格：`main.c` 解析环境与配置，装配函数按阶段创建聚合根、DataHub、总线、存储、北向、本地接口。
- 点模型放产品本地（iEMS 风格的数字点号加静态表，运行期清单走 SQLite），**不采用** DCU 的 OAD 原生寻址，避免把 DLT698 对象模型绑进配网终端。
- 角色：`role: ftu` 与 `role: dtu` 决定间隔数量、默认点表模板与是否启用 FA 组件。

### 4.6 需要登记的门禁

新增组件必须同时满足：`component.meta` 完整、`check_component_boundary`、`check_plane_isolation`、`check_public_structs`、`check_no_dynamic_memory`、`check_caller_allocated_storage`、SRP 1500 行、组件目标进 `check_component_targets.sh`、正反夹具登记进 `run_guard_selftest.py`。每修一个缺陷同步加守卫或夹具，这是仓库的既定规则。

## 5. 领域模型

| 对象 | 关键字段 | 说明 |
| --- | --- | --- |
| 间隔（bay） | id、名称、所属馈线、包含设备 | DTU 多间隔；FTU 通常 1 个 |
| 设备（device） | id、协议、总线、从站地址、点表 | 来自 SQLite 清单 |
| 点（point） | id、kind（YC/YX/YK/YT）、scale、deadband、soe 标记、quality | 产品本地点号，不引入全局 TagID |
| 事件（event） | seq、ts、point、old、new、quality | SOE 队列元素 |
| 故障记录（fault） | 触发点、时间窗采样、相别、峰值、定值命中、结论 | 由 `edge_fault` 产出 |
| 定值区（setting group） | 组号、生效标记、点值集合 | 遥控切换生效组，变更进审计 |
| FA 状态 | 当前状态、当前故障实例、跳闸历史、重合次数 | `edge_fa` 状态机 |

点号规则：`point_id` 由产品分配，遥测 1xxx、遥信 2xxx、遥控 3xxx、遥调 4xxx 分段，映射到 104 的 `(type_id << 24) | ioa` 在适配器内完成。设备槽位沿用 IEMS 的 `device_id * 16 + offset` 思路，容量上限在 spec 阶段定。

## 6. 协议方案

### 6.1 北向 IEC104（首版）

- 服务端角色，端口默认 2404，参数（t0/t1/t2/t3、k/w）可配。
- 支持：STARTDT/STOPDT、总召唤（站总召，QOI=20）、遥测 `M_ME_NC_1`、遥信 `M_SP_NA_1`、遥控 `C_SC_NA_1`（含选择/执行）、遥调 `C_SE_NC_1`、SOE 带 CP56Time2a。
- 命令确认：选择确认、执行确认、否定确认（COT 44）分开记录，便于主站侧定位。
- 时钟同步：`C_CS_NA_1` 落到时间端口。
- 待确认：是否需要 `M_DP_NA_1`（双点遥信）。配网开关位置通常用双点，建议首版就带，编解码已支持。

### 6.2 南向

- Modbus RTU/TCP：保护测控、开关柜、辅助装置。轮询计划进 SQLite，带上限、超时、重试与断路器（`edge_poll_policy_t` 已具备）。
- DL/T 645：台区电能表读取，用于需要电量对时的场景。
- 遥信去抖与 SOE：去抖窗口进定值，事件入 `edge_event`，时间戳用接收到本机的时刻，主站对时后一并校准。

### 6.3 安全

- 部署侧纵向加密认证装置或安全接入网关，终端侧预留端口与证书路径配置，不在首版实现加密协议。
- 遥控双重校验：ACL（从站平面已有写权限检查）加产品侧操作票/闭锁条件。
- WASM 分析应用跑在 app-host，受 launcher 的 uid、cgroup、seccomp 约束；部署文档必须写明需要 root 启动 product。

## 7. 分阶段计划

估算按 2 名 C 开发、1 名测试（并行）、0.5 名前端。每周可交付单元，每个阶段结束都有可检查的出口。

### M0 范围冻结（1 周）

- 交付：本方案评审记录、主站规约选定、点表与定值需求清单、`docs/adr` 新增一条「配网终端组件划分」。
- 出口：评审通过，§9 的开放问题 1、2、3、6 有结论。

### M1 骨架与四遥闭环（3 周）

- 交付：
  - `adapters/iec104_transport` 组件与 `iec104_server_ports` 端口头。
  - `cmd/dist_terminal` 装配壳、点模型、SQLite 清单与轮询计划。
  - `cmd/iems` 迁移到新适配器并删除本地实现。
  - 总召唤、遥测遥信上送、遥控遥调下发。
- 出口：
  - `acceptance/dist_terminal/tasks/iec104_four_remote.yaml` 绿，主站用模拟器。
  - `cmd/iems` 全量用例绿（迁移无回归）。
  - `run_guard_selftest.py` 绿，新组件通过边界与预算门禁。
  - REST 控制写改走点路由，`rest_server.c:191` 的写死路径消失。

### M2 南向真实化与 SOE（3 周）

- 交付：Modbus RTU/TCP 实际轮询、DL/T 645 读表、`edge_event` 组件、时间同步（104 加 SNTP）、遥信去抖。
- 出口：PTY 仿真的保护装置与开关柜用例；SOE 顺序与溢出用例；主站时钟同步用例；重启后事件不丢（`snf_store` 加 SQLite 实现）。

### M3 故障检测与记录（3 周）

- 交付：`modules/edge_fault` 组件、定值区与生效切换、故障记录存储与查询接口、故障事件上送。
- 出口：注入过流与零序采样后产出故障记录，字段与验收用例对齐；定值切换走审计；故障记录写入后重启可查。

### M4 馈线自动化（4 周）

- 交付：`modules/edge_fa` 状态机、集中型 FA 与主站的配合时序、执行闭锁与看门狗预算。
- 出口：仿真一条馈线的两开关故障场景，终端按主站指令完成隔离与恢复；状态机单测覆盖全部分支；异常路径（遥控超时、开关拒动）有用例。

### M5 安全接口、维护与整体验收（3 周）

- 交付：本地维护页面与 API、日志导出、升级与回滚、验收任务全量、部署文档。
- 出口：`acceptance/run.sh` 全产品绿；`check_coverage` 对新增目录有下限；性能门槛（主站心跳、总召唤响应、遥控端到端时延）有记录。

合计约 17 周。M2 与 M1 的后半段可以并行，M5 的文档工作从 M3 起穿插进行。

## 8. 验证与验收

- 单元：新组件的状态机、事件缓冲、点表翻译要有行为测试；时间相关的用注入时钟，避免 sleep。
- 仿真：Modbus 从站与 104 主站模拟器。`acceptance/iems/iec104_client.py` 已存在，先复用，稳定后提升为公共测试工具。
- 验收任务：`acceptance/dist_terminal/tasks/` 下至少四个任务，四遥、SOE 与时钟、故障记录、FA。产品壳照 `acceptance/iems/iems_product.py` 的四步做：准备 SQLite、起从站、分配端口、exec 产品。
- 合并后跑：`run_guard_selftest.py` 与 `test.sh` 必须在合并后的分支上重跑，单批通过不算数。
- CI 已有的矩阵（ASAN/TSAN/UBSAN、arm64、trimmed）照常覆盖新组件，不新增豁免。

## 9. 风险与开放问题

需要人拍板：

1. 主站规约最终是 IEC104、Q/GDW 1376.1，还是两者都要。这决定适配器边界与验收模拟器。
2. 首版是否覆盖 TTU。TTU 的定位不同，点模型与 FA 都不需要，但可以复用 `edge_event` 与点表框架。
3. FA 模式选集中型、就地型还是智能分布式。本方案按集中型排期。
4. DTU 是否需要 IEC 61850。需要则单独立项，MMS 与 GOOSE 都不在现有编解码里。
5. 安全接入装置型号与协议，决定预留端口形态。
6. 硬件基线：CPU 与内存预算、串口数量、DI/DO 数量、是否双电源。组件预算要按硬件回填。
7. 入网检测与型式试验的条目，验收任务要按检测项补。
8. 遥控是否要求选择/执行（SBO）与预置/撤销。建议首版支持，编解码已有基础。
9. 双点遥信是否必需。建议首版带上。
10. 主站并发连接数。当前 104 实现为单客户端，适配器设计要一次把 `max_clients` 定清楚。

已知技术风险：

- `cmd/dcu` 的点模型是 OAD 原生，照抄会把 DLT698 对象模型带进来。计划明确采用产品本地数字点号。
- 驱动会话私有区只有 1024 字节，新增协议驱动的状态要算清楚。
- 104 单客户端限制会阻塞“主站加联调工具同时在线”的场景，M1 必须解决。
- WASM 应用需要 root 启动 product，部署与联调文档要写清，别让现场以为是故障。

## 10. 参考

- `docs/design/slave_plane_architecture.md`（从站平面，AEP-403）
- `docs/design/NORTHBOUND_ARCHITECTURE.md`（北向两族模型）
- `docs/adr/0010-eliminate-global-contracts-and-tagid.md`（废除全局契约与 TagID）
- `docs/design/app-platform/03-abi-compatibility.md`、`08-events.md`（应用平台事件与 ABI）
- `cmd/dcu/CONTEXT.md`、`cmd/iems/CONTEXT.md`（现有产品上下文）
- `CONTEXT.md`（Grid-Core 上下文定义）
- `GRID_CORE_PRD.md`（一核多角色的产品设想）
