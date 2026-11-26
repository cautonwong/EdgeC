# DCU-D0-18：/no-comments（Comment Sicko）

范围：`git diff a19f3edb`（含工作树），4 个文件。

## 接受并处理

- `cmd/dcu/dcu_poller.c`：装载注释压成一行（执行已开始就补齐、每表每执行一次装载）；
  删除“装载之后再记访问”顺序说明、成功归属的三行说教（保留一行表级语义）、
  游标夹回的复述注释；`_chain_load_pending` 上方按 `task_due` 选集的旧说明改为
  “执行中任务的项取并集”。
- `cmd/dcu/dcu_poller.h`：生命周期段落压成两行（visited/succeeded 每执行清、
  档案重开清、表级 success 与 success<=total），删除 ticket 名；位图行注压短；
  S3 历史注里的“到期任务”改为“执行中任务”；`pending_masks` 行注去掉“到期”。
- `tests/test_dcu_poller.c`：删除 dedup 用例中复述断言的注释；新用例头注从“旧实现
  task_due…”改为意图（一次执行只抄一遍，下一次执行才重来）。

## 拒绝（report 的 reshape flags，超出本单且需结构改动）

- `visited`/`succeeded` 抽 `meter_set_t`、折叠 `_chain_load_pending` 与
  `_task_note_visit`、把访问身份传入 `_on_read_done`：属结构重构，与本次根因修复
  分开；在证据 §6 记录为后续任务。
- “explicit per-visit identity” 指出的 **depth>1 潜在误计**：当前 `chain.depth`
  默认 1（`dcu_poller.c` 初始化），`meter_idx` 在回调期间不被改写；depth 可调是注释
  里写明的非常规路径。记录为已知风险，待需要 depth>1 时先修身份传递。
- `success<=total` 改运行时断言：属于新增不变量检查，与 D0-19/D2 语义收敛一起做；
  单测已覆盖（`test_archive_change_reopens_execution`、新用例）。

## 保留（有证明）

- `dcu_poller.h` 的 6035 `@brief` 与两行生命周期：verifier 明确要求把 succeeded/
  visited 生命周期写进代码/文档；这是唯一说明“每执行清、档案重开清、旧回调不计”的位置。
- `cmd/dcu/dcu_poller.c` 的一行表级成功语义：多任务重叠时“任一 tag 成功即算该表”
  是 D0-16 既定语义，dedup 用例已断言两任务各计一次。
