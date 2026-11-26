# DCU 变异守护 F4/F5 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/mutation-guards-invariants`，合并 tip `c8bf2eff`（feat/dcu merge `caaf28be`），基线 `feat/dcu 197d98b9`。

## 结论

VERIFIED。四条 F4/F5 不变量测试已登记进 `ci/mutation_guards.tsv`。每条变异使目标测试红，恢复后绿；runner selftest 通过；产品源码零改动。

## 四条目

- `DCU-F4-batch-continuation`：`_chain_begin_visit` 续批被禁用 → `test_pending_capacity_starves_second_task` 红（32 != 64）。
- `DCU-F4-served-ident`：`served_ident` 比较被禁用 → `test_scheme_change_mid_batch_reopens_execution` 红（16 != 0）。
- `DCU-F5-pump-revision-fence`：pump revision 栅栏被禁用 → `test_late_callback_before_revision_observed` 红（2 != 1）。
- `DCU-F5-spare-retry-revision`：备用 DI 重试 revision 检查被禁用 → `test_spare_retry_dropped_after_slot_reuse` 红（2 != 1）。

## 独立复跑

- `python3 ci/scripts/run_mutation_guards.py --selftest` → exit 0，`mutation runner decision logic verified`。
- `python3 ci/scripts/run_mutation_guards.py --build build --only DCU-F4-batch-continuation,DCU-F4-served-ident,DCU-F5-pump-revision-fence,DCU-F5-spare-retry-revision` → exit 0，`4/4 guards sensitive`，每条 `[mutated] test rc=8`、`[restored] test rc=0`。
- 跑后 `git status` 干净；产品源码零改动。原文 `verifier/selftest.log`、`verifier/run.log`。

## 已知限制

- 只登记任务指定的四条；`test_late_callback_shift_does_not_fail_meter`、`test_spare_retry_allowed_when_revision_unchanged`、F4R 的 `test_scheme_content_change_reopens_referencing_execution` 未登记，后续可补。
- 全量 arch gates / ASAN 由最终合并树门禁覆盖。
