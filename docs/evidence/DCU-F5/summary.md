# DCU F5：档案删除后迟到回调用错表身份（数据完整性）

分支 `dcu/F5-late-callback-write`，worktree `/tmp/edgec-land/f5-late-callback`，
基线 `563c9f6f`（= 本地 `main` / `feat/dcu` tip）。未合并，等 dcu-verifier 验证。

## 1. 问题与根因

`dcu_inventory_remove` 用末元素覆盖删除位，档案数组下标会移位。一次读的身份
（表地址、OAD/tag、任务位、执行代际、量纲协议、实时发布 slot）此前只在提交时
部分记录在共享的 `dcu_chain_t` 上，回调时刻再按 `c->meter_idx` 反查实时档案：

| 路径 | 旧行为 | 后果 |
| --- | --- | --- |
| `dcu_poller.c::_on_read_done` | `poller->inv->meters[c->meter_idx]` 取 address；`record_telemetry`/`publish_realtime` 传 `meter_idx` | A 的迟到读数写到顶替其下标的 B 的地址；实时发布拿到 B 的 slot |
| `dcu_poller.c::_chain_finish_visit` | 用 `inv->meters[c->meter_idx].address` 更新熔断与通道统计 | A 的访问结果记到 B 的熔断状态 |
| `dcu_bus_source.c::_on_done` | 完成时用 `bs->inv->meters[meter_idx].protocol` 换算 `scale`；备用 DI 重试也用当前下标取 protocol/channel | 量纲按 B 的协议换算；重试可能读到 B |
| `dcu_node.c::_node_publish_realtime` | 发布时用 `meter_idx` 反查 `slot_id` | 发布会话拿到 B 的 device_id |

删除时取消在途请求不可行：`edge_api` 无 cancel，迟到回调是合法状态。

## 2. 数据形状（先设计后补丁）

**提交时快照，回调只认快照。**

- `dcu_poller.h` 新增 `dcu_read_ctx_t`：`address`、`tag_id`、`task_mask`、
  `device_id`（`slot_id`，稳定设备身份）、`exec_seq`。每个通道链内嵌一份
  `read` 快照；在途读固定最多 1（第二轮删除可变 `depth`，见 §7）。
- 每次访问在装载时冻结 `visit_address`；熔断/通道统计只用它。
- 档案变更重开执行时把 `visit_exec_seq` 置 0：迟到回调仍按自己的快照落库到
  原表（不丢合法数据），但不再计入任何一次新访问。
- `dcu_bus_source.c` 的 `bus_ctx_t` 快照 `protocol`（量纲）与提交时的
  `revision`（第二轮取代 `address`/`channel_id`/`slot_id`，见 §7）。完成时的
  `scale` 用快照 `protocol`；备用 DI 重试只在提交时的 revision 仍成立时进行。
- `poller_bus_ops_t.publish_realtime` / `poller_sink_ops_t.record_telemetry` 的
  `uint32_t` 参数语义改为稳定的 `device_id`（`slot_id`），与
  `dcu_node_feed_meter_reading` 既有的 slot 语义一致。`dcu_node` 拆成两个发布
  适配：路由端口仍按档案下标解析，poller 端口直接使用快照 slot。

### 拒绝的替代

- **删除时取消在途请求**：`edge_api` 无 cancel；任务书明确禁止用取消绕过。
- **只加代际 token、直接丢弃迟到回调**：能避免写错表，但会丢掉 A 的合法读数；
  任务要求迟到回调仍记到 A 的地址，且无法修 bus source 的量纲。
- **回调时按地址反查当前档案**：仍是反查可变档案；表被删后查不到，无法发布
  A 的 slot，也无法用提交时协议换算。
- **每次提交动态分配上下文**：`check_no_dynamic_memory.py` 与回调路径禁止；
  链式模型固定单在途读，每链一份内嵌快照是最小正确形状。要让通道流水需要把
  `read` 扩成与并发度同宽的槽位数组，本单不扩大范围。

## 3. 改动文件

| 文件 | 改动 |
| --- | --- |
| `cmd/dcu/dcu_poller.h` | 新增 `dcu_read_ctx_t`；链字段 `visit_address`、`read`；删除孤儿 `inflight_tag/inflight_mask`；端口参数语义改为 `device_id`；第二轮删除 `depth` 并按冻结时机拆分注释（§7） |
| `cmd/dcu/dcu_poller.c` | 装载/提交时快照读身份；回调只用快照；访问快照用于熔断/统计；重开作废 `visit_exec_seq`；第二轮在 pump 入口加 revision 栅栏（§7） |
| `cmd/dcu/dcu_bus_source.c` | `bus_ctx_t` 快照 `protocol`；scale 用提交时 protocol；第二轮改为快照 `revision` 并删除 `address`/`channel_id`/`slot_id`（§7） |
| `cmd/dcu/dcu_node.c` | poller 发布适配使用快照 slot；路由侧保留下标解析适配 |
| `tests/test_dcu_poller.c` | 3 个红先绿后回归；mock 记录地址/tag/device_id；`--wrap` 捕获总线提交与备用 DI 决策 |
| `tests/CMakeLists.txt` | 仅给 `test_dcu_poller` 增加两行 `--wrap` 链接选项（无组合根/规则库时驱动 bus source 完成路径） |

