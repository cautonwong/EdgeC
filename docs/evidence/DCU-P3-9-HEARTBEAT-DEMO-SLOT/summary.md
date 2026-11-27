# DCU-P3-9-HEARTBEAT-DEMO-SLOT：演示遥控按档案第一只表解析稳定 slot

分支 `dcu/P3-9-heartbeat-demo-slot`，工作树 `/tmp/edgec-land/p3-9-heartbeat-demo-slot`，
基线 `99318550`（= `feat/dcu` = `main`）。**未合并、未 push。**

## 问题与根因

`dcu_heartbeat_control_tick` 在 demo 模式下把 `0` 写死传给
`dcu_698_control_legacy_action`，而 slot 0 属于档案里排序最前的另一只表；
删除该表后 `dcu_inventory_by_slot(slot 0)` 返回 NULL，legacy action 以
`EDGE_SLAVE_SRC_NOT_FOUND` 拒绝，演示遥控一次也不出门。

第一版修复（`67f1f929`）按任务原文把目标写死为通信地址 `"10000001"`。
验收台架（acceptance/dcu `bench_meter_01`）的档案地址是 `000000000001`，
查 `10000001` 必然落空并跳过，`test_dcu_sends_trip_control_frame` 断言
“台架一帧 645 控制命令都没收到”而红。故退回重做。

## 最终改动

- `cmd/dcu/dcu_heartbeat.c`：`dcu_heartbeat_control_tick` 取
  `dcu_node_get_inventory(hb->node)->meters[0]`（档案第一只表），用其稳定
  `production_meter_t.slot_id` 调 `dcu_698_control_legacy_action(...)`；
  日志打印解析到的地址；档案为空（`inv == NULL || count == 0`）时
  `LOG_WARN` 后跳过，绝不调用 slot 0。不写死任何地址。
- 生产路径（非 demo）一行未改：`dcu_heartbeat_tick` 与生产分支的
  刷盘/日志行为保持原样。

## 测试（先红后绿）

红（commit `64b027db`，只含测试；原始输出见 `red.log`）：在回收
`10000001` 修复后、新实现前的代码上，`ctest --test-dir build -R
'^test_dcu_runtime_health$' --output-on-failure` 失败：

- `test_demo_control_targets_first_meter_slot`：`rec.calls` 0 != 1
  （`tests/test_dcu_runtime_health.c:361`），control sink 一次都没被调用，
  与验收台架“跳闸帧根本没发”一致。

基线红（原始缺陷代码，见 `red-baseline.log`）：把 `dcu_heartbeat.c` 临时回到
`99318550` 版本重建，同一用例同样在 `:361` 变红（旧码把 0 传给 legacy
action，删除 slot 0 后 NOT_FOUND，sink 0 次），证明该用例对原始缺陷也变红。

用例用 `dcu_router_set_control_sink` 注入记录型 sink，观察
`dcu_698_control_legacy_action` 经 `dcu_698_internal_control_sink` 拿到的
实际目标地址/OI/动作。档案用台架类地址 `000000000001`、`000000000002`，
删除 slot 0 后第一只表为 `000000000002`（slot_id = 1），与 slot 0 分叉。

绿（commit `8fcd469c`）：

- `ctest --test-dir build -R '^test_dcu_runtime_health$' -V`：
  6/6 子用例通过（含原有 `test_demo_freeze_trigger_uses_slot_identity`、
  `test_demo_mode_restores_bench_behavior`），1/1 ctest 通过；
- `ctest --test-dir build -R 'dlt698|dcu' --output-on-failure`：
  45/45 通过，0 失败（73.37s）。

验收（commit `8fcd469c`，`TESTPLATFORM=/home/edge/test-platform
PYTHON=/home/edge/EdgeC/.venv/bin/python3 EDGEC_BUILD_DIR=$PWD/build
./acceptance/run.sh dcu`），连续 3 轮退出码 0：

- `acceptance-run-1.log`：`全部合格:2 个验收任务`，3 passed + 35 passed；
- `acceptance-run-2.log`：同上，退出码 0；
- `acceptance-run-3.log`：同上，退出码 0。

## 证据文件

- `red.log`：新实现在 `10000001` 版本代码上的失败原文；
- `red-baseline.log`：原始缺陷代码（99318550）上的失败原文；
- `tests.log`：修复后 `dlt698|dcu` 子集（45 用例）原始输出；
- `acceptance-run-{1,2,3}.log`：验收 3 连原始输出；
- `diffstat.txt`：`git diff --stat 99318550..HEAD`；
- `comments.md`：注释清单。

## 未决

- 建议为 control tick 登记 `ci/mutation_guards.tsv` 条目（本单未改守卫清单）：
  变异 `dcu_698_control_legacy_action(meter->slot_id, 1)` →
  `dcu_698_control_legacy_action(0, 1)`，用
  `CMOCKA_TEST_FILTER=test_demo_control_targets_first_meter_slot` 定向跑
  `test_dcu_runtime_health`，必须变红。现有 `DCU-F5-heartbeat-slot`
  只变异冻结路径 `trigger.device_id`，不覆盖本路径。
- 目标定义为“档案第一只表”，与 demo 冻结管线遍历的档案一致；若产品要求
  演示遥控固定某一台表（而非第一只），需要档案层提供选择依据，属设计变更。
- `dcu_698_control_legacy_action` 仍按 698 对象模型的全局档案解析 slot；
  单节点装配下与 `hb->node` 档案一致，多节点场景不在本单范围。
