# DCU-D4-10b 独立验证报告

## 1. 独立预期推导
- **标准与规格依据**：DL/T 698.45 Class 25/26 通信参数（`4500..4501`, `4510..4517`）与 `docs/evidence/DCU-D4-10b/spec.md`。
- **独立期望值**：
  - SET 无线 APN/主站 IP/端口与以太网 IP/掩码/网关/主站端口后，回读与 A-XDR 编码应与写入一致；注入持久化 `save` 返回失败时，内存配置应原子回滚至旧值。
  - 非法 A-XDR tag 或截断载荷应返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`（DAR 7）；`cap` 不足时编码应返回 `-1`。

## 2. 合并树实测结果（`feat/dcu`）
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **目标回归**：`test_dcu_698_comm` 4/4 passed，`ctest -R 'dcu|dlt698'` 全绿。
- **守卫自检**：`run_guard_selftest.py`（含 `check_no_dynamic_memory.py`、`check_stack_usage.py`）全绿。
- **ASAN**：`test_dcu_698_comm` 0 泄漏/越界。
- **结论**：`VERIFIED`。
