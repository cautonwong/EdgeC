# AEP-202 证据包总结 (Summary)

- **工单 ID**: AEP-202 (Issue #80)
- **标题**: 迁移 `sqlite_service` → `modules/edge_infra/storage/`（保留 WAL/SnF 语义）
- **分支**: `ticket/AEP-202`
- **状态**: Done (Ready for Review)
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **抽象虚表补齐 (notes 3 ops 面扩展与解耦)**:
   - 在 `modules/edge_infra/include/storage_ops.h` 完整定义 WAL 操作 (`checkpoint`, `flush`, `compact`, `get_wal_size`)、分区生命周期 (`partition_gc`, `purge_old_data`)、SnF 离线队列 (`snf_enqueue`, `snf_dequeue`, `snf_get_pending_count`, `snf_flush`)、参数与 App 注册表管理接口；
   - 彻底解除 `storage_ops.h` 与 `datahub_ops.h` 对 `shmring.h` 的直接头文件耦合。

2. **存储源码物理迁移与驱动封装**:
   - 将 `modules/edge_core/src/sqlite_service.c` 物理平移至 `modules/edge_infra/storage/sqlite_service.c`；
   - 实现 `modules/edge_infra/storage/sqlite_storage_driver.c`，通过 `g_sqlite_storage_ops` 暴露标准的 Port-Adapter 适配层，并提供 `storage_driver_sqlite_create` 注入工厂；
   - 100% 完整保留 ADR-0007 所规定的 WAL 预写日志模式、分表分片、空间 GC 审计、事务一致性与 SnF 缓存语义。

3. **构建拓扑与依赖管理**:
   - `modules/edge_infra/CMakeLists.txt` 构建 `edge_infra` 静态库，承载存储驱动并链接 `SQLite::SQLite3` 与 `libuv::uv_a`；
   - `modules/edge_core/CMakeLists.txt` 移除 `src/sqlite_service.c` 并链接 `edge_infra`；
   - `cmd/dcu`、`cmd/iems`、`cmd/smart_box` 及 `tests` CMake 统一接入 `edge_infra` 链接拓扑。

4. **符号完整性与回归测试**:
   - `nm -g` 验证：所有 15 个 `sqlite_svc_*` 全局符号无损导出，并新增 `g_sqlite_storage_ops` 与 `storage_driver_sqlite_create`；
   - `tests/test_infra_interfaces.c` 扩充对 `g_sqlite_storage_ops` 虚表及 live SQLite 驱动的全生命周期测试；
   - CTest 135/135 100% Passed, Omnis Matrix 10/10 Passed, Go 单元测试全绿。

---

## 2. DoD 逐项验收勾选

- [x] `sqlite_service.c` 成功迁入 `modules/edge_infra/storage/`
- [x] 实现 `sqlite_storage_driver.c` 并导出 `g_sqlite_storage_ops`
- [x] 补齐 notes 3 要求的 WAL/压实/GC/SnF 等完整 ops 虚表面
- [x] 符号验证 `nm -g` 差异为 0（原有 `sqlite_svc_*` 符号全部保留）
- [x] 135/135 CTest + 10/10 Omnis + Go 测试 + 三大反熵门禁全绿
- [x] 无 R-01~R-14 红线违规
