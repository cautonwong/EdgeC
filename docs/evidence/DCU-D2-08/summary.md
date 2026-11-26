# DCU-D2-08 范围评估：6032–6035 采集状态/任务监控与 6040–6042 时标

状态：评估稿，未写实现。基线 `feat/dcu 40106f22`，分支 `dcu/D2-08-status`。

## 1. 标准形状（`docs/698_parsed.md` 8582–8625，以 PDF 表 A.7 为准）

- 6032 类 11，A2 = array 一个采集状态；6033 类 8 = structure { 通信地址 TSA, 中继级别 unsigned, 中继地址 TSA, 端口 OAD, 最后一次采集成功时间 date_time_s, 采集失败次数 unsigned, 相位 enum{未知0,A1,B2,C3}, 相序异常 enum{正常0,LN互易1,逆相序2} }。
- 6034 类 11，A2 = array 采集任务监控单元；6035 类 8 = structure { 任务 ID unsigned, 执行状态 enum{未执行0,执行中1,已执行2}, 执行开始时间 date_time_s, 执行结束时间 date_time_s, 采集总数量 long-unsigned, 采集成功数量 long-unsigned, 已发送报文条数 long-unsigned, 已接收报文条数 long-unsigned }；标准注明成功数、收发条数每次任务执行先清零。
- 6040/6041/6042 类 8，A2 = date_time_s（采集启动/成功/存储时标）。

目录（`DCU_698_OI_CATALOG.md`）：6032/6034 为 11，6033/6035/6040/6041/6042 为 8；七项都进对象模型。ACL 沿用 6013/601D/601F 的“单元类型也进 ACL”先例，七项都进 ACL：对象 47→54、ACL 46→53、`EDGE_SLAVE_ACL_MAX_OBJECTS` 46u→53u。

## 2. 现有真实运行时状态（只列可复用来源）

- `dcu_poller` 每表状态 `dcu_meter_poll_state_t`：`consecutive_fails`、熔断标志、`task_mask`；成功时清零。
- 通道统计 `edge_poll_stats_t`：`success_count`、`consecutive_fails`、`last_success_ts`/`last_poll_ts`（`uv_hrtime` 单调毫秒，不是墙钟）。
- `dcu_task_unit_t.monitor`（struct 注释已标 OI 6035）：`task_id/status/total_meters/success_meters/total_packets` 字段存在，但当前只有 `success_meters++` 一处更新；`total_meters`、`total_packets`、状态与起止时间都没有真实写入点。
- 任务执行：`_task_due` 每步判定到期并更新 `last_run_time`（单调）；通道链异步推进，**没有“任务执行完成”事件**，也没有“执行中”标志。
- 落库：`dcu_storage_write_telemetry` 由 poller sink 逐点调用，`dcu_node` 有 `saved_since_take` 计数；没有记录墙钟。
- 墙钟：`dcu_clock_port_system`（`time(NULL)`）与 `dcu_698_put_date_time_s`（date_time_s 编码）可用。
- 相位：`dcu_hplc_identify_single_phase` 求解器存在，但**没有接到采集路径**，每表相位无真实来源。
- 中继：产品只有控制 OAD 的“继电器映射”概念（OI 8000），**没有中继级别/中继地址的路由拓扑**。

## 3. 逐字段来源判定

