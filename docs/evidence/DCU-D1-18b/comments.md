# DCU-D1-18b 注释审计

审计范围：`b6af6b3d` 到工作树的 diff，文件 `cmd/dcu/dcu_698_object_model.c`、`cmd/dcu/dcu_router.h`、三个测试文件、Class 24 设计段。

## 删除

1. `tests/test_dcu_698_value_table.c`：删除 `/* 3000/3005 的 A2/A3/A5-A10/A14 未暴露：不得有标量替代冒充属性值。 */`。下方 `removed[]` 已列出属性号，注释里的 `A5-A10` 还多算了未断言的 A7，且“不得有标量替代冒充属性值”是说明设计意图而非代码事实。

## 保留（标准条款与协议规则，代码看不出）

- `cmd/dcu/dcu_698_object_model.c:444`：7.3.24 表193 的属性号到标准名映射。
- `cmd/dcu/dcu_router.h:83-91`：字段对应的 Class 24 属性号与未暴露状态。
- `tests/test_dcu_advanced_modules.c:272`：记录表在 A6-A9、A2 是关联对象属性表，属于协议规则。

## 附带修正

同一结构体的 `@brief` 仍写 “Class 7 异常事件监控对象结构体”；字段注释已改为 Class 24，标题同步为 “分项事件对象存储（Class 24，3000/3005）”。

## 第二轮（rebase 到 8acf47fe 后的新增代码）

删除 1 处：`dcu_698_object_model.c` 类上限表的 “与 tests/fixtures/dcu_698_catalog_expected.h 的类表一致” 从句。一致性已由 `test_class_attr_limit_matches_catalog` 确定性对拍，注释里的同一句话会随目录变化漂移。保留 “附录 A 的类定义上限。”。

保留（协议规则）：`_oi_event_read` 的 7.3.24/表193 映射、`dcu_698_var.c` 的 DAR 5/4 分级、`dcu_router.h` 的字段属性号、`test_dcu_advanced_modules.c` 的 A6-A9/A2 说明。

## 未处理（超出本 ticket 文件面）

`cmd/dcu/dcu_698_var.c` 的 `k_scalars[]` 仍保留 3000/3005 的 5、6、8、9 旧行。它们现在只会命中 mask 检查后返回 NOT_FOUND，不产生假值；清理由 D1-18a 一并完成。A5 打开前必须删除 `{3005,5,VK_U16}`，否则结构写入会越过 2 字节调用方缓冲。
