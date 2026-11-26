# DCU-D0-20：D0-18/19 对抗评审修复轮

状态：F1/F2/F3 修复 + 回归测试；F7 熔断状态机；F6 双重 unref 按所有权收口；
A1/A2/F4/F5 登记为后续单元（A1/A2 已由执行快照结构性修复，见 §7）。分支
`dcu/D0-18-execution-rootcause`，基于 `85c457c0`（已含 feat/dcu cde627a0 / main
beb6629b）。改动面：poller 资格/重开清理与注释、验收测试等待语义、edge_io 的
`IO_EVENT_RX` 载荷所有权（master/slave 引擎回调不再释放借用载荷）。

## 1. 修复（本 diff 引入的缺陷）

### F1 同一执行内重复提交（任务暂停）

`_chain_load_pending` 只查 `monitor.status==1`，`_task_note_visit` 还查
`task->status==NORMAL`。任务 DISABLED→NORMAL 翻转期间装载的表不记 visited，恢复后
同一执行重新装载重发（probe `test_transient_disable_reloads_same_meter`）。

修：两处使用同一资格谓词（monitor.status==1、task NORMAL、mask 允许）。
回归：`tests/test_dcu_poller.c::test_transient_disable_does_not_reload`
（2 表 3 tag、异步、暂停期间零装载、恢复后每 (meter,tag) 只一次）。反向验证：
去掉该谓词，测试红。

### F2 执行重开不清链上 pending

档案 revision 变化重开执行时只清 visited/succeeded/计数，`pending_n/pending_i`
保留，作废轮已装载未发出的 tag 重开后仍被发出并再次装载（probe
`test_reopen_retransmits_stale_pending`，重开后 8 次提交应为 6）。

修：reopen 时清所有链的 pending；在途 outstanding 保留给完成判据，迟到回调不再
触发 finish_visit（pending 空）。回归：
`test_reopen_drops_stale_pending`（重开后 submit 恰为 6、无同 (meter,tag) 重复）。
反向验证：去掉清理，测试红。

### F3 成功语义文档/测试

代码是 `else if (visit_ok > 0)`：本次访问全部 tag 成功（visit_fail==0）才计该表成功。
此前注释写成“任一 tag 成功即算”，与代码及 000031 的 success==0 判据矛盾。

修：`dcu_poller.c`/`dcu_poller.h` 注释改为“整次访问全部 tag 成功才计”，D0-18 证据
§1.4/§2 同步；新增 `test_mixed_visit_does_not_count_success`（一成功一失败 → success=0，
total=1，status=2），防止后续按“任一成功”改回。

## 2. 文档措辞（A2/F5，既有缺陷不改代码）

- `success_meters <= total_meters` 只在执行内目标集固定时成立；执行中加表或改
  task_mask 可越过（A2）。poller.h 与 D0-18 证据已改。
- “迟到旧回调不计”只对 success 计数成立；遥测写入不保证（F5）。D0-18 证据已改。

## 3. 后续单元候选（本轮不修，含复现）

| 单元 | 缺陷 | 复现 |
| --- | --- | --- |
| A2 | 执行中目标集变化时 `success>total`（6035 线上 total=2/success=3） | `/tmp/irq-a/harness_new` 场景 S5/S6（源码 `/tmp/irq-a/harness.c`） |
| A1 | 同表重叠任务：B 执行在共享访问结束前完成时，B 的成功 tag 不计入（S11） | 同上，S11 |
| F4 | 每访问 pending 上限 `MAX_SCHEME_OADS` 饿死第二任务，但两任务都计 success | `/tmp/irq-b/test_dcu_poller_probe` 的 `test_pending_capacity_starves_second_task` |
| F5 | 删表导致索引移位，迟到回调把 A 的读数写到 B 地址 | probe `test_reopen_late_callback_wrong_address` |

F4 建议：先加“计成功要求该任务位出现在本次 pending_masks”止血；容量方案把每访问
pending 改为“按任务装载、tag 并集 + mask”，或按任务独立 pending 链，单独立项。
F5 建议：回调身份改为 (表地址, OAD) 快照或代际 token，不依赖可复用的 meter_idx。
A1/A2 建议：6035 语义按“执行内目标集快照”定义——begin 时冻结 target 位图，
success 以快照为上限；与 D2-08 dedup 一起设计。