| 字段 | 真实来源 | 判定 |
| --- | --- | --- |
| 6033 通信地址 TSA | `production_meter_t.address`（6 位串→TSA octets） | 可做 |
| 6033 端口 OAD | `meter->channel_id` → OAD{0,0,0,port}（与 6001 Basic 同编码） | 可做 |
| 6033 采集失败次数 | `mp->consecutive_fails`（连续失败，成功清零，与 PRD 一致） | 可做 |
| 6033 最后一次采集成功时间 | 需在 `_chain_finish_visit` 成功分支新增每表墙钟纪元 | 可做（新增真实写入点） |
| 6033 相位 | 求解器未接采集；enum 有“未知 0” | 可做（诚实回 0，注明未识别） |
| 6033 相序异常 | **无检测器，且 enum 没有“未知”值** | **停：不能填 0（谎称正常）** |
| 6033 中继级别/地址 | **无路由拓扑来源** | **停**（若允许，0 + 空 TSA 表示“无中继直连”需你拍板） |
| 6035 任务 ID | `task_id` | 可做 |
| 6035 执行状态 | 只有 `last_run_time` 是否非 0；“执行中/结束”没有事件 | **部分停**（见未决 3） |
| 6035 开始时间 | 到期判定处新增墙钟 | 可做 |
| 6035 结束时间 | 无任务完成事件 | **停**（需定义“一轮结束”的判据） |
| 6035 采集总数量 | 到期任务按 `task_mask` 过滤后的表数（真实可算） | 可做 |
| 6035 采集成功数量 | 已有 `success_meters++`（按到期任务/表掩码归属） | 可做（补齐清零与语义） |
| 6035 已发送/已接收报文条数 | poller 只知逻辑抄读提交/回调；**总线重试（645 备用 DI）会多发帧，poller 不知道帧数** | **停或定义为“逻辑事务数”**（见未决 2） |
| 6040 启动时标 | 任务到期处新增墙钟 | 可做 |
| 6041 成功时标 | 成功结算处新增墙钟（每表或全局“最近一次成功”） | 可做（需定义范围） |
| 6042 存储时标 | `dcu_storage_write_telemetry` 调用点新增墙钟 | 可做 |

## 4. 建议形态（若下列未决项拍板后实施）

- 运行时状态放 poller/node 内存，不落 SQLite；重启后计数清零、时间为未定义（证据写清）。
- 6032 A2：按 `config_seq` 升序对每块表编码 6033；6033 单元元素索引 = `config_seq`（沿用 6001/5004 的“元素=配置序号”先例）。
- 6034 A2：按任务注册顺序（task_id 升序）对已配置任务编码 6035；6035 元素索引 = task_id（沿用 6013 的寻址先例）。
- 每任务执行开始：真实路径（`_task_due` 命中）清零成功数/收发计数并记开始时间与总数量；“结束”需要一个真实完成判据（见未决 3）。
- 6040/6041/6042 取“最近一次采集”的启动/成功/存储墙钟；6041 建议全局最近成功（与 6033 每表时间并存），6042 在落库调用点更新。
- 坏属性/坏元素：6040–6042 非 A2 回 DAR 5/4；6033/6035 元素越界 DAR 6；6032/6034 无数据回空数组。
- 对象/ACL/cap 如 §1；不动 foundation 其他头、不改 adapters/modules。

## 5. 未决项（需 verifier 拍板）

1. **相序异常**：无检测器且无“未知”枚举。选项：a) 6032/6033 整项回 DAR 2（暂时失效，不假装正常）；b) 引入相序检测（新功能，超出 D2-08）；c) 允许回“正常 0”并注明“产品未检测”——我不建议。
2. **6035 收发报文条数**：a) 定义为“本任务逻辑抄读提交/完成次数”（真实事件，但 645 备用重试时少于线上帧数）；b) 拒填（字段 0 也是假值）；c) 等总线暴露按任务的帧计数（跨层改动，需停）。
3. **6035 执行状态与结束时间**：a) 定义“执行中”= 本任务到期后到本轮链扫完；需要给 poller 增加真实的 per-task 窗口完成判据（链是异步的，当前没有）；b) 只回 未执行(0)/已执行(2)，不出现“执行中”，结束时间=本任务最后一次提交时间；c) 回 DAR 2。
4. **中继级别/地址**：a) 0 + 空 TSA 表示直连（需确认标准语义允许 0）；b) 6032/6033 回 DAR 2；c) 等拓扑。
5. **6033/6035 元素寻址**：按 §4 提议（config_seq / task_id），确认。
6. **6041 成功时标范围**：全局最近成功 vs 与 6033 同源的每表最近成功（元素索引选表）；提议全局。

## 6. §9.5 核对

