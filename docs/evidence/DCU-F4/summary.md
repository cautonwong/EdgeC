# DCU F4：单表任务并集超容量时静默丢 tag

分支 `dcu/F4-pending-capacity`，合并 tip `c6dcbccd`（feat/dcu merge `253ed19d`），基线 `feat/dcu 383c91cc`。未 push。

## 1. 问题与根因

`dcu_poller` 每次访问把一块表上所有合格任务的可抄项并集装进 `pending_tags[MAX_SCHEME_OADS=32]`，满了就静默丢弃后面的项；任务仍被标记 visited，按失败结算，执行照常完成。两个任务各 32 tag 同表时，第二任务的点永远不抄且无告警。

对抗评审在容量分批的第一版上继续复现出两个执行边界缺陷：

- 分批中途用 6012 Add 换任务方案时，`served` 位图仍按旧方案的按位下标解释：新方案 16 个 tag 静默漏抄、一个 tag 重复读、表被记假成功。
- `meter_seq_fail` 是链级全局：一个被作废旧执行的失败会污染同表新执行，健康表被记失败。

## 2. 数据形状

- 一次表访问 = 若干个容量内批次；每任务 `served` 位图记录本执行已装载的方案项位，`served_ident` 按执行身份清零。
- 执行身份 `dcu_exec_ident_t { exec_gen, scheme_id, item_mask }` 同时界定 `served` 与 `fail_ident`；旧执行的失败不结算新执行。
- `dcu_poller_upsert_task` 在 scheme_id/item_mask 变化时只对 `_task_reopen` 被改任务，不再重开全部 ACTIVE。
- 熔断与通道成功统计在整表全部分批结束后结算一次：任一批失败即整表失败；无失败且至少一次成功才记访问成功。`poll_count`/`error_count` 仍逐项计数。
- 保持：每任务每 CSD tag 每执行恰好一次、不重复；`visited` 只在项全部装载或按失败结算后置位；target 快照、exec_gen、settled_left、success<=total 与 F5 revision 栅栏不变。

拒绝的替代：扩容 pending 到最坏 512（超预算且 `pending_n/i` 为 uint8_t）；配置入口拒绝（会拒绝总线本来能串行完成的配置）；按批结算熔断（混合批次永远到不了阈值）；全量重开（无关任务被迫重抄）。

## 3. 改动与提交

- 测试：`fe8b8035`（容量分批 4 例）、`f26fd3e8`（整表失败聚合）、`5cd6c390`（方案切换/跨代失败）、`ca3b5582`（按任务重开/按访问熔断）。
- 修复：`8c7f0ec7`（容量分批）、`815ab262`（执行身份）、`edbb7a3f`（只重开被改任务、整表熔断一次）。
- 文件：`cmd/dcu/dcu_poller.c/.h`、`tests/test_dcu_poller.c`、`docs/evidence/DCU-F4/`。
- `test_dcu_poller` 34 例；新增 8 例覆盖容量饿死、跨批共享、跨 step 分批、任一批失败整表失败、方案切换只重开该任务、无关任务 exec_gen 不变、跨代失败不污染、混合批次熔断。

## 4. 验证

- 红先绿后独立复跑：`5cd6c390` 2 红/31 绿 → `ac322a1b` 33/33；`ca3b5582` 2 红/32 绿 → `c6dcbccd` 34/34；`ctest -R 'dcu|dlt698'` 45/45。
- coverage 验收：run1 台架未完成（归因窗口），run2/3/4 连续 3 轮 2/2、35 passed；ASAN 34/34；合并后守卫全绿、全量 ctest 367/367、fanalyzer 3 known/0 new、warning 0。原始日志在 `verifier/`。

## 5. 未决

- HOLD：`dcu_poller_upsert_scheme` 直接原地改被引用方案的 CSD 不改变执行身份，会重新引入 `served` 误读；当前 6014 store 拒绝该路径，根除需要方案内容 revision 或 upsert_scheme 自行重开。
- 观察：`dcu_poller_add_task_ex` 仍重开全部 ACTIVE，非正确性缺陷，留给后续 ticket。
