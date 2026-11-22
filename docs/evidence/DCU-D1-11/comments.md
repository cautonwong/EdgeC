# DCU-D1-11 注释审计（Python）

范围：`1ca7130a` 到工作树，`acceptance/dcu/tests/test_dcu_northbound.py` 的 Python 注释与 docstring。

## 删除（3 处）

1. `test_set_event_report_flag_round_trip` 的 docstring：与 `@case(clause=...)` 和三条断言逐字重复。
2. `test_set_event_valid_flag_round_trip` 的 docstring：同上。
3. `test_set_event_wrong_type_returns_type_mismatch` 的 docstring：重复 clause 与 `res[7] == 7`。

## 保留

- 模块头的验收判据列表：同文件验收合同文字，按 AGENTS.md 与行为同步。
- `_NorthSession`、`_set_request`、`_connect_request`、F200 A4、空范围、Class 20 的 docstring：协议帧布局或会话/模型事实，代码本身看不出。
