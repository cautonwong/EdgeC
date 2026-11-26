# DCU F5R：事件与记录读取的设备身份

分支 `dcu/F5-device-identity`，合并 tip `9635814b`（merge `9ab59c23`），基线 `feat/dcu 0066b073`。

## 问题与根因

`pipe_msg_t.device_id` 与 `dcu_node_record_reading` 的参数在不同站点被两种语义解释：shadow/publish/feed 写稳定 `slot_id`，事件监控的地址打印、恢复分支、冻结路径和 record reading 却按档案下标使用。`dcu_inventory_remove` 用末元素覆盖删除位，slot 跟表走、下标会移位；删除后事件、冻结断面或读数会落到顶替下标的表。

## 数据形状

- `slot_id` 是稳定身份，范围 `[0, MAX_METERS)`，删除重排时随表整体搬移；影子表、telemetry、publish/feed 都用它。
- 档案下标只在同一 revision 的同步操作内有效；跨模块字段与落库身份只用 slot。
- 所有消费站点在边界用 `dcu_inventory_by_slot` 解析一次，内部信任 slot。

拒绝的替代：保留 record reading 的下标语义并在入口反查 slot；给 `pipe_msg_t` 加第二个 slot 字段；在 C 里造 slot 包装类型。均因重复真相、改只读 foundation 或把歧义重新引入聚合根边界而拒绝。

## 改动

- `cmd/dcu/dcu_event_monitor.c`：`_monitor_proc`、`_logger_proc` 的发生/恢复、`_freeze_proc` 统一按 slot 解析地址与影子抓拍；冻结 slot 不存在改为 DROP。
- `cmd/dcu/dcu_node.c/.h`：`dcu_node_record_reading` 参数改为 `slot_id`，用 `by_slot` 进件，落库 address 与 device_id 来自同一只表；端口注释更新。
- 测试：`test_dcu_advanced_modules` 新增事件/冻结移位回归；`test_dcu_node` 新增 record reading 移位回归。
- 提交：`4fd7c9f4` 测试、`ad44426a` 修复、`39cc8d26` 证据、`9635814b` ASAN stage 描述符泄漏修复。

## 验证

- 红 `test_dcu_node` 3/4、`test_dcu_advanced_modules` 1/2 → 绿 4/4、2/2；45/45。
- coverage 验收 3 连绿，2/2 任务，35 passed，22.75/31.94/22.57s。
- 合并树守卫、全量 ctest 367/367、fanalyzer 3 known/0 new、warning 0；ASAN 目标集无泄漏。

## 未决

- heartbeat 演示模式下标、`dcu_698_var.c` 事件快照元素序号、slot 复用状态，记 §12.5。
- 冻结对不存在 slot 由 Unknown 改为 DROP；生产 stage 一律带 inv，无可达差异。
