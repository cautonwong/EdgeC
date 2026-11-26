# DCU-D5-05 注释审计

范围：`git diff 383c91cc` 的代码、测试与设计文档；审查者 `pstack:comment-sicko`（report-only）。

## 删除

1. `tests/test_dcu_698_object_model.c` 的 `/* Class 14/15/16 没有统计生产者：对象行只立存在性，掩码必须为空。 */`：
   复述测试名 `test_class14_15_16_stats_rows_are_empty` 与两条 `assert_mask_matches(..., NULL, 0)`；
   非显然的 why 已在生产表行注释与设计文档 §4.2。

## 保留

- `cmd/dcu/dcu_router.h` 的 `IC_INTERVAL_STAT/IC_AVERAGE/IC_EXTREMUM` 行尾类名：承载 DL/T 698 的
  外部词汇（15 是累加平均、16 是极值工具），并沿用该头既有的每类一注释风格。
- `cmd/dcu/dcu_698_object_model.c` 的 2100–2124 行块注释：空掩码是有意为之（无统计生产者），
  没有这条注释时 15 行空掩码与遗漏无法区分。

## 风险

- `tests.log`/`runtime.log` 被 `.gitignore` 的 `*.log` 命中，提交时用 `git add -f`。

## 行为核对

删除的只是测试注释，无行为变化；删除后 `test_dcu_698_object_model` 仍 17/17 通过。
