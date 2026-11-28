# DCU-D3-08c 独立验证 (`verify.md`)

规格：[spec.md](spec.md)。基线 `f443e688`，实现 `18e78587`，注释审计 `1a9e91de`，合并 `7232c03f`。

## 独立推导

- `docs/698_parsed.md:6940-6946`：300B 为 Class 24，属性 5（配置参数）与 6..9（四张事件记录表）。
- Class 24 类表属性上限 14 → A5..A9 已定义，A15 未定义。
- 因此 resolver 期望：A5..A9 → `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5），A15 → `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4）。
- ACL 只读广告 5..9 → 驱动 GET A15 先被 ACL 拒为 DAR 3；SET A5 同样 DAR 3。

## 观测

| 期望 | 观测 | 结果 |
| --- | --- | --- |
| resolver A5..A9 = DAR 5，A15 = DAR 4 | `CMOCKA_TEST_FILTER=test_class24_300b_empty_event_row ctest -R '^test_dcu_698_object_model$'` | PASS |
| 驱动 GET A5..A9 = DAR 5；A15/SET A5 = DAR 3 | `test_slave_class24_300b_dar`（`test_driver_dlt698`） | PASS |
| 对象 136、ACL 131、对象列表 6476 B | `test_dcu_698_catalog` / `test_dcu_698_p4_matrix` / `test_dcu_698_scheme_mgmt` | PASS |
| 无伪造事件 | 300B 空掩码且无实例；没有记录/计数路径 | 代码核对 |

## 结论

**VERIFIED**。边界：A15 的 DAR 4 只能在 resolver 层观测，驱动层按 ACL 先回 DAR 3，符合现有 Class 7 先例。

## 覆盖率复核（2026-11-28 CI 反馈）

D3-08c 初版在 `read`/`write`/`execute` 里也加了掩码分支，但 `dcu_698_read`/`dcu_698_write`/`dcu_698_call`
在调用处理器前已经检查掩码，这三处分支不可达，使 `cmd/dcu` 覆盖率跌到 86.78%（CI 红）。
删除这三段死代码后本地 COVERAGE 全量重测：6117/7043 = **86.8522%**，低于 86.8 的下限恢复为 PASS；下限不放宽。
`encode_axdr` 的掩码在前检查保留，北向 300B 的 DAR 5 观测路径不变。
