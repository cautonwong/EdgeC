# EdgeOS 历史源码树 (src/) 死代码审计报告 (Dead Code Audit Report)

- **报告编号**: REPORT-2026-08-16-DEAD-CODE-AUDIT
- **审计范围**: `/workspaces/edgeos/EdgeC/src/` 历史遗留目录
- **审计基准**: CMake 依赖链分析、全局符号追踪与调用图
- **结论**: **`src/` 目录中除已平移的 4 个存活文件外，其余全部为死代码，可安全物理删除**。

---

## 1. 存活文件平移审计 (Surviving Files Relocated in AEP-101)

以下 4 个存活源文件已在 AEP-101 中完整平移归入 `modules/edge_io/src/`：
1. `src/main_loop/northbound_mgr.c` $\rightarrow$ `modules/edge_io/src/northbound_mgr.c`
2. `src/main_loop/sched_engine.c` $\rightarrow$ `modules/edge_io/src/sched_engine.c`
3. `src/main_loop/sched_port.c` $\rightarrow$ `modules/edge_io/src/sched_port.c`
4. `src/platform/linux/io_executor_linux.c` $\rightarrow$ `modules/edge_io/src/io_executor_linux.c`

`modules/edge_io/CMakeLists.txt` 已在 AEP-102 中重构为模块内部相对路径引用。

---

## 2. 待删除死代码清单与无引用证明 (Dead Code Inventory)

| 待删除路径 | 文件功能描述 | 引用检查结果 | 处置结论 |
|---|---|:---:|---|
| `src/CMakeLists.txt` | 历史构建文件（已脱离顶层构建树） | 顶层 CMakeLists.txt 无 `add_subdirectory(src)` | 🗑️ 物理删除 |
| `src/io_executor/README.md` | 历史文档 | 无外部链接 | 🗑️ 物理删除 |
| `src/main_loop/dcu_inventory.c` | 历史 DCU 档案样板（已被 `cmd/dcu/` 替代） | 全局 0 引用 | 🗑️ 物理删除 |
| `src/main_loop/northbound_mgr.c` | 旧路径残留文件（已平移） | 已平移至 `edge_io` | 🗑️ 物理删除 |
| `src/main_loop/policy_syncer.c` | 历史策略同步（已被 `cloud_sync_service` 替代）| 全局 0 引用 | 🗑️ 物理删除 |
| `src/main_loop/sched_engine.c` | 旧路径残留文件（已平移） | 已平移至 `edge_io` | 🗑️ 物理删除 |
| `src/main_loop/sched_port.c` | 旧路径残留文件（已平移） | 已平移至 `edge_io` | 🗑️ 物理删除 |
| `src/platform/CMakeLists.txt` | 历史平台构建文件 | 全局 0 引用 | 🗑️ 物理删除 |
| `src/platform/linux/io_executor_linux.c` | 旧路径残留文件（已平移） | 已平移至 `edge_io` | 🗑️ 物理删除 |
| `src/ui/CMakeLists.txt` | 历史 UI 构建文件 | 全局 0 引用 | 🗑️ 物理删除 |
| `src/ui/core/cli_main_entry.c` | 历史 CLI 入口（已被 `cmd/*/` 替代） | 全局 0 引用 | 🗑️ 物理删除 |
| `src/ui/drivers/ui_driver_ncurses.c` | 历史 ncurses 驱动 | 全局 0 引用 | 🗑️ 物理删除 |
| `src/ui/ui_service.c` | 历史 UI 服务 | 全局 0 引用 | 🗑️ 物理删除 |

---

## 3. 删除安全性评估

- 顶层 CMakeLists.txt、所有 `modules/*/CMakeLists.txt`、`cmd/*/CMakeLists.txt` 与 `tests/CMakeLists.txt` 中均**无任何 `${CMAKE_SOURCE_DIR}/src/` 引用**。
- 删除整个 `src/` 目录不会影响任何库的目标构建、链接与 CTest 单元测试。
