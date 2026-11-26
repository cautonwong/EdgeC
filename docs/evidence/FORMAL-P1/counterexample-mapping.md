# 反例到代码步骤的映射

两条反例来自同一模型，唯一差别是检查的不变量集合。状态编号来自 TLC 的 “The behavior up to this point is”。

TLC 可能选对称变体：仓库负例本次跑出的是“先派发 priority=1 的命令 2，命令 2 超时后派发命令 1”，深度同为 8；下表用串包 cfg 的命令 1/2 trace 说明，机制同构。`tla-negative-output.txt` 保留实际跑出的两版 trace。

## 负例 1：`ArbiterNoRecovery.cfg`（RecoveryGate = FALSE，8 步）

| TLC 状态 | 模型动作 | 代码步骤 |
| --- | --- | --- |
| 1 | `Init` | `_arbiter_setup` 之后的空闲总线：`owner = 0`，`recovery_until_ns = 0` |
| 2 | `Submit(1,0)` | `arbiter_submit_cmd`（`arbiter.c:234`）：准入、`deadline_ns = submit_ts + timeout_ms`、按优先级/EDF 入队 |
| 3 | `Submit(2,0)` | 同上；两条命令同优先级、同 deadline，队列为 `{1,2}` |
| 4 | `Dispatch` | `_dispatch_next_locked`（`arbiter.c:170`）把队首 1 设为 owner；`_master_run_command`（`master_engine.c:237`）编码、`deadline_ns = now + timeout_ms`、置 `current_cmd = 1`、`write_async`；模型记 `engineSent = {1}` |
| 5–6 | `Tick` ×2 | 时间推进到 `now = 2 = deadline[1]` |
| 7 | `ArbiterTimeout(1)` | `arbiter_check_timeouts`（`arbiter.c:377`）：owner 到期，摘 owner、`last_tx_end_ns = now`、`recovery_until_ns = now + timeout_ms = 4`（`arbiter.c:403-409`），然后在锁内调 `_dispatch_next_locked`；锁外 `_on_arbiter_settled` 清 `current_cmd`、FLUSH、停读、`DISCARD_RX`、on_finish(TIMEOUT) |
| 8 | `Dispatch`（违例） | 坏实现跳过 `arbiter.c:185-191` 的 `now < recovery_until_ns` 守卫，在 `now = 2 < 4` 时把命令 2 设为 owner；`_master_run_command` 发第二帧。真实代码在这里会等到 `now >= 4` 才派发。最短反例在此终止，违反 `NoDispatchInRecovery` |

## 负例 2：串包临时 cfg（再删掉 `NoDispatchInRecovery`，9 步）

前 8 步同上。第 9 步：

| TLC 状态 | 模型动作 | 代码步骤 |
| --- | --- | --- |
| 9 | `LateResponseArrives(1)`（违例） | 命令 1 的迟到应答在 `now = 2 <= timeoutAt[1] + LateBound = 3` 到达。此时 `current_cmd = 2`，`_master_io_cb`（`master_engine.c:180`）把字节追加到 `rx_acc` 并用命令 2 的 `decode` 解码；`_process_req_result`（`master_engine.c:101`）会把它当命令 2 的成功应答。模型记 `assignedTo[1] = 2`、`engineRead = {2}`，违反 `LateResponseNotAssignedToNext`。RTU 读应答没有寄存器地址可校验（`arbiter.c:181` 注释），这正是串包无法在解码层区分的原因 |

## 安全路径（RecoveryGate = TRUE）为什么成立

- 超时结算把 `recovery_until_ns` 写成 `now + timeout_ms`；派发守卫要求 `now >= recovery_until_ns`。
- 模型的迟到到达窗口是 `now <= timeoutAt + LateBound` 且 `ASSUME LateBound < Timeout`，所以任何被建模的迟到应答到达时 `engineCurrent = 0`；随后下一次 `_master_run_command` 的 `DISCARD_RX`（`master_engine.c:218`）把它丢掉，`assignedTo` 保持 0。
- 引擎超时路径（`_master_on_timeout`，`master_engine.c:65`）和 arbiter 扫描路径写同一个窗口；两条路径都先摘 owner/current 再结算，`NoDoubleSettle` 由抢占式守卫保证。

## 建模注意

`LateResponseArrives` 建模的是“字节被交给当前命令的 decoder”这一最坏情况，不区分 decoder 返回 incomplete 还是完整帧。因此反例说明的是串包风险，不是“CPU 已经接受了错误值”的完整数据流。
