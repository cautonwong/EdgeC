# DCU-D0-19：000031/000029 完成锚定与执行窗口

状态：测试时序修复完成，coverage 3 连绿 + release 1 次绿。分支
`dcu/D0-18-execution-rootcause`（同 D0-18 单元），已 merge `feat/dcu cde627a0`
（含 main `beb6629b`）。**只改验收测试时序，未动任何断言与产品逻辑。**

## 1. 000031 `test_task_monitor_matches_bench_failures`

原谓词 `success==0 && sent>=80 && recv>=80` 会在执行中段命中（sent/recv 是执行内累计），
提前返回后把未完成的失败趟留给恢复窗口；coverage 构建慢趟下必现。

改为完成锚定（两段等待，两次）：
1. 失败执行：先等 `start != baseline.start`（新执行出现，40s），再等
   `start == exec_start && end != None && total==40 && success==0 && sent>=80 && recv>=80`
   （该执行完成，35s）。
2. 恢复执行：恢复后读 `start_before`，等 `start != start_before`（40s），再等
   `start == recovered_start && end != None && total==40 && success==40`（90s）。

`@pytest.mark.timeout(180→240)`：仅时间预算，断言未变。证据：
- coverage 单趟 bus 间隔（同表相邻执行）median 9.84~16.31s、p90 12.99~25.44s、
  max 33.33s（3 轮）；恢复完成 90s ≈ max×2.7。
- 并行构建干扰下曾见恢复趟 success=38/39、sent≈560 仍在跑（r2/r4 失败样本，已记录
  在案）；在空闲机器上三连绿，故不以该干扰放宽断言。
- 000031 整案阶段耗时（task_monitor_windows.json）：
  - cov r6 total 30.12s（6.02/5.52 + 4.52/6.02）
  - cov r7 total 33.76s（8.07/17.14 + 3.02/5.52）
  - cov r8 total 23.11s（6.53/6.04 + 4.01/6.53）
  - release total 19.59s（2.01/5.53 + 4.51/7.54）
  均远低于 120s。

## 2. 000029 `test_master_uses_spare_di_after_primary_failure`

旧实现按“先等一段、再取快照”的墙钟窗口与首尾裁剪判配对；coverage 慢趟下相邻执行
混进窗口（`['main','main','spare']`）。改为与 D0-17 同思路但按 D0-18 新语义：
- bus.json `at` 时间戳把每块表请求聚成执行（间隔 >1s = 新执行）；
- 窗口从控制生效后的新执行开始；每块表首个“半截执行”（只剩 main 或只剩 spare）
  整段跳过，之后每次执行必须恰好 `[main, spare]`；保留“无第三次请求”判据；
- 去掉固定 sleep。断言未变。

## 3. 验证

- **COVERAGE=ON**（`-DCMAKE_BUILD_TYPE=Release -DCOVERAGE=ON`，EDGEC_BUILD_DIR=build-cov）
  连跑 3 次：35 passed / 2-2 全绿，44.26~53.45s 总时长；用例耗时 23.11~33.76s。
- **非 coverage**（Release build）1 次：35 passed / 2/2，40.15s。
- 合并后 `ctest -R 'dcu|poller|dlt698|698'` 45/45；guard selftest 284/284（main 新基线）。
- 无 DCU 残留（平台收尾）。

## 4. 未决

- 机器并行构建/其他 agent 高负载时，单趟可 >90s（r2/r4 样本），此时 000031 恢复段
  仍可能超时；已按 verifier 要求先交证据、不放宽断言。CI 独占环境下未见。
- 000029 的“半截执行”跳过只作用于每表首个执行；控制生效后的完整执行仍严格成对。
