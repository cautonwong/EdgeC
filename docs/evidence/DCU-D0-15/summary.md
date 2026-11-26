# DCU-D0-15：task-monitor 验收等待语义修改（D0-16 计数修复之上）

状态：交 `dcu-verifier` 复核。基线 `feat/dcu 0f4de567`（含 D0-16 计数修复），
分支 `dcu/D0-15-task-monitor`。

## 1. 背景与根因

D0-15 首轮定位到产品 `_chain_finish_visit` 用 `task_due` 判成功归属的相位缺陷（另立
D0-16，已合入 0f4de567）。在修复后的产品上，旧用例仍有两个问题：

- 固定 30s 失败窗口 + 90s 恢复窗口：一次执行含失败重试时可超过 30s，最坏 120s，超过
  平台 `timeout=60`（thread）；r2 曾因此被杀。
- 恢复窗口只看 `success==40`，会拿恢复过程中已开始的混合执行或旧执行的 40 充数；
  r3 曾出现 40==0 的 stale 执行窗口。

## 2. 改动（只改 acceptance/dcu/tests/test_dcu_northbound.py）

新增 `_wait_monitor(task_id, predicate, deadline_s, what)`：500ms 轮询，超时带最后读数失败。

`test_task_monitor_matches_bench_failures`：

- 基线：等 `total==40 && start 存在`（10s）。
- 失败：控制生效后等 `start 变化` 且 `success==0 && total==40 && sent/recv≥80`
  （20s）；断言阈值不变。
- 恢复：以恢复完成后的当前执行 start 为新基线，等 `start 变化` 且 `success==40`
  （60s）；不会把恢复过程中的混合执行或失败执行当样本。
- 因一次完整执行（含失败重试）实测可超过平台默认 60s，用例加
  `@pytest.mark.timeout(120)`；这是派发允许的例外，证据记录实测最坏约 90s，
  未放宽任何断言。

稳定性配套（同文件、同属验收等待语义）：

- `D_00_00_02_000032`（跨零点冻结）移动到重负载的 000031 之前：避免重试洪峰后
  定时器回调延迟使 align delta 短时超 50ms（曾观测 130ms）；断言阈值不变。
- 000032/000033 的轮询对暂时性 `OSError`/`FrameError` 继续重试到 deadline，
  不吞失败、不改期望。

## 3. 验证（台架独占窗口，连续 3 次）

命令：`TESTPLATFORM=/home/edge/test-platform PYTHON=/home/edge/EdgeC/.venv/bin/python3
./acceptance/run.sh dcu`

| 轮次 | 结果 | northbound | 任务门禁 | 残留 DCU |
| --- | --- | --- | --- | --- |
| r1 | exit 0 | 34 passed / 71.28s | 2/2 | 无 |
| r2 | exit 0 | 34 passed / 43.95s | 2/2 | 无 |
| r3 | exit 0 | 34 passed / 48.89s | 2/2 | 无 |

分支单测：`ctest -R 'dcu|poller|dlt698'` 44/44（D0-16 合并树已跑）。

## 4. 保留的旧证据

首轮 r1 的“恢复等待 25s 仍 success=20”在 D0-16 合并前的树上出现；修复后本轮 3 次全部
通过。D0-15 的停止报告（产品计数缺陷）仍保留在本目录的 `monitor_repro.log` /
`monitor_recovery_stuck.log`，作为 D0-16 的根因证据。

## 5. 未决项

- 单次失败执行含重试时实测可到约 60-90s；这是产品执行时长，不是测试窗口。若未来要求
  60s 内完成，需要产品侧缩短重试/调度（另立 perf/defect），本单不预先放宽或改产品。
- 000032 顺序调整只影响同文件用例执行顺序，不改变判据与产品行为。
