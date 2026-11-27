# comments

本任务遵守 `/no-comments`：`cmd/dcu/dcu_heartbeat.c` 未新增代码注释，也没有
需要以一行注释解释的 why。

`tests/test_dcu_runtime_health.c` 新增两个测试注释，均为夹具必需说明：

- `test_demo_control_targets_first_meter_slot` 上：“删除 slot 0：000000000002
  成为档案第一只表，slot_id = 1，slot 与下标分叉。”
- `test_demo_control_skips_empty_inventory` 上：“空档案时跳过遥控，绝不回落到
  slot 0。”

未删除任何既有 why 注释；`dcu_heartbeat.c` 中被替换的旧实现不携带注释。
