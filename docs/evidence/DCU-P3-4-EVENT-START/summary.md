# DCU-P3-4-EVENT-START 证据包

恢复记录（recovery）的 start_time 必须来自本 slot 的发生记录。

- 基线：`feat/dcu` = `main` = `99318550`
- 分支：`dcu/P3-4-event-start-time`
- worktree：`/tmp/edgec-land/p3-4-event-start-time`
- 红测试提交：`4a7e94e5`（只含 `tests/test_dcu_advanced_modules.c`）
- 修复提交：`24271028`（`cmd/dcu/dcu_event_monitor.c`、`cmd/dcu/dcu_router.h`）
- 证据：本提交

## 缺陷与根因

`_logger_proc` 按事件 OI 选 `evt`，同一 OI 的所有表共用 `evt->records` 与
`evt->last_start_time`。发生（`!is_recover`）时写入 `new_rec->start_time` 并刷新
A14 `evt->last_start_time`；恢复时原来直接
`new_rec->start_time = evt->last_start_time`，于是当另一只表同 OI 发生更晚时，
恢复记录拿到另一只表的时间，`archive_event` 也把该错误起点写入 event_history。

## 修复

- `dcu_event_record_t` 增加 `uint32_t slot_id`；`_logger_proc` 创建记录时写入
  `ev->device_id`（管线的稳定 slot_id，不是档案下标）。
- 恢复时在 `evt->records[0..record_count)` 里找本 slot 最新的 **active** 记录
  （即本次恢复所关闭的发生记录），复制其 `start_time`；`archive_event` 的 start
  参数改用该时间。
- A14 `evt->last_start_time` / `last_end_time` 语义不变：每 OI 最近一次发生 /
  结束时间；发生仍刷新 `last_start_time`，恢复仍不改变 `last_start_time`。
- 记录表满 10 条回绕、本 slot 的发生记录已被更晚的 10 条记录挤出窗口时：
  A6 的发生时间是必填非空字段（`_put_required_stamp` 对 `mono_ns == 0` 直接
  拒绝），伪造起点会重蹈缺陷、回退 A14 被本任务禁止，因此**不追加恢复记录**
  （不改 `record_count`、不写 event_history），但仍更新 A14 `last_end_time`
  并打印恢复日志。此路径需同一 OI 的其它表在发生与恢复之间产生 10 条记录。

## 红证据

```
cmake --build build -j1 --target test_dcu_advanced_modules
ctest --test-dir build -R '^test_dcu_advanced_modules$' --output-on-failure
```

基线实现 + 新测试（提交 4a7e94e5 的测试）结果（`red.log`）：

```
[  ERROR   ] --- 2001000000000 != 1001000000000
[   LINE   ] --- /tmp/edgec-land/p3-4-event-start-time/tests/test_dcu_advanced_modules.c:757: error: Failure!
[  FAILED  ] test_event_recovery_start_time_is_per_slot
 1 FAILED TEST(S)
```

slot 0 在 1001s 发生、3000s 恢复；其间 slot 1 在 2001s 发生。恢复记录本应带
1001s，实际拿到 slot 1 的 2001s（`evt->last_start_time`）。断言用 `mono_ns`
（`ev->ts`，测试中完全可控），不依赖 `_wall_epoch_s()`。

## 绿证据

修复提交 `24271028` 上：

```
cmake --build build -j1 --target test_dcu_advanced_modules
ctest --test-dir build -R '^test_dcu_advanced_modules$' --output-on-failure
```

`tests.log`：`1/1 Test #19: test_dcu_advanced_modules ... Passed`，
`100% tests passed, 0 tests failed out of 1`。

```
ctest --test-dir build -R 'dcu|dlt698' --output-on-failure
```

`tests.log`：`100% tests passed, 0 tests failed out of 45`（含
`test_dcu_698_axdr` 的 A14/A6 编码测试、`test_dcu_698_object_model`、
`test_dcu_router`、两个 slot 身份测试）。

