# DCU-D5-03 注释审计

范围：`git diff f25d390f` 的代码、测试与设计文档。审查者 `pstack:comment-sicko`（report-only，
独立子代理）返回 4 处删除、3 处保留、2 处事实错误；本文件记录已接受的删除与修正。

## 删除（4 处，5 行）

1. `cmd/dcu/dcu_698_object_model.c` 的 7000/7010 行块注释（2 行）：复述 A.8 方法范围与
   空掩码/DAR 5 惯用法（Class 14/15/16/17 行注释与 `test_class11_collection_rows_are_empty`
   已承载），且把 D4-06（Class 18 文件传输）误写成脚本集合存储的 owner；也漏掉类未定义
   ID 回 DAR 4 的分支。已删。
2. `tests/test_dcu_698_axdr.c` 的“类 11 定义 A1..A4；7000 实例不广告属性，7010 只广告 A2”：
   “只广告 A2”把 ACL 层混进 resolver 测试（两行对象掩码都为空，测试也绕过 ACL），并且
   两个 OI 的 A1..A4/A5 期望完全相同，说明不了任何差异。已删。
3. `tests/test_dcu_698_axdr.c` 的“A.8：7000 方法 127–129，7010 方法 127–130”：逐字复述
   下方循环边界。已删。
4. `tests/test_dcu_698_object_model.c` 的“A.8 的实例方法：标准定义过但无处理器 → DAR 5；
   类上限外 → DAR 4”：机制写错（类 11 类上限是 2，127–129 也在类上限外却回 DAR 5；
   DAR 4 来自 130/131 不在 OI 专属 `k_oi_max_method`），其余复述断言。已删。

## 保留（3 处，3 行）

- `cmd/dcu/dcu_698_object_model.c` 的 `k_oi_max_method` 注释：A.12 与 A.8 是 127+ OI 表的
  外部标准来源；OI 常量无法表达附录号与“类表 vs OI 表”的分层。
- `cmd/dcu/dcu_router.h` 的 `OI_FILE_SET 0x7000` 行尾注释：OI → A.8 / IC 11 是 DL/T 698.45
  的公开宏契约，标识符本身承载不了附录与接口类。
- `cmd/dcu/dcu_router.h` 的 `OI_SCRIPT_SET 0x7010` 行尾注释：同上。

## 事实修正（审计发现，已改）

- `docs/design/DCU_698_OBJECT_MODEL.md` §8.8 的 DAR 5 列表漏了 7010 方法 127–130；已补。
- 同文件 §7 Class 11 行把 7010 A2 与方法都写成“resolver 层”；GET 走
  `dcu_north_resolve_axdr`，ACTION 走 `dcu_698_check_call`，已分开表述。
- `docs/evidence/DCU-D5-03/summary.md` 把“没有文件/脚本集合存储”归因于 D4-06；D4-06 只是
  Class 18 文件传输，已改为“D4-06 仅覆盖 Class 18 文件传输且未做”。
- 本文件原稿称“删除：无”并保留了上述错误注释；本稿覆盖之。

## 核对（审计确认准确）

- A.8：7000 只有方法 127–129、无属性；7010 有 A2 array 脚本与方法 127–130。代码
  `k_methods_7000`、`k_attrs_7010`、`k_methods_7010`、`k_oi_max_method` 逐项一致。
- 7.3.11：Class 11 A1–A4、方法 1–2；`k_class_max_attr[11]=4`、`k_class_max_method[11]=2`。
- 对象表 105 行（94 字面 + 11 `DCU_698_PARAM_OIS`）；ACL 91（80 字面 + 11 生成）。
- 对象列表 3211 B = 3141（89 项）+ 29（7000）+ 41（7010）；runtime.log 实测 len=3211。
- DAR 映射：CLASS_MISMATCH→5、UNDEFINED→4、NOT_FOUND→6、DENIED→3。

## MUST KILL

无。空掩码 → 类定义 DAR 5、未定义 DAR 4 的不变量已由可执行测试钉住；没有源符号依赖注释
才能正确。`_class_defines_method` 的两套表是既有结构，不在本次注释 diff 范围。

## 跳过

- 未审计 touched 文件中的既有注释（Class 8/14/15/16/17 块、掩码注释），只作上下文。
- 未复核 Class 11 行既有部分（6014..6034 的 IC、6032 A2 DAR 2、6000 A.7 偏差）。
- 未重跑测试；依赖已入库的 45/45 证据与代码阅读。

## 行为核对

删除与文档修正均为说明性文字，无行为变化；修正后 `ctest -R 'dcu|dlt698'` 仍 45/45。
