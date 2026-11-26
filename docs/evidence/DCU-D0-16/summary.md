# DCU-D0-16：修复任务监控成功计数的 tick 相位缺陷

状态：交 `dcu-verifier` 复核。基线 `feat/dcu c71bcba7`，已普通 merge `feat/dcu fcead39c`；
分支 `dcu/D0-16-monitor-count`。

## 1. 根因

`cmd/dcu/dcu_poller.c::_chain_finish_visit` 在成功结算时只对
`poller->task_due[t]` 为真的任务 `monitor.success_meters++`。`task_due` 只在任务周期到的
那个 pump tick 为真，而异步读回调可跨多个 tick 完成；落在非到期 tick 的成功访问被丢弃。
实测同一次执行 `total=40`、`status=2` 完成时 success 只有 2/20/30，恢复后也可卡在 30
（D0-15 monitor_repro.log / monitor_recovery_stuck.log）。

失败路径审计：monitor 没有独立失败计数；失败只更新通道统计与 `consecutive_fails`，并从
`meters_left` 的首次访问扣减（`_task_note_visit`，与成功无关），所以不存在对称的丢失计数。
`sent/recv` 在 `note_tx/note_rx` 按执行持续累计，不受 `task_due` 影响。

## 2. 修法（根因）

`_task_note_visit` 已经把“本次访问属于哪些任务的当前执行”写进
`c->visit_new_mask`。`_chain_finish_visit` 改为：

- 仅当任务 `monitor.status == 1`（执行进行中）、任务状态 NORMAL、表级 mask 允许、且
  `visit_new_mask` 含该任务时计数；
- 删除 `task_due[t]` 瞬时判断。

这样每个表每次执行最多计一次成功（visited 位 + mask），失败访问不计数；重试/备用 DI 仍是
同一次访问，不会多计。执行完成仍由 `meters_left==0 && outstanding==0 && !pending` 决定，
`total` 每次 `_task_begin` 重置；档案变更重开清空 success 与 mask，语义与 D2-08 一致。
6034/6035 的线上字段与含义未变。

## 3. 回归测试

`tests/test_dcu_poller.c::test_success_counts_across_non_due_tick`（新增）：

- 两块表分别在通道 1/2，任务周期 10s，异步提交：到期 tick 提交 2 项，`success=0`；
- 下一 tick 非到期，此时分别回调成功与失败；
- 断言 `success_meters==1`、`total==2`、`success<=total`；旧代码此处为 0。

保留原 `test_success_counts_only_due_enabled_tasks`（禁用任务/TI=0 任务仍不计成功）、
1m2tag、档案变更重开、submit 失败等 D2-08 用例；poller 测试 10/10。

## 4. 验证

- clean Release `--clean-first` build：`check_build_warnings.py` 0 new violations。
- `ctest -R 'dcu|poller|dlt698'` 44/44。
- ASAN `detect_leaks=1`（poller/polling_loop）2/2 零泄漏。
- SRP 干净；guard 110/110。
- 合并 feat/dcu 后重建、ctest 再跑 44/44；验收 `acceptance/run.sh dcu` 2/2，3 + 34 passed /
  66.11s，无 DCU 残留（task-monitor 用例在 60s 平台预算内通过）。

## 5. 未决项

- D0-15 的测试改写（按 start 变化等新执行、去掉 30s+90s 固定窗口）在 D0-16 合入后另行提交，
  本单不改验收测试。
- 失败计数没有独立 monitor 字段；6035 仍只暴露成功数，符合 D2-08 已定线上语义。
