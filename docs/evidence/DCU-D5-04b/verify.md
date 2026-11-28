# DCU-D5-04b 独立验证报告

## 1. 独立预期推导
- **标准依据**：DL/T 698.45 附录 A.12 / 表 157（Class 12 脉冲计量接口类），OI `0x2402..0x2408` 为脉冲计量 2..8，与 `0x2401` 同属 Class 12（类定义属性 1..19，方法 1..4）。
- **独立期望值**：
  - 原对象表共 136 个对象、ACL 共 131 个对象；补齐 `0x2402..0x2408`（7 个 OI）后，对象总数应为 `136 + 7 = 143`（`<= 224`），ACL 对象总数应为 `131 + 7 = 138`（`<= 224`）。
  - 因无脉冲输入与脉冲电能示值来源，7 个对象保持空掩码（`attr_mask=0, method_mask=0`）与只读 ACL（属性 `1..15`、方法 `1..4`）：
    - GET 属性 `1..15` 应返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）；
    - GET 属性 `16..19` 在 resolver 层应返回 DAR 5，经 Class 20 ACL 应先返回 DAR 3（`EDGE_SLAVE_SRC_DENIED`）；
    - GET 类外属性 `20` 在 resolver 层应返回 `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4）；
    - SET 任意属性应被 ACL 拒为 DAR 3；
    - ACTION 方法 `1..4` 应返回 DAR 5，方法 `5` 应返回 DAR 4。

## 2. 合并树实测结果（`feat/dcu`）
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **全量单元测试**：`ctest --test-dir build --output-on-failure` — **382/382 passed**（含 `test_dcu_698_catalog`、`test_dcu_698_object_model`、`test_dcu_698_appconn`、`test_dcu_698_axdr`、`test_dcu_698_p4_matrix`、`test_driver_dlt698`）。
- **变异守卫**：`DCU-D5-04b-class12-pulse-2402` — 1/1 sensitive（变异后红，恢复后绿）。
- **守卫自检**：`run_guard_selftest.py` — 291/291 + 全部 live 守卫通过。
- **结论**：`VERIFIED`（无 `modules/edge_core` 或受保护头文件改动）。