## 4. 回归测试（红先绿后）

提交顺序（第二轮见 §7）：`a695fd09`（测试）→ `800e6eba`（修复）→
`8b2b827c`（第二轮测试）→ `f26118c6`（第二轮修复）。

- `test_reopen_late_callback_wrong_address`：A 在途时删除 A，B 顶到下标 0。
  迟到回调必须落 `record_telemetry("A0000001", device_id=5, tag)`、
  发布 `device_id=5`；新执行 `success_meters=0`、`settled_left=1`，随后 B 正常
  抄完计 1，后续记录全部是 B 的地址/slot 6。
  （slot 与下标刻意错开：`inv.next_slot=5`，否则 0 与 0 无法区分。）
- `test_late_callback_uses_submit_protocol_for_scale`：A=DLT645 在途，删除后
  下标 0 是 B=DLT698；完成值 1000.0 必须按 A 的 `scale=0.001` 得 1.0，旧码
  按 B 反查得到无规则、原样 1000.0。
- `test_late_failure_does_not_retry_shifted_index`：A 的迟到失败且规则要求备用
  DI 重试时，身份已不在原下标，不得再向移位后的下标发读（`--wrap` 计数保持 1）。

## 5. 完成定义逐项

- [x] 静态分析（第二轮修复后重跑）：`check_build_warnings.py`（全量构建日志 0 新增）、
      `check_strncpy_bounds.py`、`check_no_dynamic_memory.py`、`check_alloc_null.py`、
      `check_plane_isolation.py`、`check_component_boundary.py`、
      `check_composition_root.py`、`check_caller_allocated_storage.py`、
      `check_public_structs.py` 全过。
- [x] 测试：第二轮修复前 3 红 1 绿（正例），修复后 `test_dcu_poller` 27/27 绿；
      `ctest -R 'dcu|dlt698'` 45/45 绿（原始输出见 `tests.log` / `runtime.log`）。
- [x] 运行时证据：红/绿二进制原始输出见 `runtime.log`。
- [x] 现有断言未放宽、未加 skip、未删失败用例；第二轮未修改既有断言。
- [x] exec_gen / settled_left / target 快照语义不变（既有 23 个用例全绿）。
- [ ] ASAN/UBSAN：本单未跑（时间盒内仅 Release 构建；建议 verifier 在 CI matrix 覆盖）。
- [ ] 验收台架：按任务书由 verifier 串行执行，本单不占用台架。

## 6. 影响面与未决

- 端口语义变化（`poller_bus_ops_t` / `poller_sink_ops_t` 的 `uint32_t` 参数）：
  实现只有 `cmd/dcu/dcu_node.c`；其余 mock 均忽略该参数。`record_telemetry` 的
  `device_id` 现在写 `slot_id`（与 `dcu_node_feed_meter_reading` 一致），
  `dcu_telemetry_latest` 按地址键控，不依赖该列。
- 观察到的既有不一致（不在本单修改）：`dcu_event_monitor.c` 把消息里的
  `device_id` 当档案下标使用，而 shadow/publish 路径写的是 `slot_id`；
  `dcu_node_record_reading` 的 `device_id` 也是下标。slot 与下标在删除后才会
  分叉，建议另立 finding 评估；本单的快照修复不依赖它们。
- 时间盒内未跑 ASAN/TSAN 与台架验收，见上。

## 7. 第二轮（对抗评审后）：一个 revision 一次访问

### 7.1 新增发现（均已复现）

| 发现 | 复现 | 旧行为 |
| --- | --- | --- |
| 迟到回调先于 step 到达时，pump 继续提交旧访问的剩余项 | adv-2 `test_probe_late_callback_before_revision_observed` | A 被删、B 顶到下标 0：第二项按 B 的 DLT698 协议读出并缩放，却以 A0000001/dev=5 落库并发布（reads=2） |
| 表只移位未删 | adv-1 `probe_pending_submit_out_of_range_before_step`（`/tmp/verify-f5/adv1-run.log`） | C 被删使 A 从下标 1 移到 0：旧下标 1 越界，一帧未发（`[SRC-ERR] idx-out-of-range`），却给 A 记一次访问失败、error_count+1、breaker fails=1 |
| slot 耗尽回收后备用 DI 重试 | adv-2 `test_probe_spare_retry_slot_reuse` | 新表 C 继承 slot 1 并落在下标 1，旧守卫 `by_slot(slot)==&meters[idx]` 成立，重试读到 C（reads=2） |

