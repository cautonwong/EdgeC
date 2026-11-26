# DCU-D0-18：执行根因核查与修复

状态：修复完成，3 连验收绿。分支 `dcu/D0-18-execution-rootcause`，基 `a19f3edb`。
只改产品 poller 与验收台架记录（bus.json 增时间戳/连接号），不改断言。

## 1. 根因（三条实测）

### 1.1 同一执行反复重抄（sent 497~2327、执行 60-90s）

`cmd/dcu/dcu_poller.c::_chain_load_pending` 每个 step 按 `task_due[t]` 重新装载，
不看该表本次执行是否已访问。失败注入了电压 DI 后每趟变慢，`task_due` 在趟中翻假，
未访问表要等下一个周期；而已访问表在下一个到期 tick 又被重新装载 → 同一执行 4~8 遍/表。
实测（加临时 DIAG 到 `_chain_finish_visit`）：一次执行内每表访问 4~8 次，
`meters_left=0` 后仍持续提交；monitor `sent` 497/1668/2327，远超 40 表×主备 2 次=80。

### 1.2 慢趟被 due 截断（执行跨 60s）

即 `task_due` 门禁：执行已开始后未访问的表在非到期 tick 不装载，只能等下一个周期
（台架默认周期 60s），一趟被拖到分钟级。修复后无需该门禁：`status==1` 即补齐目标表。

### 1.3 同 DI 重复帧（4ms 双 spare / main→main→spare）

台架 bus.json 增加 `at` + 连接号后：
- **修复前**（D0-17 分支实测）：`605.416/605.420` 同连接同 DI 两帧 4ms 内 → 同一执行
  被重复装载导致的重复提交，随 1.1 的修复消失。
- **修复后**（本分支失败复现 run r4）：`02010100` 同连接两帧间隔 15.1s，随后是第二帧的
  spare（322.456→322.691）——**两次执行各自的主用请求**，不是重复提交。
- 仅存的相邻同 DI 对是 7 对 `<unparsed>`，全部**不同连接**、9~47ms，属台架连接建立阶段的
  噪声记录，不是产品 645 请求。
结论：没有证据表明修复后产品对同一 (meter,tag) 双提交；驱动 `driver_dlt645.c` 无重试。
台架记录只在 `_record` 调用一次/帧，未发现重复记录路径；`<unparsed>` 跨连接记录作为
台架噪声如实保留（未折叠）。

### 1.4 失败执行里 success>0

不是台架 `_sim_overrides` 竞态，也不是归属错误：`success_meters` 只在**本次访问全部
tag 成功**（visit_fail==0 且有成功项）时计一次，表级、每表每执行一次。失败注入电压 DI
时该表访问必含失败 tag → success 保持 0（验收 000031 的 success==0 判据因此成立）；
失败注入运行里出现的 success>0 样本来自恢复阶段执行或 6035 中段读数，不是失败访问被
误计。注意：`success_meters <= total_meters` 仅在执行内目标集固定时成立；执行中加表
或改 task_mask 可越过（既有缺陷 A2，后续单元修）。

## 2. 修法（产品根因）

`cmd/dcu/dcu_poller.c`：
- `_chain_load_pending`：仅对 `monitor.status == 1` 的任务装载；**每张目标表每次执行只装载
  一次**（`visited` 位守门）；不再用 `task_due` 在趟中截断。
- `_chain_next_meter`：先 `_chain_load_pending` 再 `_task_note_visit`（visited 在装载后置位，
  本次装载不受自己影响）。
- `_chain_finish_visit`：success 归属改为 `visited` + 新增 `succeeded` 位图，每表每执行
  只计一次；表级语义（多任务共享同表时，成功访问给所有归属任务各计一次）。
- 删除 `visit_new_mask`（一趟一表后无“首次”信息需求）与相关注释。

生命周期（`dcu_poller.h` 注释同步）：`_task_begin` 清零 visited/succeeded/sent/recv/
success 与时标；档案重开（revision 变化）清 visited+succeeded 后重算 total；迟到旧执行
回调因 visited 不匹配不计入 success（遥测写入不保证，见 F5）；`success_meters <=
total_meters` 仅当执行内目标集固定（执行中档案/mask 变化可越过，既有 A2）。
6034/6035 线上字段与 D2-08/D0-16 语义不变（只是计数时机正确）。

## 3. 回归测试

`tests/test_dcu_poller.c`：
- 新增 `test_execution_polls_each_meter_once`：两表两通道、每步到期；异步回调；
  断言一块回调成功/另一在途时**不重抄**（submit 仍为 2）；两块完成即结算 status=2、
  success=2；下一次执行才各表一遍（submit=4）。
- `test_dedup_overlapping_tasks` 增加表级成功断言：同表两任务各计 success=1。
- 修复后 `ctest -R 'dcu|poller|dlt698|698'` 45/45。

## 4. 验收（最终产品代码，注释/测试改动不影响）

3 连绿（每次 2/2）：
- r1 `35 passed in 39.64s`
- r2 `35 passed in 78.22s`
- r3 `35 passed in 59.24s`
均 `全部合格:2 个验收任务`，无 DCU 残留（平台收尾）。

一趟执行耗时分布（bus.json 同表 02010100 相邻执行间隔，含失败注入与正常）：
min 3.7~5.0s，median 10.0~19.9s，max 26.4s（n=114~148/轮）。供 D0-19 设等待预算：
按最坏 26.4s×2≈53s，两趟（失败+恢复）加两段 start 等待，120s 上限不够，证据建议 240s。

## 5. 门禁

- clean Release build warning 0（`check_build_warnings.py`，third-party open62541 不计）。
- ctest 45/45（dcu/698/poller/dlt698）；ASAN `detect_leaks=1` 同集；guard 118/118；
  fanalyzer 3 已知 0 新增；SRP/plane/component 干净。
- 台架 `acceptance/dcu/dcu_product.py` 只加 `at`/`conn` 记录字段（诊断用，不判断言）。

## 6. 未决项

- D0-19：按“一趟=一次执行”重写 000029/000031 的窗口与时间预算（240s mark、实测最坏×2）。
- `<unparsed>` 跨连接噪声帧（台架连接建立阶段）未定性，不影响 645 计数；如需要可另立台架清理。
- poller 每执行多 512B/任务 succeeded 位图（16 任务≈8KB 静态 RAM），不在 component budget 范围。