## 4. 验证

- 单测：`ctest -R 'dcu|poller|dlt698|698'` 45/45（新增 3 例，共 14 个 poller 用例）；
  反向验证 F1/F2 新测试在去掉修复后必红。
- F6 所有权回归：`test_master_link_lifecycle` 新增用例修复前红、修复后绿（6/6）；
  同文件既有 5 例不回归；`test_iems_full_stack`/`test_io_executor`/`test_passthrough`/
  `test_bus_orchestrator` 全绿。
- guard：284/284（main 新基线）。
- **验收**：F6 修复前 coverage 构建 000029/000031 反复失败（恢复读全超时、熔断连片）；
  修复后 dcu_acceptance 3/3、dcu_northbound 35/35，安静窗口 3 连绿与 release 构建
  结果见 runtime.log 末尾的验收汇总。

## 5. 新发现（阻塞验收，非本轮 F1/F2/F3 引入）

### F7 表级熔断只开不合（已按状态机修复）

已按 verifier 方案实现 CLOSED/OPEN/HALF_OPEN 显式状态机：冷却 30s，到期放行一次
探测；探测成功 CLOSED、失败回 OPEN 重新计时；601E 规则重配显式 reset。单测覆盖
触发→跳过→探测成功关闭、探测失败重开、reset 关闭；反向验证去掉半开迁移必红。
失败注入用例的熔断累积因此可自愈（冷却后恢复探测）。

### F6 “同一请求被扇出到多条连接”——真因：RX 载荷被双重 unref（已按所有权修复）

早先只看到 bench 侧现象：同一 `frame_hex` 出现在多条通道 socket 上，据此猜“同一逻辑
请求被写多次”。verifier 授权后按机制取证，结论相反：**poller/adapter/arbiter 都只提交
一次、每个通道只写自己表地址的帧**；字节是在“入队 → 真正 write()”之间被覆盖的。

机制证据（本分支插桩运行，工件在 runtime.log）：

1. 提交侧正确：`[API-SUBMIT]` 6438 次 ≈ `[IO-WQ]` 入队 6545 次；15 个 tag × 40 表 ≈ 一
   轮 600 帧，无重复提交。
2. 编码侧正确：`[ENG-TX]` 按 bus 统计，bus 22 只编码表 01–04、bus 31 只编码 37–40，
   每 bus 一个 io 句柄、一个 fd（bus22→fd16 … bus31→fd25），映射无交叉。
3. 写侧异常：`[IO-TXW]`（write() 前的缓冲内容）里每个 fd 都出现了全部 40 个表地址；
   且 2796 次写中 2113 次 `ref=0`——**排队待写的 TX 缓冲已经被放回空闲池**。
4. `net_buf` 追踪（ALLOC/FREE + 调用栈）定位到：TX 分配发生在一次 RX 回调的同步
   调用链里（`_on_uv_poll → _master_io_cb → _process_req_result →
   arbiter_on_io_done → _master_run_command`），拿到的正是该 RX 回调刚释放的那块
   缓冲；随后 io 层在回调返回后又 unref 一次，把这块新 TX 缓冲再次释放。后续任何
   RX/编码分配都会复用并覆盖它，延迟的 write() 于是把别的表/别的通道的帧发出去。
5. 双重 unref 来源：io 句柄对 `IO_EVENT_RX` 载荷“交付后 unref”，而 master/slave
   引擎回调里也 `net_buf_unref(rx)`。两条约定冲突，在“回调内同步派发下一条命令”的
   多通道轮询下必然踩中。

修法（单一所有权，写进 `io_executor.h`）：`IO_EVENT_RX` 的 payload 是**借用**——io
句柄持有到回调返回后自行释放；回调只读，跨回调使用必须自己 `net_buf_ref`。
- `master_engine.c` / `slave_engine.c`：删除回调内对 RX 载荷的 3+3 处 unref。
- `passthrough_mgr.c`：异步 `uv_write` 前 `net_buf_ref`，写完在 `_on_write_done`
  unref（原实现借用缓冲做异步写，本身就是悬垂使用）。
