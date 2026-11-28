# DCU-D4-06b 独立验证报告

## 1. 独立预期推导
- **标准与规格依据**：DL/T 698.45 Class 18 文件传输（`F000..F002`）与 `docs/evidence/DCU-D4-06b/spec.md`。
- **独立期望值**：
  - 启动 512 字节（2 块 × 256 B）传输并写入块 0/1 后，回读字节应逐字节一致且 IEEE 802.3 CRC32 校验通过；缺块或篡改任意 1 字节应使 `verify` 失败。
  - 超出 `16384` B 总长或越界块号应被拒绝，不越界写；未注入 DFU 硬件端口时 `activate`/`rollback` 应诚实返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）。

## 2. 合并树实测结果（`feat/dcu`）
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **目标回归**：`test_dcu_698_file` 6/6 passed，`ctest -R 'dcu|dlt698'` 全绿。
- **守卫自检**：`run_guard_selftest.py`（含 `check_no_dynamic_memory.py`、`check_stack_usage.py`）全绿。
- **ASAN**：`test_dcu_698_file` 0 泄漏/越界。
- **结论**：`VERIFIED`。
