# FORMAL-P1 实测时间

机器：WSL2，Java 17.0.21，TLC `2026.11.15.002146 (rev: bc1b506)`，4 workers。绝对秒数随机器负载波动。

| 运行 | 配置 | 结果 | 状态数 | 耗时 |
| --- | --- | --- | --- | --- |
| TLC 正例 | `Arbiter.cfg`, `RecoveryGate=TRUE` | `No error has been found` | 84,083 generated / 30,538 distinct，depth 15 | 2 s |
| TLC 负例 | `negative/ArbiterNoRecovery.cfg`, `RecoveryGate=FALSE` | `Invariant NoDispatchInRecovery is violated` | depth 8 最短反例 | 1 s |
| TLC 串包临时 cfg | 负例去掉 `NoDispatchInRecovery` | `Invariant LateResponseNotAssignedToNext is violated` | depth 9 反例 | 1 s |
| `check_formal.sh --strict` | CBMC 1012 + TLA 正例 | rc=0 | CBMC 1012 properties；TLA 84,083 generated | 63 s（重复 35–63 s） |
| `check_formal_negative.sh --strict` | CBMC mutation + TLA 两跑 | 脚本 rc=0；TLC 进程各 rc=12 | 两个 TLA 反例 | 10–12 s |
| `run_guard_selftest.py` | 全部 guard 正反夹具 | rc=0 | — | 19 s |
| `ctest --output-on-failure` | 全量 | rc=0 | 354/354 passed | 245.97 s |
| `run_arch_gates.sh build 1f7f2133` | 架构门禁 + formal | rc=0 | — | 110 s |

- 负例的状态生成数随并行 worker 在找到反例时的停止位置变化（实测约 1.0k–6.0k generated），反例深度固定（8 / 9）。门禁只记录反例深度与不变量名，不把负例状态数写进 manifest。
- 正例状态数是完整搜索，重复运行稳定在 84,083 generated；manifest 的 `state_count` 即此值，门禁运行时比对。
- 单次 TLC 上限 120 s；`check_formal.sh` 总上限 300 s，本机实测 35–63 s。
