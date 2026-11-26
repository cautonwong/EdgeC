# FORMAL-P2 实测时间与计数

本机：WSL2，Java OpenJDK 17.0.21，CBMC 5.12，TLC `2026.11.15.002146 (rev: bc1b506)`；Release+Ninja 构建空闲时。绝对秒数随负载变化。

## 形式化门禁（最终 tip 独立复跑）

| 运行 | 结果 | 耗时 |
| --- | --- | --- |
| `check_formal.sh --strict`（1 shard / 4 rows） | 1012 properties；84083 states | 21 s |
| 同上，含临时 DCU probe（2 shards / 5 rows） | 1013 properties；84083 states | 22 s |
| 同上，隐藏 `/opt/formal/env.sh`（vendored jar 回退） | 1012 properties；84083 states | 22 s |
| `check_formal_negative.sh --strict` | 1 mutation + 2 TLA 反例 | 4 s（arch gates 内 3 s） |
| `check_formal_sync.py` | 4 rows / 1 shard / 0 warning | <1 s |
| `check_formal_isolation.py` | clean | <1 s |
| `test_formal_manifests.py` | 10/10 | <1 s |
| `run_guard_selftest.py` | 228/228 | ~15 s |
| `run_arch_gates.sh build 6ad00780` | rc=0（含 formal 正负例） | ~55 s（代理实测 54.9–55.7 s） |

## 按行分解（manifest `runtime` 与实测）

| 行 | mode | 计数 | 实测 |
| --- | --- | --- | --- |
| `formal/cbmc/modbus_decode.c` | proof | 1012 properties | 18–21 s |
| 同上 | cover | `[main.coverage.1] SATISFIED` | 0–2 s |
| 同上 | function `crc_reject` | 两条 SUCCESS 断言 | 0–1 s |
| `formal/tla/Arbiter.tla` | tla | 84083 states | 1–2 s |
| mutation 脚本 | script | `_get_crc.pointer_dereference.* FAILURE` | 3–4 s |
| `ArbiterNoRecovery.cfg#NoDispatchInRecovery` | cfg 负例 | depth 8 反例 | ~1 s |
| `ArbiterCrosstalk.cfg#LateResponseNotAssignedToNext` | cfg 负例 | depth 9 反例 | ~1 s |

预算：总 300 s，CBMC proof 240 s、CBMC aux 60 s、TLC 每行 120 s；所有分片串行共享总预算。
