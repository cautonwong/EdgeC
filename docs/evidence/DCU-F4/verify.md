# DCU F4 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/F4-pending-capacity`，合并 tip `c6dcbccd`（feat/dcu merge `253ed19d`），基线 `feat/dcu 383c91cc`。

## 结论

VERIFIED。单表任务并集超容量时改为容量内分批，每个任务的每个 CSD tag 在一次执行中提交恰好一次。执行身份 `{exec_gen, scheme_id, item_mask}` 同时界定 served 与失败记录；6012 换方案只重开被改任务；熔断与通道成功统计在整表全部分批结束后结算一次。两轮对抗评审的可复现发现（方案切换 served 误读、跨代失败污染）已由红先绿后产品回归钉住。

## 红先绿后（独立复跑）

- 测试 commit `5cd6c390`：33 例中 2 红（scheme 切换重开、跨代失败不污染），31 绿。原文 `verifier/round2-red.log`。
- 修复 tip `ac322a1b`：同一命令 33/33 绿；`ctest -R 'dcu|dlt698'` 45/45。原文 `verifier/round2-green.log`、`round2-broad.log`。
- 测试 commit `ca3b5582`：34 例中 2 红（无关任务 exec_gen 被全量重开、混合批次熔断到不了阈值），32 绿。原文 `verifier/round3-red.log`。
- 修复 tip `c6dcbccd`：同一命令 34/34 绿；45/45。原文 `verifier/round3-green.log`、`round3-broad.log`。

## 验收（COVERAGE=ON，安静窗口）

- run1：dcu-northbound 退出码 3（未完成），27 passed，8 skipped，7.72s；无产品断言失败。归因为 rebuild 后首轮台架/夹具窗口，不是产品失败。原文 `verifier/round3-accept-1.log`。
- run2/3/4：连续 3 轮 2/2 任务，35 passed，27.41/22.77/22.90s。原文 `verifier/round3-accept-{2,3,4}.log`。

## 运行时与门禁（合并树 `feat/dcu` `253ed19d`）

- ASAN（Debug+ASAN，`detect_leaks=1`）`test_dcu_poller` 34/34，无 sanitizer 报告。原文 `verifier/round3-asan-run.log`。
- `run_guard_selftest.py` 全绿（exit 0）。
- 全量 ctest 367/367，79.66s（排除 `app_platform_e2e`），原文 `verifier/round3-merged-ctest.log`。
- `check_build_warnings.py` 0 新增，`run_fanalyzer.py` 3 known / 0 new。原文 `verifier/round3-warnings.log`、`round3-fanalyzer.log`。

## 已知限制

- HOLD：`dcu_poller_upsert_scheme` 直接原地改被引用方案的 CSD 不会改变执行身份，会重新引入 served 误读。当前 6014 store 拒绝该路径；根除需要方案内容 revision 或 upsert_scheme 自行重开引用任务。
- 观察：`dcu_poller_add_task_ex` 新增任务仍重开全部 ACTIVE 执行，与“只重开变化方”的形状不一致，非正确性缺陷，留给后续 ticket。
- 本机 TSAN 仍受 `vm.mmap_rnd_bits=32` 限制，依赖 CI matrix。
