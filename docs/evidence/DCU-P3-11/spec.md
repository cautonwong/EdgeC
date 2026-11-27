# DCU-P3-11 Specification: `bus_orchestrator_destroy` Resource Teardown

## 1. 业务规则 (Business Rules)

- `bus_orchestrator_destroy(ctx)`：
  1. 若 `ctx == NULL`，直接返回。
  2. 遍历 `i = 0 .. min(ctx->max_buses, ARBITER_MAX_BUSES) - 1`：
     - 若 `ctx->engines[i] != NULL`，若其 `detach` 回调非空则调用 `ctx->engines[i]->detach(ctx->engines[i])`，随后将 `ctx->engines[i]` 置 `NULL`。
     - 将 `ctx->io_handles[i]` 置 `NULL`。
  3. 若 `ctx->loop != NULL` 且 `ctx->audit_timer.type == UV_TIMER` 且 `!uv_is_closing((uv_handle_t *)&ctx->audit_timer)`：
     - 调用 `uv_timer_stop(&ctx->audit_timer)`；
     - 调用 `uv_close((uv_handle_t *)&ctx->audit_timer, NULL)`。
  4. 若 `ctx->pool != NULL`，调用 `net_buf_pool_unref(ctx->pool)` 并置 `NULL`。
  5. 若 `ctx->slave_pool != NULL`，调用 `net_buf_pool_unref(ctx->slave_pool)` 并置 `NULL`。
  6. 若 `g_active_orch == ctx`，将 `g_active_orch` 置 `NULL`。
  7. 对部分初始化（例如 `pool == NULL`、`slave_pool == NULL`、`engines[i] == NULL`、`audit_timer` 未初始化）或重复调用保持安全。
- `_slave_detach(self)`（`modules/edge_io/src/engines/slave_engine.c`）：
  - 与 `_master_detach` 一致，不解引用 `priv->io`（`io_handle_t` 由调用方持有，可能在 `bus_orchestrator_destroy` 前已销毁）。

## 2. 不变量 (Invariants)

- 执行 `bus_orchestrator_create_in` + `bus_orchestrator_setup_master` + `bus_orchestrator_setup_slave` + `bus_orchestrator_run` + `bus_orchestrator_destroy` 并经 `uv_run(loop, UV_RUN_DEFAULT)` 排空关闭回调后：
  - ASAN/LSAN（`detect_leaks=1`）报告 0 字节内存泄漏、0 UAF / double-free。
  - `uv_loop_close(&loop)` 返回 `0`。

## 3. 可观测输出 (Observable Outputs)

- ASAN/LSAN 下运行 `test_bus_orchestrator` 退出码为 `0`，无 LeakSanitizer 报错，无 AddressSanitizer 报错。
- `uv_loop_close(&loop) == 0`。
- 既有 `edge_io`、`dcu`、`iems` 测试全部通过。

## 4. 验收判据 (Acceptance Criteria)

- `tests/test_bus_orchestrator.c` 新增回归测试 `test_bus_orchestrator_destroy_releases_engines_and_pools`，覆盖 `create_in` + `setup_master` + `setup_slave` + `run` + `destroy` + `uv_run` + `uv_loop_close == 0`，在 ASAN/LSAN 下通过。
- 移除 `bus_orchestrator_destroy` 中的 engine/pool 释放后，该回归测试在 ASAN/LSAN 下必红（通过 `ci/mutation_guards.tsv` 或 ASAN 红/绿对照验证）。
- Clean build + `python3 ci/scripts/check_build_warnings.py` 无新增编译告警。
- `python3 ci/scripts/run_guard_selftest.py` 通过。

## 5. 非目标 (Non-goals)

- 不修改 `bus_orchestrator_storage_t` 公开 ABI。
- 不修改 `modules/edge_core` 或受保护的 foundation 头文件。
