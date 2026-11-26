# DCU F5R 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/F5-device-identity`，合并 tip `9635814b`（feat/dcu merge `9ab59c23`），基线 `feat/dcu 0066b073`。

## 结论

VERIFIED。事件监控、事件日志、冻结与记录读取统一用稳定 `slot_id`，经 `dcu_inventory_by_slot` 解析。档案删除重排后，事件、冻结断面或读数不再落到顶替其下标的表。`dcu_node_record_reading` 端口参数改为 `slot_id`。

## 红先绿后（独立复跑）

- 测试 commit `4fd7c9f4`：`test_dcu_node` 3/4（`test_dcu_node_record_reading_uses_slot_id` 红），`test_dcu_advanced_modules` 1/2（`test_event_and_freeze_keep_slot_identity_after_archive_shift` 红）。原文 `verifier/red-node.log`、`red-adv.log`。
- 修复 tip `39cc8d26`：4/4、2/2，`ctest -R 'dcu|dlt698'` 45/45。原文 `verifier/green-node.log`、`green-adv.log`、`broad.log`。

## 验收（COVERAGE=ON）

3 连绿，2/2 任务，35 passed，22.75/31.94/22.57s。原文 `verifier/accept-{1,2,3}.log`。

## 合并树门禁

- 守卫自检全绿；全量 ctest 367/367，61.22s；fanalyzer 3 known / 0 new；warning 0（F4R/F5R 两份 clean build 日志）。原文 `verifier/merged-ctest.log`、`merged-fanalyzer.log`、`warn-*.log`。
- ASAN 目标集发现 `test_dcu_advanced_modules` 泄漏 6 个 stage 描述符（新旧用例各 3 个）。修复 `9635814b` 在 `pipeline_add_stage` 后 `free` 临时描述符；ASAN 2/2、无 LeakSanitizer，Release 2/2。原文 `verifier/leakfix-asan.log`、`leakfix-release.log` 与证据 `asan-leak-fix.log`。

## 语义变化

- 冻结在 `inv` 为空或 slot 不存在时由“归档到 Unknown”改为 `STAGE_DROP`。生产 stage 一律带 inv，无可达差异；测试覆盖。
- `dcu_node_record_reading` 目前无生产调用者；端口契约改为 slot_id 是为未来进件定稳定身份。

## 已知限制

- `dcu_heartbeat.c:72` 演示模式仍把循环下标写进 `trigger.device_id`；仅 DCU_DEMO_MODE，生产不走该管线，记 §12.5。
- `dcu_698_var.c::_read_element` 把 OAD 低字节当 `device_id` 传给 `read_shadow`，事件快照不跟表；新测试把 `snapshot_count` 置 0 隔离，记 §12.5 另立。
- slot 在 4096 次分配后回收，`_monitor_proc` 的按 slot 状态数组会继承上一只表的越限计时；既有行为，未改。