### 7.2 数据形状（先设计后补丁）

**一次访问属于一个档案 revision；一次读和它的重试属于同一个 revision。**

- `_dcu_chain_pump` 入口：`poller->inv && poller->inv->revision != poller->last_inv_rev`
  直接返回。revision 变了而 step 尚未重开时，链不得再提交本轮任何项；迟到回调
  自己的结果仍按快照落库。下一次 step 的 `_poller_reopen_active` 清 pending、
  重开、重抄。
- `bus_ctx_t` 在提交时快照 `revision`；`_on_done` 只在
  `bs->inv && c->revision == bs->inv->revision` 时允许备用 DI 重试。revision
  不变则下标不可能移位，原下标仍是同一只表；channel 从该原下标当时的档案读取。
- 删除 `bus_ctx_t.slot_id` / `address` / `channel_id`：revision 判定后不再需要。
  保留 `protocol` 用于 scale、`proto_id`/`tag_id`/`task_mask` 用于重试与回调。
- 删除 `dcu_chain_t.depth`：init 固定 1、pump 仍读它，单份 `read` 快照只在单在途
  读下成立。pump 固定"最多 1 个在途读"，"单快照 + 多在途"不可表达。
- 注释按实际冻结时机拆分：`dcu_read_ctx_t.address/device_id` 装载时冻结，
  `tag_id/task_mask/exec_seq` 每项提交时写入；`read` 字段注释同口径。

### 拒绝的替代

- **pump 里只加越界检查（`meter_idx >= inv->count` 才停）**：能挡住"移位未删"的
  假失败，但挡不住同一下标被 B 顶替（A 被删场景），仍会写错表。
- **回调时按地址反查档案再决定重试**：地址可被 6000 方法 129 改写，且"表还在但
  改名"与"表已删"无法用地址区分；revision 是档案整体形状的身份。
- **把 depth 设为 0/禁用而不是删除**：非法状态仍可表达，下一个人仍会读它。

### 7.3 改动文件（第二轮）

| 文件 | 改动 |
| --- | --- |
| `cmd/dcu/dcu_poller.c` | pump 入口 revision 栅栏；删除 depth 读取与初始化 |
| `cmd/dcu/dcu_poller.h` | 删除 `dcu_chain_t.depth`；拆分读快照注释 |
| `cmd/dcu/dcu_bus_source.c` | `bus_ctx_t` 用 revision 取代 slot_id/address/channel_id；重试守卫按 revision |
| `tests/test_dcu_poller.c` | 4 个新用例（3 负 1 正）；`--wrap` 增加 bus_id 捕获 |

### 7.4 回归（红先绿后）

提交：`8b2b827c`（测试）→ `f26118c6`（修复）。

- `test_late_callback_before_revision_observed`（负）：修复前 reads=2（断言 2 != 1），
  修复后 reads=1；迟到项仍落 A0000001/dev=5。
- `test_late_callback_shift_does_not_fail_meter`（负，移位未删）：修复前
  `[SRC-ERR] reason=idx-out-of-range`，A 的 `consecutive_fails` 1 != 0；
  修复后 reads/error_count/visit_fail/consecutive_fails 均不增，step 重开后
  新执行把 A 的两个 tag 抄完、success_meters=1。
- `test_spare_retry_dropped_after_slot_reuse`（负）：修复前 `[SRC-ERR] spare-retry`
  reads=2（断言 2 != 1），修复后 reads=1，回调收到 PROTO_ERROR。
- `test_spare_retry_allowed_when_revision_unchanged`（正）：两轮均绿；
  重试 reads=2，沿用原 channel=2/protocol=DLT698/index=0，spare DI=0xDEAD，
  第一次失败不向调用方回调。证明重试没有被关掉。

修复前 3 红 1 绿原文见 `runtime.log` 的 ROUND 2 RED，修复后 27/27 见
`runtime.log` 的 ROUND 2 GREEN 与 `tests.log`。

### 7.5 未决

- adv-1/adv-2 探针所在的 `/tmp/interrogate-f5/` 是评审工作区，本单未修改；
  两个负例已移植为产品回归。
- ASAN/UBSAN 与台架仍由 verifier 覆盖（同 §5）。
