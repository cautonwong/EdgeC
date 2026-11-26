# DCU F5-HEARTBEAT 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/F5-heartbeat-slot`，merge `7dbbeb54`（tip `2c57aff2`），基线 `feat/dcu 77dd8e1e`。

## 结论

VERIFIED。DCU_DEMO_MODE 的心跳演示冻结与演示跳闸都改用 `inv->meters[i].slot_id`，不再把循环下标当 `device_id`；`dcu_inventory_remove` 的交换删除造成 slot 与下标分离后，冻结断面与跳闸仍归正确的表。修复为两处替换 + 一条 why 注释；同时登记变异守卫 `DCU-F5-heartbeat-slot`（把 `slot` 改回 `(uint32_t)i` 时新用例转红）。

## 红先绿后（独立复跑）

- 红：测试 commit `e242de07` 在旧码上 `test_demo_freeze_trigger_uses_slot_identity` 失败 `0 != 3`（删 slot 0 后错表/丢冻结），4 例中 1 红，原文 `verifier/red-test.log`。
- 绿：tip `2c57aff2` 同用例转绿，`test_dcu_runtime_health` 4/4、`ctest -R 'dcu|dlt698'` 45/45，原文 `verifier/green-test.log`、`verifier/green-ctest.log`。

## 验收（worktree `build`，Release）

5 轮 4 绿：run1/3/4/5 均 2/2 任务、3+35 passed（7.70/17.82、10.56/18.30、7.25/22.43、14.23/17.41s）。run2 的 `test_master_reads_midnight_freeze_with_test_clock` 报零点对齐差 208ms（要求 <50ms），当时 F5S 合并树全量 ctest `-j2` 正在同机运行，归因调度争用；随后连续 3 轮绿。原文 `verifier/accept-*.log`。

## 合并树门禁（`7dbbeb54`）

- 合并树 build 0 第一方 warning、守卫自检全绿、变异守卫 `DCU-F5-heartbeat-slot` 1/1 sensitive、全量 ctest 367/367；ASAN `test_dcu_runtime_health`、`test_dcu_advanced_modules` 绿，无 sanitizer 报告；fanalyzer 3 known/0 new。原文与 D4-10 合并树门禁同批保存（`docs/evidence/DCU-D4-10/verifier/merged-*`、`verifier/asan-*`、`verifier/fanalyzer.log`）。

## 备注

- 演示控制 tick 里 `dcu_698_control_legacy_action(0, 1)` 仍写死 slot 0，删档后演示跳闸可能 NOT_FOUND；demo-only、与本缺陷无关，保留。
- 本地构建 `ENABLE_WEB_BUILD=OFF`（纯 C 改动），ASAN 由合并树补齐。
