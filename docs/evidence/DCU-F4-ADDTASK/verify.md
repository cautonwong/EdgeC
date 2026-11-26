# DCU F4-ADDTASK 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/F4-addtask-targeted`，merge `77dd8e1e`（tip `10fd9c59`），基线 `feat/dcu a3421169`。

## 结论

VERIFIED。`dcu_poller_add_task_ex` 不再重开已 ACTIVE 任务：新增任务只追加槽位，旧任务的 exec_gen、visited/settled 与在途批次保留；新任务按自己的 due 首次 `_task_begin`。回调级链式 fence（`served_ident`/`exec_gen`）仍在，跨代旧位不会串用。

## 红先绿后（独立复跑）

- 红：测试 commit `acafecf4` 在修复前 `test_add_task_does_not_reopen_active_execution` 失败 `2 != 1`（exec_gen 被递增），36 例中 1 红。
- 绿：修复 tip `93877f9e` 同一用例转绿，`test_dcu_poller` 36/36、`ctest -R 'dcu|dlt698'` 45/45。

## 验收（worktree `build`，Release）

3 连绿，2/2 任务：`dcu-acceptance` 3 passed，`dcu-northbound` 35 passed；时长 10.40/31.03、10.87/22.85、8.20/17.45s。原文 `verifier/accept-{1,2,3}.log`。

## 合并树门禁（`77dd8e1e`）

- 合并树 build 0 第一方 warning，原文 `verifier/merged-build.log`、`verifier/warnings.log`。
- 守卫自检全绿，原文 `verifier/merged-guards.log`。
- 全量 ctest：367 例中 `test_edge_app_mgr` 的 `test_async_install_loop_lag_and_busy` 在负载 24 下报 `timing_ok is not true`（18.73s）；单独复跑 8.50s 通过。归因为主机负载（同窗口另一团队 iems ASAN 构建在跑），非本改动回归；安静窗口再补 367/367 原文。
- 该用例与本改动无文件交集（`modules/edge_core` app 管理），失败断言是墙钟时序。

## 未决项

- 无功能未决；计划/交接文字由本记录承接。
