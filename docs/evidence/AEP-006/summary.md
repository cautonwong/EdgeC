# AEP-006 证据包总结 (Summary)

- **工单 ID**: AEP-006
- **标题**: 核心目录只读门禁原型与 CODEOWNERS 配置
- **状态**: Done
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. 建立 `.github/CODEOWNERS`，将 `/modules/edge_core/`、`/include/contract_data_types.h`、`/contracts/schema/` 与顶层构建文件锁定，要求架构委员会双人评审签署。
2. 实现机械化门禁脚本 `scripts/check_core_readonly.sh`，可基于 PR 差异自动化扫描受保护核心文件变动，支持告警提示与严格模式阻断（`STRICT_CORE_GATE=1`）。

---

## 2. DoD 逐项验收勾选

- [x] `.github/CODEOWNERS` 配置生效
- [x] `scripts/check_core_readonly.sh` 脚本可执行并通过测试
- [x] 故意修改 core 文件可触发门禁告警与阻断
- [x] 无 R-01~R-14 红线违规
