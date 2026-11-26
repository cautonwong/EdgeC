# FORMAL-P1 残余、bounds 与 assumptions

## 1. 证明范围

| 常量 | 值 | 含义 |
| --- | --- | --- |
| `Cmds` | `{1,2}` | 两条命令；足以覆盖“上一条超时、下一条派发、迟到应答到达”的串包路径 |
| `Priorities` | `{0,1,2}` | 有效优先级；模型按最高优先级、最早 deadline 选队首 |
| `Timeout` | `2` | `cmd->timeout_ms`，发送时刻起算 |
| `LateBound` | `1` | 超时结算后允许迟到应答到达的时钟上界 |
| `MaxQueue` | `2` | `max_queue_len` |
| `MaxTime` | `6` | 模型时钟上界；保证 TLC 有限状态 |
| `RecoveryGate` | `TRUE` | 正例；负例只翻成 `FALSE` |

模型内 `ASSUME LateBound < Timeout`。正例结论：在上述常量下，四条不变量对所有可达状态成立。TLC 正例 84,083 states generated / 30,538 distinct，depth 15。

## 2. 结论边界（不偷放宽不变量）

- 模型声明的迟到上界是“超时结算后、小于一个 `timeout_ms` 的恢复窗口内到达”。在这个窗口内，下一次派发被 `recovery_until_ns` 挡住；迟到字节到达时 `current_cmd = 0`，随后的 `DISCARD_RX` 把它丢掉。
- **超出“超时 + 窗口”的应答仍会串包**：从原帧发出算起，总延迟达到 `timeout_ms + timeout_ms`（超时 + 恢复窗口）时，恢复窗口已经关闭，下一帧可能已在线上；只要应答在下一帧发出后才到，RTU 读应答没有寄存器地址可校验，`_master_io_cb` 会把它当 `current_cmd` 的应答。即使只略微越过窗口边界（而不是等满两个 `timeout_ms`）也可能串包。模型没有把 `LateBound` 放宽到 `>= Timeout` 来“证明”它安全；这条残余是设计边界，不是模型缺陷。
- 即使应答在窗口内，真实系统里 `DISCARD_RX` 与字节到达的同一时刻竞态也不在模型内。模型用离散 tick 把“窗口内到达”与“派发”分开；真实连续时间的边界行为落到上一条残余。同理，`now = deadline` 时模型只允许超时结算、不允许正常应答；真实事件循环里两者是竞态，模型取了保守的一侧。

## 3. 模型抽象

- 单总线、单 owner、单 `current_cmd`。多总线之间不共享 owner 或队列；模型不覆盖总线间交错。
- 未建模且不能派发或结算命令的路径：`arbiter_pause`/`arbiter_resume`、`min_gap_us` 帧间静默、`is_exclusive`、队列满拒绝。它们只会延迟派发或拒绝提交，不会产生第二个 owner，也不会跳过结算。`pause`/`min_gap` 延迟派发只会让恢复窗口相对更长（安全方向）。
- 队列满时的 EMERGENCY priority-drop 会结算被剔除的队尾命令（`arbiter.c:261-269`）。该命令不在 owner 上，不产生双结算；但这条结算路径未入模型，属于残余覆盖缺口。
- 所有命令共用一个 `Timeout`；真实代码允许 per-command `timeout_ms`，恢复窗口取被超时命令自己的 `timeout_ms`。模型取等值。
- 引擎的 setup/encode/write_async 失败用 `EngineError` 原子结算；未建模写失败与定时器回调之间的真实时序，因为它们不改变 owner/窗口语义。
- 多帧事务（`REQ_STATUS_CONTINUE`）不建模。真实代码在续帧时重挂 `current_cmd` 并续展引擎定时器，而 arbiter 的 `deadline_ns` 仍是首次发送时的值；这条时序不在模型内。四条不变量只依赖“结算前先摘 owner/current”的所有权检查，这一点在续帧路径上仍然成立，但“arbiter 扫描在续帧中途结算”的交错没有 TLC 证据。
- `engineSent` 在 `Dispatch` 时置位；模型没有把 setup/encode 失败和真正 `write_async` 拆开，因此对“未真正上线”的命令也记为已发送。没有不变量依赖 `engineSent`，它只用于覆盖“引擎已发送/已读取”。
- `_master_on_timeout` / `_process_req_result` 的 `!arbiter_owns_cmd` 陈旧分支不结算，只清本地状态；模型由原子结算（先清 owner/current）使其不可达，因此不单独建模。

## 4. `NoDoubleSettle` 的建模假设

- 模型里每个结算动作先清 owner/current，再增加 `settleCount`，所以第二条路径的守卫自然失败。对应代码：`arbiter_on_io_done` 在锁内先 `if (owner == msg) owner = NULL`（`arbiter.c:319-322`）；`_master_on_timeout` 和 `_process_req_result` 在回调 arbiter 前先 `arbiter_owns_cmd`（`master_engine.c:69`、`:104`）。
- `arbiter_check_timeouts` 在锁内清 owner、写窗口并调 `_dispatch_next_locked`，锁外才由 `_on_arbiter_settled` 清引擎的 `current_cmd`。模型只把“清 owner”和“清 current_cmd”合并为 `ArbiterTimeout` 一个原子动作；派发是独立的 `Dispatch` 动作，仍受恢复窗口守卫。uv 是单线程事件循环，RX/定时器回调不能插进这两步之间；若将来 arbiter 回调跨线程，这个假设失效。

## 5. 未做

- 活性/公平性：没有 `PROPERTY`，不证明命令最终结算、队列最终排空、超时最终触发。
- App 生命周期、Registry 收敛、IEC104 会话接管、WAL 崩溃恢复：Phase 5 扩展，未纳入。
- `arbiter_destroy` / `_master_detach` 的关停路径未建模；模型只覆盖运行中的事务边界。
- C 精化：TLA+ 是设计层安全模型，靠函数名/行号锚点人工对应，没有提取或 refinement 证明。负例是模型常量变异，不是产品 C 的 mutation。
- 未发现产品代码违反四条不变量，因此没有 finding，也没有为过门禁改产品代码。
