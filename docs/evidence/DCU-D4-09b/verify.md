# DCU-D4-09b 独立验证报告

## 1. 独立预期推导
- **标准与规格依据**：DL/T 698.45 Class 23 总加组（`0x2301..0x2308`）与 `docs/evidence/DCU-D4-09b/spec.md`。
- **独立期望值**：
  - 成员 A（`+1000.0`）、成员 B（`-300.0`，`add_sub_flag=1`）、成员 C（`500.0`，`calc_flag=0`）聚合计算：总加有功功率 = `1000.0 - 300.0 = 700.0`（成员 C 不参与）。
  - 滑差窗口深度为 `15`：注入 16 个样本后，最旧样本被淘汰，滑差平均仅覆盖最近 15 个样本。
  - 无有效样本时，总加与滑差不除零、不伪造。

## 2. 合并树实测结果（`feat/dcu`）
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **目标回归**：`test_dcu_698_group` 5/5 passed，`ctest -R 'dcu|dlt698'` 全绿。
- **守卫自检**：`run_guard_selftest.py`（含 `check_no_dynamic_memory.py`）全绿。
- **ASAN**：`test_dcu_698_group` 0 泄漏/越界。
- **结论**：`VERIFIED`。
