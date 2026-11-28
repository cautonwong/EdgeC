# DCU-D5-05b 独立验证报告

## 1. 独立预期推导
- **标准与规格依据**：DL/T 698.45 Class 14/15/16（`0x2100..0x2124`，15 个 OI）与 `docs/evidence/DCU-D5-05b/spec.md`。
- **独立期望值**：
  - 未配置统计 OAD 时，读 `0x21000200` 应返回空 A-XDR 数组 `01 00`（2 字节），不伪造样本值。
  - 通过 Method 3 添加统计条目 `{src_oad=0x20000200, period_s=60}` 并连续注入 3 个样本（如 `220.0`, `230.0`, `225.0`）：
    - 区间平均应为 `(220.0 + 230.0 + 225.0) / 3 = 225.0`，最小 `220.0`、最大 `230.0`、计数 `3`；
    - 执行 Method 1 复位后，计数归 `0` 但配置条目保留；
    - 执行 Method 4 删除后，恢复为空数组 `01 00`。

## 2. 合并树实测结果（`feat/dcu`）
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **目标回归**：`ctest -R 'dcu|dlt698'` 全绿（含 `test_dcu_698_stat` 4/4、`test_dcu_698_axdr`、`test_dcu_698_object_model`）。
- **守卫自检**：`run_guard_selftest.py`（含 `check_no_dynamic_memory.py`）全绿。
- **ASAN**：`test_dcu_698_stat` 0 泄漏/越界。
- **结论**：`VERIFIED`。
