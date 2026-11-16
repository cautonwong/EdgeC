# SQLite 单连接并发

日期：2026-11-14
状态：已实施最小完整修复（刷新 worker 独立连接）

## 1. 问题

`sqlite_service` 的主连接被 loop 线程和 uv worker 共用。SQLite 的串行化模式只保证单次 API 调用安全，不保证 `BEGIN IMMEDIATE` 到 `COMMIT` 的序列。两条路径同时开事务时，一条语句会被卷进另一条路径的事务，或者 `BEGIN` 直接返回 `database is locked`。

已确认的可达路径：

- 刷新 worker：`modules/edge_infra/storage/sqlite_service.c:380` 的 `_flush_batch_internal` 在 `_worker_flush_cb` 里对共享连接执行 `BEGIN IMMEDIATE` / `COMMIT`。
- loop 线程：`adapters/sqlite_adapters/src/app_points_sqlite.c:145` 的 `app_points_sync` 在安装 after_work 里对同一连接执行 `BEGIN IMMEDIATE`。
- 两者可以同时发生。刷新的 `BEGIN` 成功时，`app_points_sync` 的 `BEGIN` 返回 busy，安装失败；反过来，app 点位的写入会加入刷新事务，刷新异常时一起回滚。

管理器的安装、升级、启停 worker 只做单条 `save_app` / `save_limits` / `delete_limits`，不开事务。`edge_app_mgr.c:1082-1085` 在 after_work 回调前清空 `job_active`，而下一个任务只能从 loop 线程提交，所以 manager worker 不会与 `app_points_sync` 重叠。

## 2. 约束

- 不改 schema，不引入第二个数据库文件。
- 备份/恢复需要跨多条语句的独占事务。
- 安装/升级的 20 MB 包写入必须留在 worker，不能回到 loop 线程。
- `:memory:` 数据库不能跨连接共享。

## 3. 选项

### A. 递归连接锁，begin/commit 交接

所有拿 `sqlite_svc_get_db_handle` 的生产入口先 lock、结束 unlock；`begin_trans` 加锁，`commit_trans` 解锁。

优点：能覆盖未来新增的 worker 事务。
缺点：锁跨 API 边界，begin 之后遗漏 commit 或 rollback 会永久持锁；需要审计全部适配器。

### B. 每线程一个连接

每个线程 `sqlite3_open` 自己的连接，开 WAL 和 `busy_timeout`。事务天然按连接隔离。

优点：没有跨 API 的锁。
缺点：写锁竞争变成 SQLITE_BUSY，需要统一超时；连接生命周期要跟随线程池。

### C. 单写队列

所有写操作投递到一个专用写线程。

优点：长期最干净。
缺点：改动最大，涉及任务模型和同步 API。

### D. 刷新 worker 独立连接（已实施）

刷新 worker 自己开一个连接，主连接只服务 loop 线程和单条语句的 manager worker。两条连接都在 WAL 下，写冲突由 `sqlite3_busy_timeout(5000)` 等待而不是立即失败。`:memory:` 或空路径退回主连接，保持测试兼容。

优点：改动最小，覆盖当前唯一可达的 worker 事务，不需要给适配器加锁。
缺点：未来如果在主连接上新增 worker 事务，竞态会回来。需要把"主连接事务只在 loop 线程"写成不变量。

## 4. 建议

采用 D，已实施。A 作为未来新增 worker 事务时的升级路径；C 只在写吞吐成为瓶颈时评估。

## 5. 实施记录

1. `sqlite_svc_t` 增加 `flush_db`，`_worker_flush_cb` 通过 `flush_connection()` 懒加载独立连接。
2. 主连接和刷新连接都设置 `busy_timeout=5000`。刷新连接补 `synchronous=NORMAL` 和 `temp_store=MEMORY`。
3. `_flush_batch_internal` 和 `_ensure_shard_exists` 接受 `sqlite3 *db` 参数；同步刷盘仍用主连接。
4. `test_flush_worker_persists` 用文件库验证异步 worker 写入落到同一个库。
5. `sqlite_svc_destroy` 先等 `is_flushing` 清零，再关闭刷新连接和主连接。

## 6. 不变量

- 主连接上的 `BEGIN` / `COMMIT` 只在 loop 线程发起。
- worker 只做单条语句，或使用自己的连接。
- `:memory:` 只用于单线程测试。

## 7. 不做什么

- 不改 schema 版本。
- 不把 worker 的文件写搬回 loop 线程。
- 不引入单写队列。