## 资源 / 栈 / 公开结构体

```
python3 ci/scripts/check_resource_discipline.py --root .
  ✅ clean: getenv_calls=0, heap_calls=175, thread_creations=0
python3 ci/scripts/check_stack_usage.py
  ✅ No new violations. 0 tracked debt item(s) remain pinned.
python3 ci/scripts/check_public_structs.py --root .
  ✅ clean: 10 public structs audited.
```

`check_public_structs.py` 只扫 `modules/*/include/**` 与 `include/**`，不覆盖
`cmd/dcu/dcu_router.h`；该守卫通过只说明公开头未受影响。`dcu_event_record_t`
新增 4 字节字段（无堆、无自动数组、无按 sizeof 的序列化依赖）。

## 改动文件

- `cmd/dcu/dcu_router.h`：`dcu_event_record_t` 增加 `slot_id`
- `cmd/dcu/dcu_event_monitor.c`：slot 发生记录查找、恢复起点取本 slot、窗口
  淘汰时的跳过语义、archive 起点改用本 slot 时间
- `tests/test_dcu_advanced_modules.c`：新增
  `test_event_recovery_start_time_is_per_slot`（两表同 OI 交错发生/恢复、
  二次发生、10 条窗口淘汰）、`test_event_archive_start_is_per_slot`、
  `test_event_recovery_evicted_occurrence_does_not_archive`
- `ci/mutation_guards.tsv`：`DCU-P3-4-event-archive-start` 变异守卫
- `docs/evidence/DCU-P3-4-EVENT-START/`：本证据包

`diffstat.txt` 由 `git diff --stat 99318550..HEAD` 生成于本文件提交之前，
因此不列 `diffstat.txt` 自身。

## 未决 / 说明

- 窗口淘汰路径按“不追加恢复记录”处理（见上）。若产品要求该恢复也必须出现在
  A6 里，需要契约层给出可空发生时间或更长的记录窗口，属于设计变更。
- `archive_event` 在窗口淘汰路径不写 event_history（起点不可知，写 0 会制造
  脏行）；A14 结束时间仍更新。
- 未改动 A14 编码路径与 `dcu_698_class_event.c`。
- 未修任务范围外缺陷。

## 8. 持久化侧守卫（对抗评审补，仅测试）

评审指出恢复路径的 `archive_event` 起点没有可执行守卫。本次只改测试与变异清单，
产品代码未动：

- `test_event_archive_start_is_per_slot`：捕获 `archive_event` 实参；两只表同 OI 的
  发生记录 `epoch_s` 分别设为 1700000000（slot0）与 1700000001（slot1），A14
  `last_start_time` 指向更晚的 slot1；推 slot0 恢复，断言 archive 起点等于
  slot0 的发生 1700000000。把 archive 起点回退成 `evt->last_start_time.epoch_s`
  时测试红（`1700000001 != 1700000000`，`red-archive.log`），恢复后绿
  （`green-archive.log`）。
- `test_event_recovery_evicted_occurrence_does_not_archive`：10 条记录窗口全为
  slot1，slot0 的发生记录已被挤出；推 slot0 恢复后断言 archive sink 调用 0 次、
  `record_count` 不变、`last_end_time` 仍更新。
- 变异守卫 `DCU-P3-4-event-archive-start` 已登记 `ci/mutation_guards.tsv`
  （find/replace archive 起点实参）。`run_mutation_guards.py --build build --only
  DCU-P3-4-event-archive-start`：mutated 红（rc=8），restored 绿（rc=0），原文件
  字节与 hash 校验通过，见 `mutation-archive.log`。
- 回归：`ctest -R 'dlt698|dcu'` 45/45 通过（`tests.log`）。

## 状态

未合并、未 push。未运行 `./acceptance/run.sh`（由 verifier 串行验收）。
