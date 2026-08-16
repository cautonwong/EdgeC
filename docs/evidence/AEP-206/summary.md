# AEP-206 证据包总结 (Summary) - Reworked

- **工单 ID**: AEP-206 (Issue #84)
- **标题**: 清理 SQLite 调用点、头文件路径重定向与依赖收敛
- **分支**: `ticket/AEP-206`
- **状态**: Done (Rework Completed - Ready for Review)
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **头文件物理迁移与包含路径规范化**:
   - 将 `modules/edge_core/include/sqlite_service.h` 物理迁移至 [modules/edge_infra/include/sqlite_service.h](file:///workspaces/edgeos/EdgeC/modules/edge_infra/include/sqlite_service.h)；
   - 全局 39 个相关 C/H 文件统一解析至 `modules/edge_infra/include/`，彻底消除旧路径依赖；
   - 在 `.gitignore` 中增加 `!docs/evidence/**/tests.log` 豁免规则，确保所有证据包测试日志全部正确入库。

2. **穿透调用消除与存储接口封装**:
   - **`cmd/iems/main.c`**：移除 `#include <sqlite3.h>` 以及裸 `sqlite3_open_v2` / `sqlite3_exec` 调用，在 `edge_infra/storage/sqlite_service.c` 中新增抽象接口 `sqlite_svc_load_poll_tasks` 统一装载总线轮询任务并处理仿真元数据判定；
   - **`cmd/dcu/main.c`**：移除裸 `sqlite3_exec` 事务控制代码，封装为 `dcu_storage_begin_trans` 与 `dcu_storage_commit_trans` 事务方法；
   - **`cmd/dcu/dcu_storage.h`**：使用结构体前置声明替代直接包含 `<sqlite3.h>`，杜绝头文件传递性污染；
   - **`tests/test_dcu_storage.c` & `test_dcu_integration.c`**：显式包含 `<sqlite3.h>` 以满足单元测试直接断言需求。

3. **构建拓扑收敛与依赖清理**:
   - `modules/edge_core/CMakeLists.txt`：彻底移除 `SQLite::SQLite3` 与 `PahoMQTT::PahoMQTT`；
   - `modules/edge_io/CMakeLists.txt`：彻底移除 `SQLite::SQLite3` 与 `PahoMQTT::PahoMQTT`；
   - `cmd/dcu/CMakeLists.txt`：彻底移除 `SQLite::SQLite3`；
   - 底层数据库与总线依赖 100% 收敛至 `edge_infra` 基础设施静态库。

4. **全系统全链路回归验证**:
   - `ctest --test-dir build -j1`: **135/135 100% Passed (35.81s)**；
   - `./test.sh --omnis`: **10/10 Passed (53.34s)**；
   - `go test ./...`: 100% Passed；
   - 三大反熵架构门禁脚本无损通过。

---

## 2. DoD 逐项验收勾选

- [x] `sqlite_service.h` 落盘 `modules/edge_infra/include/` 并更新所有 C/H 引用
- [x] `edge_infra` 外的业务代码（`cmd/iems`, `cmd/dcu`）消除裸 `sqlite3_*` 调用
- [x] `edge_core`、`edge_io`、`cmd/dcu` 清理 `SQLite::SQLite3` 与 `PahoMQTT` 直接链接
- [x] `.gitignore` 解除 `tests.log` 忽略，真实入库
- [x] 135/135 CTest -j1 + 10/10 Omnis + Go 测试全绿
- [x] 无 R-01~R-14 红线违规