- 依赖 D2-03 已合并（6012/6013 任务在 feat/dcu）；不涉及 modules/edge_core、受保护 foundation 头或 ADR；ACL cap 的 `edge_slave_plane.h` 不在 `check_core_readonly.sh` 清单。
- 不新造调度：状态只挂现有 poller 链与落库路径。若未决 2/3 选择需要跨层帧计数或新的任务完成判据，可能触发“超出最小修复”，届时按 §9.5 停。

结论：6033 的相序异常、中继级别/地址、6035 的帧计数/执行中/结束时间没有诚实来源，未决 1–4 需要拍板后才能实现；不建议先填默认值。请就未决项裁决，我再按裁决开实现切片（失败测试 → 实现 → 证据），门禁照旧。

## 7. 切片 A 落地（6032–6035 对象/DAR 2、6040–6042 时标）

- 七项 OI 全部进对象模型与 ACL：对象 47→54、ACL 46→53、`EDGE_SLAVE_ACL_MAX_OBJECTS` 46u→53u；静态断言与计数测试同步。
- 6032/6033：按裁决整项 GET 返回 DAR 2（相序/中继无诚实来源，失败次数因此本单不可读）；6034/6035 同样暂回 DAR 2（见 §8）。
- 6040/6041/6042：poller 在真实路径记录墙钟纪元——任务到期（6040）、一次访问成功（6041）、`record_telemetry` 落库成功（6042）；编码用 `dcu_698_put_date_time_s`；从未发生回 DAR 2。运行时只在内存，重启归零。
- 非 A2 属性按类定义回 DAR 5 / DAR 4；6032 数组本单 DAR 2，不复核元素。
- 测试：`test_acq_timestamps_and_status_dar`（未采集 DAR 2；受控 poller 一次成功轮询后三个时标为 date_time_s 且年份合理；6032–6035 DAR 2；非 A2 属性 DAR 4）；计数测试与对象列表测试同步；验收新增 `D_00_00_02_000030`。
- 验证：受影响 ctest 19/19（含 poller/polling_loop）；ASAN detect_leaks=1：scheme_mgmt 16 / rules 10 / schemes 12 / node 3 / poller 9 / polling_loop 2，0 泄漏；验收 2/2（3 + 30）；SRP 干净；guard 105/105；warning 0（clean Ninja Release）；fanalyzer 3 known / 0 new。

## 8. 6034/6035 停：任务执行完成判据与按任务收发计数超出本切片

裁决 3 要求“任务到期置执行中；本轮该任务所有读回调结束后置已执行；per-task 未完成计数由 poller 维护”。真实实现需要：

1. poller 增加每任务执行状态机：到期激活（开始墙钟、总表数=task_mask 过滤后的表）、每表在本轮是否已访问的 visited 位图（异步链跨多回调、通道独立推进、熔断跳过、档案删除重排都要处理）、per-task outstanding 读计数、完成判据 = 所有目标表各访问一次且 outstanding=0；档案变更需中止/重开执行。
2. 裁决 2 的“每次 `edge_api_read_tag` 提交（含备用 DI 重试）计一次发送、收到响应的回调计一次接收”要求按任务归属：poller 把每个 pending tag 的 task 位掩码随提交传下去，`bus_source` 在每次真实 `edge_api_read_tag`（含重试）与响应回调里回调 poller 计数。这需要扩展 `poller_source_ops_t.submit` 契约（现有 7 个实现者：bus/twin source + 5 个测试桩），并新增 poller 的 note_tx/note_rx API；`bus_source` 已有 node→poller 访问器。
3. 接收计数只能按现有状态区分：`CMD_STATUS_TIMEOUT` 不计接收，成功与 `CMD_STATUS_PROTO_ERROR` 计接收；twin_source（demo）没有总线回调，需要单独定义。

