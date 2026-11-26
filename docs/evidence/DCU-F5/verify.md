# DCU F5 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/F5-late-callback-write`，合并 tip `08f59c6a`，基线 `563c9f6f`。

## 结论

VERIFIED。首轮交回树上有两个对抗评审可复现的窗口：迟到回调跨未观测的档案 revision 续发，以及 slot 耗尽复用绕过备用 DI 重试守卫。第二轮以档案 revision 边界修复。独立复跑红/绿、验收、ASAN、守卫与静态分析全部通过。

## 红先绿后（独立复跑）

- 测试 commit `8b2b827c`：`./build/tests/test_dcu_poller` 27 例中 3 红（`test_late_callback_before_revision_observed`、`test_late_callback_shift_does_not_fail_meter`、`test_spare_retry_dropped_after_slot_reuse`），24 绿。原文 `verifier/round2-red.log`。
- 修复 commit `f26118c6`：同一命令 27/27 绿。原文 `verifier/round2-green.log`。
- `ctest -R 'dcu|dlt698'`：45/45，52.64s。

## 对抗评审

- adv-2 复现“迟到回调跨未观测 revision 续发，B 的读数按 B 协议缩放却记到 A 名下”，以及“slot 耗尽复用后新表继承旧 slot，重试守卫放行”。
- adv-1 复现同一根因的“表只移位未删，旧下标续发被拒，一帧未发却给健康表记访问失败与熔断 strike”。
- 修复轮的 pump revision 栅栏与 `bus_ctx.revision` 重试门覆盖两路。产品回归见 `tests/test_dcu_poller.c` 的 4 个新用例。

## 验收（COVERAGE=ON，安静窗口）

3 连绿，2/2 任务，35 passed，22.80/22.97/22.94s。原文 `verifier/accept-{1,2,3}.log`。

## 运行时与门禁（合并树 `feat/dcu` `08f59c6a`）

- ASAN（Debug+ASAN，`detect_leaks=1`）`test_dcu_poller` 27/27，无 sanitizer 报告。原文 `verifier/asan-run.log`。
- `run_guard_selftest.py` 全绿（exit 0）。
- `check_build_warnings.py`（coverage clean build log）0 新增。
- `run_fanalyzer.py`：3 known / 0 new。
- `ctest -R 'dcu|dlt698'` 45/45。

## 已知限制

- 本机 TSAN 仍受 `vm.mmap_rnd_bits=32` 限制，依赖 CI matrix。
- 既有不一致未在本单修改：`dcu_event_monitor` 与 `dcu_node_record_reading` 把 `device_id` 当档案下标；`slot_id` 在 4096 次分配后回收。重试路径已由 revision 门覆盖，遥测历史 `device_id` 语义变化见 `summary.md` §6。
