# DCU F4 round 3 注释变更记录

依据 AGENTS.md「注释删除范围：只删当前 diff 新增的注释；删除既有 why 注释须在证据里逐条给理由」。
逐条列出本轮删除或改写的注释、原文与理由。

## 删除的既有注释

1. `cmd/dcu/dcu_poller.h`，`dcu_meter_poll_state_t` 内（引入于 `834a4582`）：

   ```
   /* 注：S3 曾在这里放 done_mask/done_step 做"同一步去重"。链式模型下不需要——
    * 去重由链上的 served 位图与 pending 掩码完成：一批内的 tag 取并集，
    * 并集超容量时同一块表连续分多批，跨批共享 tag 只提交一次。 */
   ```

   理由：前半是已删除机制（S3/S4 的 done_mask/done_step）的历史旁白，不解释当前代码；
   后半的去重 why 已经在仍然存在的 `dcu_chain_t.served` / `pending_tags` 字段注释和
   `_chain_load_pending` 里，删除不丢信息。

2. `tests/test_dcu_poller.c`，`_run_new_exec_mid_batch` 内（本轮新增，来自 `5cd6c390`）：

   - `/* 此刻第二批（任务 2 的 32 项）已装载并发出首项；任务 1 仍在执行中。 */`
   - `/* 任务 1 在分批中途开新执行。 */`

   理由：两行都只是复述紧接其后的断言/调用，删掉后断言本身可读；场景 why 保留在
   该 helper 顶部的「前一批的失败属于旧执行，新执行 32 项全部成功必须记成功」。
   这两条不是 main 上的既有注释，仍按同标准记录。

3. `tests/test_dcu_poller.c`，`test_capacity_batch_failure_fails_whole_meter` 内
   （本轮新增，来自 `f26fd3e8`）：

   - `/* 失败确实发生在任务 2 的批次 */`

   理由：复述下一行 `assert_int_equal(failed_task2_first, 1)`；变量名已表达同一事实。

   保留的真正 why：`/* 与任务 1 共享，使并集 63 > 32 */`、`/* 链上在途最多 1：先只发出一项 */`、
   `/* 第一批：任务 1 的 32 项；首项按参数决定成败。只消耗这一批。 */`、
   `/* 重开后的新执行里，方案 3 的每个 tag 恰好提交一帧且都带任务 2 位。 */`，
   以及 helper 顶部的 taint 边界说明（旧执行的失败不得污染新执行）。

## 改写的注释

4. `cmd/dcu/dcu_poller.c`，`dcu_poller_upsert_task` 函数体（`815ab262` 引入）：

   原文：`执行身份（方案/项掩码）变化则重开 ACTIVE 执行，与 add_task_ex / 档案 revision 变化同语义。`

   改写：只重开被改任务自己的 ACTIVE 执行，并写明 upsert 是完整任务单元替换、
   `item_mask` 一并同步、6012 路径恒传 0（会清掉既有 mask）。

   理由：原文为「重开全部 ACTIVE 任务」的过宽行为背书，与真实变化范围不符。

5. `cmd/dcu/dcu_poller.h`，`dcu_poller_upsert_task` 文档注释：同一理由，改为
   「只重开被改任务自己的 ACTIVE 执行」。

6. `cmd/dcu/dcu_poller.h`，`dcu_exec_ident_t` 文档（`815ab262` 引入）：

   原文：`任一分量变化（6012 改方案/item_mask，或 begin/重开递增的 exec_gen）...`

   改写：`exec_gen` 在 begin/重开时递增；`scheme_id/item_mask` 由任务 upsert 写入；
   6012 只改 `scheme_id`、`item_mask` 恒写 0（`dcu_task_config_t` 无该字段）；
   直接 C API 原地改方案内容不改变身份，记为 HOLD。

   理由：6012 不清 `item_mask`（`_ram_add` 清零后不填，store 传 0），原文会让人以为
   6012 能表达「按位选列」，与真实边界相反。

7. `cmd/dcu/dcu_poller.c`，`_task_reopen` 头注释（`815ab262` 引入）：

   原文：`...链上 pending 由调用方清`。

   改写：链上 pending 不在这里清；只重开单个任务时由 `served_ident` 按身份清 served，
   旧批次失败/结算由代际挡掉。

   理由：新的 upsert 调用方不再清整条链，原文会误导调用方去清 pending。

8. `cmd/dcu/dcu_poller.c`，`_chain_finish_visit` 头注释：

   原文：`/* 一批访问的结果：按结果更新表级熔断与通道统计 ... */`

   改写：整表全部分批结束后结算一次，并写明「任一批失败即整表失败；无失败且至少
   一次成功才记访问成功」。

   理由：函数的调用时点与聚合口径都是本轮改动的一部分，原注释描述的是已删除的
   逐批口径。

9. `cmd/dcu/dcu_poller.c`，`_chain_settle_visited` 头注释：

   原文：`exec_gen 早于旧访问的不结算新执行；...`

   改写：`装载点代际早于任务当前执行代际的访问不结算该任务；...`

   理由：原文把 `mon->exec_gen > c->visit_exec_seq` 的主语说反了。该条件为真表示
   任务当前执行比装载点新，旧的本次访问不能结算新执行。

10. `cmd/dcu/dcu_poller.c`，`_chain_load_pending`：新增一行 why，说明
    `visit_ok/visit_fail` 跨本表所有批次累计，由 `_chain_next_meter` 在新表访问
    开始时清零（解释计数器为什么不在本函数清零）。
