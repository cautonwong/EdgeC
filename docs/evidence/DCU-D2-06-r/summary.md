# DCU-D2-06-r：上报方案 store 显式存储与事务助手单一来源（重构）

行为不变的重构，处理 D2-06 的 Comment Sicko MUST KILL。公开 API、AXDR 形状、DAR 分类、SQLite schema、测试全部不变。

## MUST KILL 1：删除 `dcu_scheme_store_storage()`

- `cmd/dcu/dcu_scheme_store.c` 不再导出 `dcu_storage_t *dcu_scheme_store_storage(void)`。
- `cmd/dcu/dcu_report_scheme_store.c` 持有自己的绑定 `static dcu_storage_t *s_storage`，由 `dcu_scheme_store_report_bind(dcu_storage_t *)`（`dcu_scheme_store_internal.h` 声明）在 `dcu_scheme_store_init()` 建表成功后显式注入，与模块自己的 `s_storage = storage` 同一处。
- 组合链：`dcu_node` 组合根调用 `dcu_scheme_store_init(&node->storage, ...)` → store 包装层显式绑定 report 子模块。report TU 不再跨文件读取别人的隐藏状态。

## MUST KILL 2：事务助手单一来源

- 删除 `dcu_scheme_store.c` 的 static `_exec` / `_rollback` / `_begin` / `_commit`。
- 全部调用点改用 `dcu_scheme_store_internal.h` 的 inline `dcu_scheme_store_begin/commit/rollback/exec`（21 处；`_seed_default` 的裸 `BEGIN`/`COMMIT` 也收敛为 `begin`/`commit`）。

## 不变项

- `dcu_scheme_store.h` 公共 API 未改（`dcu_scheme_store_report_apply/count/at/get/op` 签名不变）。
- 测试文件未改（`test_dcu_698_scheme_mgmt` 10 条、`test_dcu_698_schemes` 12 条、`test_dcu_698_axdr` 37 条）。
- 初始化失败的可见行为不变：绑定与 `s_storage` 同在 seed 之前，失败时不回退旧绑定（与 D2-06 一致）。

## 验证

- `check_single_responsibility.py`：干净。
- `run_guard_selftest.py`：105/105。
- 受影响 ctest 15/15。
- ASAN `detect_leaks=1`：scheme_mgmt 10、schemes 12、axdr 37 全过、0 泄漏。
- 验收 `./acceptance/run.sh dcu`：2/2 任务合格（dcu_acceptance 3 passed、dcu_northbound 26 passed）。
- `check_build_warnings.py`：clean Ninja Release build，0 violations。
- `run_fanalyzer.py`：3 known / 0 new。
