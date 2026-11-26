# DCU-D0-13：删除无条件 60 秒自退，改为显式选择加入

状态：交 `dcu-verifier` 复核。基线 `feat/dcu 143c6125`，分支 `dcu/D0-13-selfexit`。

## 1. 根因与失败原文

`cmd/dcu/main.c` 旧代码无条件启动 60 秒定时器：

```c
static uv_timer_t safety_timer;
uv_timer_init(loop, &safety_timer);
uv_timer_start(&safety_timer, on_safety_timeout, 60000, 0);
```

60 秒后 `on_safety_timeout` 刷盘并 `service_stop`，产品自行退出。这与
`cmd/dcu/dcu_heartbeat.c:75,115-117` 的契约冲突（自退只在演示模式按
`DCU_HEARTBEAT_CYCLES` 启用；`0` = 一直跑、由平台收尾），也与
`acceptance/dcu/tasks/dcu_northbound.yaml --keep-alive` 的注释冲突。

合并树验收实测（dcu-verifier 在 main/feat 树上）：套件约 50–61 秒，r1/r2 在 60 秒处出现
ConnectionReset/ConnectionRefused；`product.out` 出现
`[SAFETY] Timeout reached, initiating clean shutdown...` 与
`DCU Node stopped successfully`，33 条里 3 条失败；r3 通过（50.6 s）；r4 55.2 s 时另一用例
失败。60 秒线与套件时长赛跑，是产品侧确定性缺陷。

## 2. 修复

- 新增纯函数 `dcu_runtime_self_exit_ms(const char *value)`（`cmd/dcu/dcu_runtime_self_exit.{c,h}`）：
  未设/NULL/空串/`0`/负数/非法/溢出 → 0；正整数 N → N*1000 毫秒。
- 装配也移入模块：`dcu_self_exit_start(loop, cb)` 内部解析 `DCU_SELF_EXIT_SECONDS`，>0 才
  `uv_timer_init/start`；`main.c` 只留一行调用，组合根行数 225→224（预算内，未用
  `--update-baseline`）。未设即不存在定时器。生产与 keep-alive 永不自退；演示自退继续由心跳
  `DCU_HEARTBEAT_CYCLES` 负责，不新增机制。
- 定时器/回调/日志改名为 self-exit（`on_self_exit_timeout`、`self_exit_timer`、
  `[SELF_EXIT]`），不再把选择加入的自退伪装成“安全超时”。
- 未改 acceptance 用例、keep-alive 语义、心跳自退、foundation/modules。

## 2b. 对抗复核追加修复（F2/F3/F4）

- **F2 壳不转发 SIGTERM**：`acceptance/dcu/dcu_product.py` 安装 SIGTERM/SIGINT 处理器，
  只记标志并 `terminate()` 子进程（不再在信号处理器里 `wait()`：主线程阻塞在 `proc.wait()`
  时同线程重入 waitpid 锁会卡死，实测 child 变 zombie、壳不退）。主循环 poll 收尾，超时 3 秒
  `kill()`；子进程已退出时幂等退出（143）。新增守卫
  `ci/scripts/check_acceptance_wrapper_signals.py`（good/bad 夹具 + live，110/110）。
  运行时：wrapper=54714 child=54832，`kill -TERM` 后 `DCU_ORPHANED=no`、`WRAPPER_GONE=yes`、
  退出码 143；child 先退出再 SIGTERM 同样 143。
- **F3 解析前导字符**：`dcu_runtime_self_exit_ms` 只接受首字符 `'0'..'9'`；`" -18446744…"`、
  `" 5"`、`"+5"` 均 0，单测覆盖。
- **F4 拆除竞态**：`dcu_heartbeat.c` 新增 `dcu_heartbeat_stop`（stop + close 两个 timer，
  幂等）；`main.c` 在 `dcu_node_destroy` 前调用，main.c 224→225（正好基线 225，未超）。
  回归测试 `tests/test_dcu_heartbeat_stop.c`：stop 后两个 timer 不 active、重复 stop 安全、
  `uv_run(NOWAIT)` 不跑回调；先 stop 心跳再 `dcu_storage_destroy` 再 run 不崩。
  运行时：`timeout -s TERM 15/20/25 dcu` 三次均 rc=124、`FATAL SIGNAL|Aborted|core` 0 次、
  每次都有 `DCU Node stopped successfully`；`DCU_SELF_EXIT_SECONDS=15` 自退 rc=1、fatal 0、
  `[SELF_EXIT]` 与 `Node stopped` 各一次。

## 3. 测试

`tests/test_dcu_runtime_self_exit.c`（5 例）：

- `test_unset_is_zero`：NULL → 0（不启动）。
- `test_five_seconds`：`"5"` → 5000 ms。
- `test_zero_is_disabled`：`"0"` → 0。
- `test_invalid_is_disabled`：`"abc"` → 0。
- `test_negative_is_disabled`：`"-3"` → 0。

## 4. 验证

- clean Release build：`check_build_warnings.py` 0 新告警；单测 8/8（解析）+ 2/2（心跳停止）。
- `ctest -R 'dcu|freeze|runtime'` 38/38。
- ASAN `detect_leaks=1` 4/4 零泄漏；fanalyzer 3 known / 0 new；SRP 干净；guard 110/110；
  composition-root budget clean（main.c 225/225）。
- 分支验收 `acceptance/run.sh dcu`：2/2，3 + 33 passed；northbound 套件 64.92 s 跑完，
  产品未再因 60 秒自退退出（product.out 无 `[SAFETY] Timeout`），平台收尾后无孤儿。

## 5. 未决项

- `DCU_SELF_EXIT_SECONDS` 供测试/运维显式要求退出时使用；非法值静默按 0（不自退），
  没有告警。若需要诊断，可在组合根加一条 WARN，但本次保持最小。
- 验收首轮出现北向 socket 未就绪导致 8 条 skip（已知平台启动竞态），干净重跑 33 passed；
  与本修复无关，未改台架。
