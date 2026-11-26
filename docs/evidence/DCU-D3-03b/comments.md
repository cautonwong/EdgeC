# D3-03b Comment Sicko 结果

删除 14 处注释（9 行独立注释、5 处行内），2 处公共契约被纠正而非删除：

- 纠正 `dcu_698_freeze.h` tick 契约：补上“先处理到期延时触发，再触发标准边界”。
- 纠正 `dcu_698_freeze_cfg.h` 默认集契约：删掉“深度只持久化/广告”的过时句（depth 已用于裁剪）。

保留的注释都带不可由代码表达的约束：标准 2000-01-01 基准与周期 0 语义、OI 块 5000–5007、
`last_fire` 二维索引契约、`dcu_freeze_schedule` 入池/池满/uv 驱动契约、`dcu_storage_trim_freeze`
的“同一写入事务内、按三键保留最新 N”契约。

## MUST KILL 处理

1. `_apply` 容量拒绝回 DAR 3 而非 DAR 8：不接受。派发明令“DAR 8 在 foundation 头无映射且
   `edge_slave_plane.h` 不在 core_readonly_whitelist，不得改，回 DAR 3”，测试钉住 DAR 3；
   该分歧已写进 summary §6。
2. `dcu_freeze_schedule` 的 delay=0：接受并修复。0 现在在 `dcu_freeze_schedule` 内直接调用
   `dcu_freeze_trigger`，`_apply` 不再预分支；标准 0 = 立即，不再回 DAR 7。
3. `dcu_freeze_next_boundary` 的“0 = 无”哨兵对合法参数不可达：Sicko 标为 skip（未改行）；
   保持原样，避免扩大 diff。

## 验证

注释删除后重建并重跑 `test_dcu_698_freeze`（9/9）；delay=0 修复后再次 ctest/ASAN/验收，结果见
`tests.log`、`runtime.log`。