结论：这不是“填一个结构”而是调度器契约与完成语义的改动，超过本切片“最小修复”；按裁决 3 的停条件停在这里，不用“最后提交时间”假装结束时间。6034/6035 继续 DAR 2（与 6032/6033 同口径，不返回假计数）。请批准以下之一：
- a) 批准上述契约扩展（`submit` 增加 task 掩码 + poller note_tx/note_rx + poller 执行状态机），作为 D2-08b 实现切片；
- b) 收窄定义（例如“执行中/结束时间”只对同步源有意义，或收发计数只统计逻辑事务），我按新口径实现；
- c) 6034/6035 长期保持 DAR 2 并写入计划风险。

## 9. D2-08b 落地：6034/6035 任务监控与按任务收发计数

- 契约：`poller_source_ops_t.submit` 携带该 pending 项的到期/执行中任务位掩码；poller 新增 `dcu_poller_note_tx/note_rx`，只统计执行中的任务。`bus_source` 在每次真实 `edge_api_read_tag`（含 645 备用 DI 重试）记发送，回调 `SUCCESS`/`PROTO_ERROR` 记接收、`TIMEOUT` 不计；twin/demo 合成源无总线回调，不维护 6035 收发计数（代码注释写明，验收不用）。
- 任务执行状态机（poller 内，不新造调度）：到期且未在执行 → 执行中、开始墙钟、总表数（`task_mask` 过滤）、成功/收发清零、visited 清零；每个目标表在本轮首次被链访问时记 visited（熔断跳过也算访问），任务在途读由 `outstanding` 计数；所有目标表各访问一次且 `outstanding=0` → 已执行、结束墙钟。成功数只在每表本轮首次访问成功时计一次（同一执行跨两圈不重复计）。档案增删使执行重开本轮（visited 清零、总表数重算），计数继续。
- 6034 A2 按 task_id 升序编码已配置任务（跳过 FREE 槽）；6035 elem = task_id，未知回 DAR 6。未执行/执行中缺的一侧时标按计划 §11.1.10 用 year=FFFF 的非法值表示“无”；long-unsigned 按标准 2 字节编码。
- 6032/6033 保持 DAR 2。
- 运行时状态只在内存，重启清零；计数在任务开始清零。

验证（最终树）：受影响 ctest 20/20（scheme_mgmt 17 含状态机用例，poller/polling_loop/scale_benchmark）；ASAN detect_leaks=1：17/10/12/3/9/2，0 泄漏；验收 2/2（dcu-acceptance 3、northbound 31；新用例 `D_00_00_02_000031` 让主用与备用 DI 都失败，断言 total=40、success=0、sent/recv ≥2×40（含备用重试与两次错误响应），6034 含任务；恢复后 success=40）；SRP 干净；guard 105/105；warning 0（clean Ninja Release）；fanalyzer 3 known / 0 new。

## 10. D2-08b 评审修复

- 完成判据：`_task_maybe_complete` 增加“所有链上没有该任务的 pending 项”守卫（`_task_has_pending`），并移除 `_on_read_done` 里 pump 之前的提前结算——最后一个回调后由 pump 的 `finish_visit`/步末统一结算。回归：1 表 2 tag，第一个回调后仍“执行中”，第二个回调后才“已执行”，sent/recv 各 2。
- 档案变更：按新执行重开——更新开始墙钟、清 success/sent/recv/visited、重算 total，保留 outstanding；同时清掉各链的 `visit_new_mask`，避免重开前的“首次访问”在新执行里重复计成功。回归：执行中增表后 total=2、success=0、sent=0；驱动回调后 success ≤ total 且新表被计。
- 发送计数：`note_tx` 移到 `edge_api_read_tag` 返回 0 之后（主用与备用重试一致）；提交立即失败不计发送。回归：拒绝提交的桩，sent/recv 均不增。

验证（最终树）：受影响 ctest 20/20（scheme_mgmt 20 含三条回归）；ASAN detect_leaks=1：scheme_mgmt 20 / rules 10 / schemes 12 / node 3 / poller 9 / polling_loop 2，0 泄漏；验收 2/2（3+31，D_00_00_02_000031 恢复后 success=40）；SRP 干净；guard 105/105；warning 0；fanalyzer 3 known/0 new。
