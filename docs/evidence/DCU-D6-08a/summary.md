# DCU-D6-08a TSAN lock-order-inversion

基线与分支：`dcu/D6-08a-tsan-lock`，工作树 `/tmp/dcu-d6-08a`，父提交 `fb0f16b9`。

## 现象

TSAN 构建里 `test_dcu_runtime_health` 报 lock-order-inversion：

```
Cycle in lock order graph: M18 (…) => M28 (…) => M18
  Mutex M28 acquired here while holding M18: dcu_node_create_in
  Mutex M18 acquired here while holding M28: dcu_node_destroy
```

第二次运行时还有 M18 ⇄ M43 的同类环（同一测试）。

## 定位

两个栈帧都是 `pthreadMutexEnter`（SQLite 自己的 unix 互斥量包装，不是 uv 的 `uv_mutex_lock`），地址分别落在 `dcu_node_create_in` 调用链里的 `dcu_storage_init`/`dcu_inventory_load_from_db` 与 `dcu_node_destroy` 调用的 `dcu_storage_destroy` 内。用 `addr2line` + 反汇编确认：

- create：`dcu_storage_init`（打开 A 连接）→ `dcu_inventory_load_from_db`（打开并使用 B 连接）；
- destroy：`dcu_storage_flush`（写 A）→ `sqlite3_finalize`/`sqlite3_close`（关 A）。

环的两端都是 SQLite 内部的全局锁与连接锁，在 open 路径按 G→C、close 路径按 C→G 获取。这是第三方库内部的顺序，用户代码没有包住 SQLite 调用的自有锁。

## 是否真实死锁

不是。storage 的所有权是单一 uv 循环线程：

- `cmd/dcu` 不创建线程（`uv_thread_create` / `pthread_create` 全仓搜索为空）；
- `dcu_node_create_in`、心跳刷盘、`dcu_node_destroy` 都在同一个循环线程上调用；
- 新增用例 `test_node_lifecycle_runs_on_one_thread` 在 create、telemetry 写入 + flush、destroy 四处记录 `pthread_self()` 并要求相等，证明这条生命周期没有跨线程路径。

没有可并发的第二个 SQLite 调用者，环不可达。SQLite 是 `.local` 里的预编译库，改不到它的编译选项（`SQLITE_THREADSAFE=0` 会更彻底地移除互斥量，但不在本仓库构建范围内）。

## 决定

按任务允许的"第三方库假阳性"路径处理：

1. `ci/tsan.supp`：只抑制 `deadlock:pthreadMutexEnter`（SQLite 的互斥包装），不抑制 uv 锁或任何第一方函数。
2. `tests/CMakeLists.txt` 的四处 TSAN_OPTIONS 加上 `suppressions=${CMAKE_SOURCE_DIR}/ci/tsan.supp`。
3. `cmd/dcu/dcu_storage.c` 的 init 注释写明单线程所有权，并指向 suppression 的原因。
4. 新增单线程生命周期用例。

## 验证

- A/B：同一 TSAN 二进制，无抑制时 3 条 lock-order 警告；带 `ci/tsan.supp` 时 0 条。
- TSAN `test_dcu_runtime_health` 直接运行：3/3 通过，无 ThreadSanitizer 输出（含新增单线程用例）。
- TSAN `ctest -R 'dcu|dlt698'`：32/32；把 32 个测试二进制逐个直接跑，TSAN 行数为 0。
- Release `ctest -R 'dcu|dlt698'` 32/32、ASAN 32/32、告警门禁 0 新增、守卫通过、验收 3 + 9、`git diff --check` 干净。

完整输出见 `tests.log` 与 `runtime.log`。
