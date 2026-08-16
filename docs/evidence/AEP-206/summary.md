# AEP-206 证据包总结 (Summary) - Rework Round 2

- **工单 ID**: AEP-206 (Issue #84)
- **标题**: 清理 SQLite 调用点、头文件路径重定向、显式适配器声明与依赖闭环
- **分支**: `ticket/AEP-206`
- **状态**: Done (Rework Round 2 Completed - Ready for Review)
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **头文件物理迁移与包含路径规范化**:
   - 将 `modules/edge_core/include/sqlite_service.h` 物理迁移至 [modules/edge_infra/include/sqlite_service.h](file:///workspaces/edgeos/EdgeC/modules/edge_infra/include/sqlite_service.h)；
   - 全局 39 个相关 C/H 文件统一解析至 `modules/edge_infra/include/`；
   - 彻底移除临时注入的 `sqlite_svc_load_poll_tasks`，保持 `edge_infra` 基础设施的通用纯粹性，业务特定的轮询任务装载与元数据判定保留在 `cmd/iems/main.c` 本地实现。

2. **显式适配器声明与架构解耦 (Option B 显式声明)**:
   - **`cmd/dcu`**：`dcu_storage.c`、`dcu_inventory.c`、`dcu_dynamic_poller.c` 作为集中器本地微数据库引擎适配器保留，在 `cmd/dcu/CMakeLists.txt` 中恢复显式链接 `SQLite::SQLite3` 并添加架构说明；
   - **`modules/edge_io`**：`snf_manager.c`（Store-and-Forward 本地持久化缓冲）以及 `edge_agent.c`/`io_executor_linux.c`（MQTT 南北向执行器）作为 IO 边缘适配器保留，在 `modules/edge_io/CMakeLists.txt` 中显式声明 `SQLite::SQLite3` 与 `PahoMQTT::PahoMQTT` 依赖；
   - **`cmd/iems`**：在 `cmd/iems/CMakeLists.txt` 中显式声明 `SQLite::SQLite3` 与 `PahoMQTT::PahoMQTT` 依赖，承载 `main.c` 本地任务装载与 `mqtt_producer.c` 适配器；
   - **`modules/edge_core`**：彻底移除 `SQLite::SQLite3` 与 `PahoMQTT::PahoMQTT`，核心层 100% 达成解耦。

3. **`cmd/dcu` 事务与互斥锁架构保护**:
   - `cmd/dcu/dcu_storage.h` 采用前置声明隔离 `<sqlite3.h>`，切断头文件传递性污染；
   - 将 `SQLITE_CONFIG_SINGLETHREAD` 配置从 `cmd/dcu/main.c` 移入 `dcu_storage.c` 内部封装；
   - `cmd/dcu/main.c` 中的大事务调用收敛至 `dcu_storage_begin_trans` / `dcu_storage_commit_trans`，0 裸 SQLite 语法穿透。

4. **R-08 豁免说明**:
   - 本票对 `modules/edge_core/CMakeLists.txt`（移除 SQLite/Paho 外部依赖）与 `modules/edge_core/include/sqlite_service.h`（搬移至 `edge_infra`）的修改属于解耦必要操作，已在 Issue #84 留痕记录，将在 AEP-204 中统一落盘白名单。

5. **全系统全链路回归验证**:
   - `ctest --test-dir build -j1`: **135/135 100% Passed (38.06s)**；
   - `./test.sh --omnis`: **10/10 Passed (52.01s)**；
   - `go test ./...`: 100% Passed；
   - 三大反熵架构门禁脚本全部通过；
   - 所有测试日志与证据文件真实入库。

---

## 2. DoD 逐项验收勾选

- [x] `sqlite_service.h` 落盘 `modules/edge_infra/include/` 并更新所有 C/H 引用
- [x] `edge_core` 彻底清除 `SQLite::SQLite3` 与 `PahoMQTT` 外部链接
- [x] `cmd/dcu`、`modules/edge_io`、`cmd/iems` 显式声明 SQLite/Paho 适配器依赖
- [x] `sqlite_svc_load_poll_tasks` 移出 `edge_infra`，由 `cmd/iems` 本地实现
- [x] `cmd/dcu` 事务与互斥配置在 `dcu_storage` 内部完成封装，`main.c` 0 裸穿透
- [x] `.gitignore` 解除 `tests.log` 忽略，真实入库
- [x] 135/135 CTest -j1 + 10/10 Omnis + Go 测试全绿
- [x] 无未披露违规
