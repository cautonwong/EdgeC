# DCU-D5-04 注释审计

范围：测试提交 `6ae69e78` 与实现提交 `a105a4cb` 的新增注释；审查者 `pstack:comment-sicko`
（report-only，2 分 39 秒、21 次工具调用），另附人工自评。

## 重写

1. `tests/test_dcu_698_appconn.c` 的 `16/17 记 HOLD，18/19 超 ACL schema` 改为
   `16 记 HOLD，17..19 超 ACL schema`：ACL 属性 schema 只到 16（`DLT698_ACL_MAX_ATTRS`），
   17 也在 schema 之外；原句把 17 放在 HOLD 一侧，会让人以为补完冲突后 17 能进 ACL。
   DAR 结果不变。

## 增补（缺失的 why）

1. `cmd/dcu/dcu_698_appconn.c` 的 `k_methods_pulse`：补充“方法 1..4 广告只读，ACTION
   才能越过 ACL 到对象表如实回 DAR 5；撤掉广告会静默变成 DAR 3”。对象 `method_mask`
   为空但 ACL 仍广告方法，是诚实骨架的关键选择，代码本身看不出；删掉广告后同样的
   ACTION 会从 DAR 5 变成 ACL 的 DAR 3。

## 保留

- `tests/test_dcu_698_axdr.c` 的 `表157 类定义 A1..A19；16/17 的归属在表157/158 之间
  有冲突...`：冲突与 ACL 先于 resolver 的顺序是断言看不出的外部事实，解释了 A16/17 与
  A20 的 DAR 分类。
- `tests/test_driver_dlt698.c` 的 `ACL 广告 A1..A15...`：把断言的 DAR 映射到
  ACL-before-resolver 顺序，fixture 字节与断言本身不携带该信息。
- `cmd/dcu/dcu_698_appconn.c` 的 `k_attrs_2401`：解释 15 项数组为何在类定义 19 项处停下
  （schema 上限 16，16 记 HOLD）。
- `cmd/dcu/dcu_698_object_model.c` 的 2401 行：无脉冲量来源是唯一不能用代码表达的事实，
  其余沿用类 14/15/16/17 的诚实 DAR 注释惯例。
- `cmd/dcu/dcu_router.h` 的 `IC_PULSE`：沿用该头每条宏的“类名 + 示例 OI”公共词汇。
- `cmd/dcu/dcu_router.h` 的 `OI_PULSE_METER`：7.3.12 / 表157 出处无法从 `0x2401` 推出。

## 删除

0 条（MUST KILL: none）。审查者未提出代码改动。

## 行为核对

仅注释改动；`test_dcu_698_appconn` 与 `test_driver_dlt698` 重跑绿（2/2）。
`docs/design/DCU_698_OBJECT_MODEL.md` 无新增注释；Class 12 行的 16/17 与 18/19
表述正确，未改。
