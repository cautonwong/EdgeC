# DCU-D1-10B 注释审查

工具：`pstack:comment-sicko`（只读子代理），范围为 `4da3a938..HEAD` 的
`scripts/extract_dcu_698_catalog.py`、`tests/test_dcu_698_catalog_source_window.py`、
`ci/scripts/run_guard_selftest.py`。本次改动只新增注释/文档字符串，未删除既有注释。

结论：4 条 KEEP，1 条 MUST KILL，1 条文档字符串删减；已全部执行。

## 已执行的删除/删减

- `scripts/extract_dcu_698_catalog.py` 的
  `"""Return the half-open line range that holds Appendix A's object tables."""`：
  私有助手，无外部调用者，内容只是重复函数名；半开区间在三个 `range(*window)`
  调用点已经可见。已删除。
- `tests/test_dcu_698_catalog_source_window.py` 模块文档字符串末句
  “These cases shift the real source and require the same catalog, then break
  each appendix/table title ...”：
  测试只破坏 `附 录 A` 与 `表A.7` 标题，未逐个覆盖其余标题；“each”与事实不符，
  已删除，由测试名表达覆盖面。
- 平移量注释由“(9451-140)”改为点名被丢的行：
  “140 blank lines push F300/F301 past the old fixed window's end, so the old
  code dropped them and still returned success.” 保留解释 140 这个魔数的必要性。

## 保留

- `scripts/extract_dcu_698_catalog.py` 常量上方注释：说明原文是抽取件、行号会整体
  平移，这是代码本身看不出的外部事实，也是按标题定位而不是按行号的理由。
- 测试模块文档字符串前两句：记录固定行窗口静默丢行的回归背景。
- `ci/scripts/run_guard_selftest.py` 登记块的注释：说明为什么平移输入必须仍得到
  同一目录、标题缺失/重复必须报错；通过字符串本身只描述结果。

## 未覆盖（审查员记录）

- 审查范围不含未提交的 `ci/mutation_guards.tsv`、计划 §12.5 行与证据目录；
  变异守卫条目在后续提交中登记并已独立复跑先红后绿。

## 审查后的验证

- `tests/test_dcu_698_catalog_source_window.py`：6/6 通过（注释改动后复跑）。
- 变异守卫 `DCU-D1-10B-fixed-line-window`：突变后测试 rc=1，恢复后 rc=0。
