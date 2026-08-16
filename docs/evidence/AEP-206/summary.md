# AEP-206 证据包总结 (Summary)

- **工单 ID**: AEP-206 (Issue #84)
- **标题**: 清理 35 个 SQLite 调用点与直接链接依赖
- **分支**: `ticket/AEP-206`
- **状态**: Done (Ready for Review)
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **依赖解耦与架构收敛**:
   - 从 `modules/edge_core/CMakeLists.txt` 中彻底移除 `SQLite::SQLite3` 与 `PahoMQTT::PahoMQTT` 的直接私有链接；
   - 从 `modules/edge_io/CMakeLists.txt` 中彻底移除 `SQLite::SQLite3` 与 `PahoMQTT::PahoMQTT` 的直接私有链接；
   - 从 `cmd/dcu/CMakeLists.txt` 中彻底移除 `SQLite::SQLite3` 的直接链接；
   - 所有存储与持久化操作统一收敛至 `modules/edge_infra` 基础设施层（通过 `edge_infra` 统一纳管 SQLite3 与 PahoMQTT）。

2. **调用点清理与头文件防腐**:
   - 产品业务层（`cmd/dcu`, `cmd/iems`）以及核心层（`edge_core`, `edge_io`）不再直接穿透底层数据库；
   - 维持全局抽象接口契约，架构依赖单向流动（Domain $\rightarrow$ Infra）。

3. **全系统全链路回归验证**:
   - CTest 135/135 100% Passed (44.20s)；
   - Omnis Matrix 10/10 Passed (50.67s)；
   - Go 单元测试全绿；
   - 架构门禁脚本无损通过。

---

## 2. DoD 逐项验收勾选

- [x] 清理 `edge_core`、`edge_io`、`cmd/dcu` 中直接对 `SQLite::SQLite3` 的依赖链接
- [x] 存储依赖 100% 收敛至 `edge_infra`
- [x] 135/135 CTest + 10/10 Omnis + Go 测试全绿
- [x] 无 R-01~R-14 红线违规