- `tests/test_iems_full_stack.c`：测试直接注入的 RX 缓冲改由注入方 unref（借用语义）。

回归（红→绿，`tests/test_master_link_lifecycle.c` 新增
`test_rx_payload_outlives_engine_callback`）：在引擎回调的 `on_finish` 里从同一池
申请一块并持有为哨兵，回调返回后再申请一块覆盖它。修复前哨兵被 io 层收尾 unref 误
释放 → 断言红；修复后哨兵内容保持、测试绿（连跑 6/6）。

修复后 bench 判据：bus.json 6329 读 → 2624 读（无重发），**同一帧出现在多条通道的
数量 475/576 → 0/600**；000029/000031 及其余 34 个北向用例全绿，dcu_acceptance 3/3。

### 台架记录修复（本 diff 保留）

`dcu_product.py` 的 bus.json 原来每帧全量重写（MB 级），高请求率下饿死台架 asyncio
循环；改为 100ms 合并写（首次写入即落盘）。诊断字段 at/conn 保留。

## 6. 建议

- F7 已修（状态机 + 601E reset）；验证见 tests.log。
- **F6 已收口**：真因是 RX 载荷双重 unref 导致排队 TX 缓冲被提前释放（非请求扇出）。
  修复后同一帧在多通道的计数归零，000029/000031 全绿；验收在安静窗口重跑 3 连。
- F1/F2/F3 + 状态机的单元证据已齐（45/45、反向验证）。

## 7. 重设计：显式状态与单一权威（按 verifier 方向）

### 7.1 任务执行状态机（每任务）

状态：`IDLE(0)` / `ACTIVE(1)` / `DONE(2)`（6035 线上枚举保持 0/1/2）。

| 当前 | 事件 | 迁移 | 动作 |
| --- | --- | --- | --- |
| IDLE | 任务到期且 NORMAL | ACTIVE | begin：按档案+mask 快照 target 位图；total=popcount；清 visited/settled/succeeded/outstanding/sent/recv；start=now |
| ACTIVE | 档案 revision 变化 / task 配置变化 | ACTIVE（重开） | 重新快照 target；作废旧轮：清链 pending；exec_gen++；在途回调不结算新执行 |
| ACTIVE | 所有 target 已 settled 且本任务无在途读 | DONE | end=now；success 已按 target 记账 |
| ACTIVE | 任务被 DISABLE | IDLE | 清 monitor；后续装载/记账被资格谓词拒绝 |
| DONE | 下一次到期 | ACTIVE | 重新 begin（快照） |

资格谓词（load / note_visit / 结算记账 / 完成判定共用）：
`_execution_accepts(t, meter_idx)` = ACTIVE && task NORMAL && target 位图包含该表。
执行代际：`exec_gen` 在 begin/重开时取自全局递增序号；链访问记录装载时的
`visit_exec_seq`；只有 `exec_gen <= visit_exec_seq` 的任务在该访问上结算（A1：任务
后开始时不认领旧访问）。`settled ⊆ target`、`succeeded ⊆ settled` 由快照与单调记账
保证 → `success_meters <= total_meters` 无条件成立（A2 结构性修复）。

成功口径：整次访问全部 tag 成功（visit_fail==0 且 visit_ok>0）才记该 target 成功；
每 target 每执行只记一次。

### 7.2 表级熔断状态机

| 当前 | 事件 | 迁移 | 动作 |
| --- | --- | --- | --- |
| CLOSED | 连续失败访问 ≥ 阈值 | OPEN | since=now；ch_broken++ |
| OPEN | now-since ≥ 30s | HALF_OPEN | 放行一次探测访问 |
| HALF_OPEN | 探测成功（整次访问全成功） | CLOSED | ch_broken-- |
| HALF_OPEN | 探测失败 | OPEN | since=now（重新计时） |
| HALF_OPEN | 探测迟迟未发生（≥30s） | HALF_OPEN | since=now，再次放行一次探测 |
| 任意 | 601E 规则重配成功 | CLOSED | reset_circuit_breaker(ch)（显式迁移） |

实现只保留枚举 + 单调时间戳，不再有 bool 与状态两份真相；访问资格、结果记账、
复位共用同一组迁移函数。
