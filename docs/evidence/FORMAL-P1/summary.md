# FORMAL-P1 证据包总结

- **任务**: FORMAL-P1 TLA+ arbiter 事务模型（Phase 1）
- **分支**: `worktree-formal-p1`，基线 `1f7f2133`（feat/iems tip）
- **worktree**: `/tmp/iems-fix/formal-p1-wt`
- **状态**: 通过（TLC `2026.11.15.002146`，本机）
- **产品代码改动**: 零（只改 `formal/`、`ci/scripts/`、`docs/evidence/`、`formal/README.md`）
- **未合，未 push**

## 1. 交付

- `formal/tla/Arbiter.tla`、`formal/tla/Arbiter.cfg`
- `formal/tla/negative/ArbiterNoRecovery.cfg`（只翻 `RecoveryGate = FALSE`，不复制 `.tla`）
- `ci/scripts/check_formal.sh` 增 TLA+ 正例（jar 哈希、TLC 版本、`No error has been found`、`states generated > 0`、`state_count` 比对，120 s 上限）
- `ci/scripts/check_formal_negative.sh` 增 TLA+ 负例（仓库 cfg + `mktemp` 串包 cfg，非零命中，不写仓库）
- `formal/manifest/iems.tsv` 增 TLA+ 行（状态数、耗时、TLC 版本、jar SHA256、sync `1f7f2133`）
- `ci/scripts/check_formal_sync.py` 覆盖 TLA+ 行（伴随 `.cfg`、`state_count`、`jar_sha256`、负例 cfg 形状）
- `formal/README.md` 增 Phase 1 工件、工具 pin、运行时间、残余

## 2. 模型与不变量

| 不变量 | 模型含义 | 代码守卫 |
| --- | --- | --- |
| `NoDoubleSettle` | 每条命令的 `settleCount <= 1` | `arbiter_on_io_done` 先摘 owner（`arbiter.c:319-322`）；引擎超时/完成前 `arbiter_owns_cmd`（`master_engine.c:69`、`:104`） |
| `NoDispatchWhileOwner` | 派发时 `owner = 0` | `_dispatch_next_locked` 的 `slot->owner != NULL` 检查（`arbiter.c:173`） |
| `NoDispatchInRecovery` | `now < recovery_until_ns` 时不派发 | `_dispatch_next_locked` 的恢复窗口守卫（`arbiter.c:185-191`） |
| `LateResponseNotAssignedToNext` | 迟到应答只能被丢弃或由本事务消费，不能落到下一事务 | `_dispatch_next_locked` 窗口 + `_master_run_command` 开帧前 `DISCARD_RX`（`master_engine.c:218`）+ `_master_io_cb` 只对 `current_cmd` 解码（`master_engine.c:180`） |

代码锚点（行号会漂移，函数名与不变量名稳定）：`arbiter.c:181` `_dispatch_next_locked` 恢复窗口；`arbiter.c:313` `arbiter_on_io_done`；`arbiter.c:403` `arbiter_check_timeouts` 窗口写入；`master_engine.c:237` `_master_run_command` 发送/deadline/current_cmd。

## 3. 门禁结果（本机实测）

| 检查 | 结果 | 耗时 |
| --- | --- | --- |
| `./ci/scripts/check_formal.sh --strict` | rc=0；CBMC 1012 properties；TLC `No error has been found`，84,083 states generated | 63 s（重复运行 35–63 s） |
| `./ci/scripts/check_formal_negative.sh --strict` | 脚本 rc=0；CBMC 变异副本 rc=10 命中 `_get_crc.pointer_dereference.*: FAILURE`；TLC 进程 rc=12，仓库 cfg 违反 `NoDispatchInRecovery`（depth 8），临时 cfg 违反 `LateResponseNotAssignedToNext`（depth 9） | 10–12 s |
| `python3 ci/scripts/check_formal_sync.py --root .` | rc=0，2 个工件，0 warning | <1 s |
| `python3 ci/scripts/check_formal_isolation.py --root .` | rc=0，产品构建图/源码零引用 | <1 s |
| `python3 ci/scripts/run_guard_selftest.py` | rc=0 | 19 s |
| `ctest --output-on-failure`（354 用例，含 `app_platform_e2e`） | 354/354 passed | 245.97 s |
| `./ci/scripts/run_arch_gates.sh build 1f7f2133` | rc=0（含 formal 正负例、sync、isolation） | 110 s |

TLC 负例的“非零命中”指 TLC 进程返回 12；`check_formal_negative.sh` 在确认命中后返回 0，失败时才非零。

TLC 正例：84,083 states generated / 30,538 distinct，depth 15，约 2 s。负例最短反例 8 步；串包反例 9 步。

## 4. Manifest 行

```text
formal/tla/Arbiter.tla	arbiter transaction safety: no double settle, no dispatch while owned or in recovery, no late response assigned to the next command	modules/edge_core/src/arbiter.c:arbiter_submit_cmd;modules/edge_core/src/arbiter.c:_dispatch_next_locked;modules/edge_core/src/arbiter.c:arbiter_on_io_done;modules/edge_core/src/arbiter.c:arbiter_check_timeouts;modules/edge_io/src/engines/master_engine.c:_master_run_command;modules/edge_io/src/engines/master_engine.c:_master_on_timeout;modules/edge_io/src/engines/master_engine.c:_process_req_result	TLC 2026.11.15.002146 (rev: bc1b506)	RecoveryGate=TRUE; Timeout=2; LateBound=1; MaxQueue=2; MaxTime=6; Cmds={1,2}; Priorities={0,1,2}	n/a	n/a	formal/tla/negative/ArbiterNoRecovery.cfg	1f7f2133	iems	positive 2s; negative 1s; crosstalk 1s	n/a	84083	7e53aa636dac8a3cb0533f343115c0548e03fc0ce5edc7739becf58d3d0fb84f
```

## 5. 结论与残余

- 四条安全不变量在声明的有界模型内成立。
- 结论只覆盖迟到应答上界 `LateBound < Timeout`（本次 `1 < 2`）。从原帧发出算起，超过“超时 + 恢复窗口”（`2 * timeout_ms`）才到达的应答必然落在下一帧发出之后，仍会串包；越过窗口边界也可能串包。模型没有放宽不变量，残余见 `residual.md`。
- 反例到代码步骤的映射见 `counterexample-mapping.md`。
- 未做活性/公平性、App 生命周期、WAL；未改产品代码。

## 6. 证据索引

- `commands.md`：可重跑命令
- `runtime.md`：耗时与状态数
- `tla-positive-output.txt`：正例 TLC 输出
- `tla-negative-output.txt`：负例与串包反例输出
- `counterexample-mapping.md`：反例状态到代码步骤
- `residual.md`：bounds、assumptions、残余
