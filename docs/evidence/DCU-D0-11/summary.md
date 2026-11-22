# DCU-D0-11 测试临时路径隔离

基线与分支：`dcu/D0-11-test-tmp`，工作树 `/tmp/dcu-d0-11`，父提交 `78fd6083`。

## 现象

D1-18c 的 ASAN 目标里 `test_dcu_runtime_health` 偶发 `sqlite3_exec` 返回 `SQLITE_IOERR`，直接重跑与 ctest 重跑都通过。多个 agent 在不同 worktree 并发跑测试，而用例用固定 `/tmp` 路径（如 `/tmp/test_dcu_runtime_health_prod.db`），两个进程共享同一个 SQLite 文件与 WAL。

## 修复

每个测试进程用 `getpid()` 后缀派生路径，测试开始前 `unlink`，结束时清理：

| 文件 | 原路径 | 现路径 |
| --- | --- | --- |
| `tests/test_dcu_runtime_health.c` | `/tmp/test_dcu_runtime_health_{prod,thread,demo}.db` | `/tmp/test_dcu_runtime_health_<tag>_<pid>.db` |
| `tests/test_dcu_node.c` | `/tmp/test_dcu_node{,_events,_srcops}.db` | `/tmp/test_dcu_node<tag>_<pid>.db` |
| `tests/test_dcu_inventory.c` | `/tmp/test_dcu_inv_load.db` | `/tmp/test_dcu_inv_load_<pid>.db` |
| `tests/test_caller_allocated_storage.c` | `/tmp/test_telemetry_caller_alloc.wal`、`/tmp/test_sqlite_caller_alloc.db` | 同名前缀 + `<pid>` |

不碰产品代码。`test_dcu_698_record.c` 已用 `mkstemp("/tmp/dcu-record-XXXXXX")`，`test_storage_powercut_resilience.c` 已用 pid 后缀，保持原样。

## 守卫

`ci/scripts/run_concurrent_binary.sh <binary> [rounds]` 在同一个 build dir 里并发启动两个进程，逐轮等待两者退出；任一失败就打印该进程日志并失败。

注册为 CTest `test_dcu_runtime_health_concurrent`（3 轮，TIMEOUT 180），ASAN/TSAN 构建下带对应 `*_OPTIONS`。它随 `ctest` 与 ASAN 目标一起跑，因此在每次 CI 矩阵里生效。

## 并发结果

- 修复后：`test_dcu_runtime_health_concurrent` 在 Debug 与 ASAN 构建均通过（3 轮 × 2 进程）。
- 修复前证据：D1-18c 的 ASAN 运行里 `test_dcu_runtime_health` 报 `SQLITE_IOERR`，同一 binary 单跑/重跑通过；未在共享 `/tmp` 上重放旧 binary，避免影响其他 agent 正在跑的实例。

## 剩余审计项（不在本 ticket 文件面）

`tests/` 仍有固定 `/tmp` 路径的非 DCU 用例：`test_app_settings.c`、`test_datahub.c`、`test_telemetry_store_crash.c`、`test_infra_interfaces.c`、`test_dds_cli_diagnostics.c`、`test_cli_migrate.c`、`test_web_server_path.c`、`test_mcp_uds.c`、`test_edge_app_mgr.c`、`test_edge_backup.c`。它们同样会在并发 worktree 之间碰撞，建议按同一模式另开 ticket。

## 门禁

- 干净构建 + `check_build_warnings.py`：0 warning
- `ctest -j`：337/337 通过（含并发守卫）
- ASAN：5/5 相关目标通过（含并发守卫）
- `run_guard_selftest.py`：全部通过
- 验收 `test.sh --omnis dcu`：dcu-acceptance 3/3、dcu-northbound 10/10
- `git diff --check`：干净
