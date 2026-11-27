# DCU-P3-1-ACL-CAP 注释记录

/no-comments 口径：本单不新增代码注释。

行为变更导致既有注释失真，按 AGENTS.md「行为变更必须同步同文件注释」只做订正：

- `cmd/dcu/dcu_698_appconn.c`：Class 19/21/23/18 的“每对象方法槽 8 / 属性 schema 16，
  故记 HOLD”说明改为新的广告范围；Class 12 的“ACL schema 只到 16”改为
  “16/17 归属冲突、18/19 未裁决”的实际 HOLD 理由。
- `adapters/io_drivers/src/driver_dlt698.c`：属性权限 32 位/16 属性的说明改为
  5 字节/1..18；方法数组改为会话池后的既有说明同步。
- `tests/test_dcu_698_appconn.c`、`tests/test_driver_dlt698.c`：HOLD 断言反转为
  已广告/已可达断言，随断言同源的说明同步订正。
- `tests/test_driver_dlt698.c` 池界哨兵用例新增一行注释，说明“第一只对象广告
  属性 2”是为了让池满拒绝与接受两种路径在响应上有差别。

无新增“why”注释；无删除既有 why 注释。
