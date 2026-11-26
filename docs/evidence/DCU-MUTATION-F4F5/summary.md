# DCU F4/F5 不变量测试的变异敏感守护

分支 `dcu/mutation-guards-invariants`，worktree `/tmp/edgec-land/mutation-guards`，
基线 `feat/dcu 197d98b9`。未合并，等 dcu-verifier 验证。

## 1. 目标与改动

AGENTS「关键不变量测试必须对真实缺陷变异变红」把 `ci/scripts/run_mutation_guards.py`
与 `ci/mutation_guards.tsv` 定为执行者，但 manifest 里此前没有 F4/F5 的四条新不变量
测试。本单只补登记，不改产品代码，也不改既有测试：变异由 runner 瞬时施加并在
`finally` 里按 hash 恢复。

| manifest 条目 | 测试（`CMOCKA_TEST_FILTER`） | 变异（守卫改回缺陷行为） | 修复提交 |
| --- | --- | --- | --- |
| `DCU-F4-batch-continuation` | `test_pending_capacity_starves_second_task` | `_dcu_chain_pump` 一批耗尽后不再经 `_chain_begin_visit` 续批 | `8c7f0ec7` |
| `DCU-F4-served-ident` | `test_scheme_change_mid_batch_reopens_execution` | 执行身份变化时不清 `served` 位图，旧方案下标按新方案解释 | `815ab262` |
| `DCU-F5-pump-revision-fence` | `test_late_callback_before_revision_observed` | 去掉 pump 入口 `inv->revision != last_inv_rev` 栅栏，迟到回调续发旧访问剩余项 | `f26118c6` |
| `DCU-F5-spare-retry-revision` | `test_spare_retry_dropped_after_slot_reuse` | 去掉 `_on_done` 的 `c->revision == bs->inv->revision`，slot 回收后备用 DI 重试落新表 | `f26118c6` |

`test_cmd` 用 `CMOCKA_TEST_FILTER=<该条测试>` 把红/绿锁到条目自己的不变量测试。
不筛用例时同二进制任一用例失败都会满足守护，红无法归因到目标测试。

## 2. 数据形状与被拒绝的替代

一行 = 一个历史缺陷 + 一条不变量测试。`find` 必须在全文件唯一、施加后能构建；
runner 要求源文件干净，变异后测试必须红，恢复后必须绿。

- 拒绝：为每条写独立脚本。runner 已实现 manifest 解析、单次替换、构建、红/绿判定、
  hash 恢复与信号处理；另写脚本只会复制这些语义并制造漂移面。
- 拒绝：`test_cmd` 跑整个 `test_dcu_poller`。四条共用同一二进制，整跑时红绿没有
  归因；`CMOCKA_TEST_FILTER` 是 cmocka 自带能力，无新依赖。
- 拒绝：把变异提交进产品源码或 `#ifdef` 开关。变异必须是瞬时、可证明恢复的；
  `#ifdef` 会留在产品里成为第二套行为。
- 拒绝：扩 runner 判定逻辑或 manifest 格式。四条都能用现有列表达。

## 3. 根因与改动文件

- F4 容量：`_chain_load_pending` 每批最多 `MAX_SCHEME_OADS=32` 项；一批耗尽后泵必须
  重入 `_chain_begin_visit` 续批。旧行为直接结束访问，第二任务 tag 静默丢失且任务
  按失败结算。
- F4 方案身份：`served` 位图按方案下标解释；6012 Add 换方案走 `upsert_task`，执行
  身份 `{exec_gen, scheme_id, item_mask}` 变化时必须清 `served` 重载。旧行为沿用旧
  位图，新方案部分 tag 漏抄、部分重复读。
- F5 revision 栅栏：一次访问属于一个档案 revision。`_dcu_chain_pump` 入口在
  `inv->revision != last_inv_rev` 时必须停发，等下一次 step 重开；旧行为让迟到回调
  按移位后的下标续发。
- F5 slot 复用重试：备用 DI 重试只认提交时的 revision。revision 不变则下标仍是同一
  只表；变了就放弃。旧行为按当前下标读，slot 回收后重试落到继承 slot 的新表。

改动文件：`ci/mutation_guards.tsv`（新增 4 行）；`docs/evidence/DCU-MUTATION-F4F5/`
（本证据）。产品与测试源码零改动。

## 4. 红先绿后（原始输出）

```
python3 ci/scripts/run_mutation_guards.py --build build \
  --only DCU-F4-batch-continuation,DCU-F4-served-ident,\
DCU-F5-pump-revision-fence,DCU-F5-spare-retry-revision
```

结果 `4/4 guards sensitive, build refreshed`（exit 0）。每条都是
`[mutated] test rc=8`、`[restored] test rc=0`，源文件按 hash 恢复。原文见
`mutation-run.log` 前半。

补充的红/绿原文（同一 manifest 变异，直接看目标用例的 cmocka 断言）：

| 条目 | 变异后失败断言 | 恢复后 |
| --- | --- | --- |
| `DCU-F4-batch-continuation` | `test_dcu_poller.c:500: 32 != 64`（提交数只剩一批） | rc=0 |
| `DCU-F4-served-ident` | `test_dcu_poller.c:1496: 16 != 0`（新方案 16 个 tag 漏抄） | rc=0 |
| `DCU-F5-pump-revision-fence` | `test_dcu_poller.c:1228: 2 != 1`（迟到回调续发一帧） | rc=0 |
| `DCU-F5-spare-retry-revision` | `test_dcu_poller.c:1373: 2 != 1`（重试读继承 slot 的新表，并打 `[SRC-ERR] spare-retry`） | rc=0 |

未变异基线 `./build/tests/test_dcu_poller`：34/34 通过。

## 5. 目标命令与结果

| 命令 | 结果 |
| --- | --- |
| `python3 ci/scripts/run_mutation_guards.py --selftest` | exit 0，`mutation runner decision logic verified` |
| `python3 ci/scripts/run_mutation_guards.py --build build --only <4 条>` | exit 0，4/4 敏感 |
| `./build/tests/test_dcu_poller` | 34/34 通过 |
| `git status --porcelain --untracked-files=no`（runner 后） | 仅 manifest 变更；产品源干净 |

构建配置：`cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
-DENABLE_WEB_BUILD=OFF -DCMAKE_C_COMPILER_LAUNCHER=ccache`。本单只构建 native
测试目标；CI 用完整配置，manifest 不依赖 web 产物。

## 6. 未决

- 未跑全量 `ci/scripts/run_arch_gates.sh`（时间盒；本单范围是 manifest 与 runner
  selftest，已覆盖）。
- 只登记任务指定的四条。F4/F5 其他回归（如
  `test_late_callback_shift_does_not_fail_meter`、`test_spare_retry_allowed_when_revision_unchanged`）
  未登记；是否补由 verifier 决定。
- `test_dcu_poller` 其余用例对既有变异的敏感性未评估，不在本单范围。
