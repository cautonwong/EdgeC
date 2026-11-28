# DCU-D4-WIRING 独立验证报告

## 1. 独立预期推导
- **标准与架构依据**：DL/T 698.45 Class 23（`0x2301..0x2308`）与 Class 19（`0x4300`）、`docs/evidence/DCU-D4-WIRING/spec.md`、`check_composition_root_budget.py`（`dcu_north_adapter.c <= 205` 行）。
- **独立期望值**：
  - `0x2301` 经 M3 添加成员后，`dcu_north_resolve_axdr` 读 `0x23010200` 应返回包含该成员的 A-XDR 数组；M5 清空后应恢复为空数组。
  - `0x4300` `A2..A5` 经 `dcu_north_resolve_axdr` 应返回合法 A-XDR 数据；`M3`/`M5`/`M6` 走软件初始化端口；`M1`/`M4` 无硬件复位端口时应诚实返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）。
  - 抽取 `dcu_698_axdr_handles_action_oi` 后 `cmd/dcu/dcu_north_adapter.c` 行数应 `<= 205`。

## 2. 合并树实测结果（`feat/dcu`）
- **行数预算**：`cmd/dcu/dcu_north_adapter.c` 缩减至 `201` 行（预算 `205`）。
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **目标回归**：`ctest -R 'dcu|dlt698'` 全绿。
- **守卫自检**：`run_guard_selftest.py` 全绿。
- **ASAN**：`test_dcu_698_axdr`、`test_dcu_698_object_model`、`test_dcu_698_p4_matrix` 0 泄漏/越界。
- **结论**：`VERIFIED`。
