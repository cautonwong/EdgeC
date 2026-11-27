# comments

本任务遵守 `/no-comments`：`scripts/extract_dcu_698_catalog.py` 未新增代码注释。

新增的两个模块级常量需要 why 说明，按要求记在这里而不是代码里：

- `CLASS_METHODS`：26 个 7.3 类各自真实的方法号集合。它是逐号校验的参照，
  不是生成物的一部分。标准本身有空号：Class 7/24 无方法 3，Class 9 无方法 6，
  所以不能用“1..max 连续”规则；集合已用独立于解析器的原文扫描复核
  （26/26 一致，见 `design.md` 与 `tests.log` 的测试证据）。原文合法修订时
  需要人工同步这张表，和 `OI_CLASS_DECISIONS` 一样是记录下来的标准事实。
- `OI_METHOD_EXCEPTIONS`：`{"F001": {9, 10}}`。表 A.11 给 F001（Class 18）
  在 127 以下定义了方法 9（读文件）和 10（软件比对），7.3.18 类表没有它们；
  产品侧 `cmd/dcu/dcu_698_object_model.c::_class_defines_method` 已有同一例外。
  没有这个例外，真实输入的 per-OI 校验会误红。

未删除任何既有 why 注释；新增测试文件中的 docstring 只说明被守住的缺陷，
不是对实现的解释。
