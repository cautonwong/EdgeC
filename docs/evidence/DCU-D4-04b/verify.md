# DCU-D4-04b 独立验证报告

## 1. 独立预期推导
- **标准与规格依据**：DL/T 698.45 Class 19 `0x4300` 设备管理与 `docs/evidence/DCU-D4-04b/spec.md`。
- **独立期望值**：
  - `A2..A5` 编码输出符合 A-XDR 标签（`visible-string` / `structure` / `date_time_s` / `array`），且 `cap == 0` 或不足时返回 `-1`。
  - 注入软件初始化 ops 后，`M3`（数据初始化）、`M5`（事件初始化）、`M6`（需量初始化）分别精确命中对应回调；`M1`（设备复位）与 `M4`（恢复出厂）无硬件端口时返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）。

## 2. 合并树实测结果（`feat/dcu`）
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **目标回归**：`test_dcu_698_devmgr` 6/6 passed，`ctest -R 'dcu|dlt698'` 全绿。
- **守卫自检**：`run_guard_selftest.py`（含 `check_no_dynamic_memory.py`）全绿。
- **ASAN**：`test_dcu_698_devmgr` 0 泄漏/越界。
- **结论**：`VERIFIED`。
