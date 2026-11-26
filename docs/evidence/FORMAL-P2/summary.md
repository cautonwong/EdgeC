# FORMAL-P2 证据包总结

- **任务**: FORMAL-P2 形式化门禁分片化与 CI TLA 工具链（`/home/edge/migration/formal-followups-2026-11-24.md` 第 1、2 项）
- **分支**: `worktree-formal`，基线 `42e1360d`（feat/iems tip）
- **worktree**: `/tmp/iems-formal/wt`
- **状态**: 通过（CBMC 5.12 + TLC `2026.11.15.002146`，本机；vendored jar 回退实测）
- **产品代码改动**: 零（只改 `formal/`、`ci/scripts/`、`.github/workflows/ci.yml`、`formal/README.md`、`docs/evidence/`）
- **未合，未 push**

## 1. 交付

- `formal/tools/tla2tools.jar`（4,510,884 B，SHA256 `7e53aa63…d0fb84f`）与 `formal/tools/README.md`：vendored 固定 TLC，nightly 无稳定 URL，故入库；README 记 Build-Revision `bc1b506`、Build-TimeStamp、许可证与升级步骤。
- `ci.yml` formal job：改名 `Formal Verification (CBMC 5.12 + TLC 2026.11.15.002146)`；安装 `openjdk-17-jre-headless` 并校验 `java -version` 为 17；4 条 formal 命令与 `build/formal` 上传不变。
- `formal/manifest/iems.tsv` 扩为 18 列并拆 4 行：modbus proof（unwind 261 / max_len 260 / 1012 properties）、modbus cover（assertions `^\[main\.coverage\.1\].*: SATISFIED`）、modbus function `crc_reject`、TLA 正例（cfg + `state_count 84083` + jar 哈希）。
- 新列：`mode cfg function assertions`。`negative` 改为分片声明：`<cfg>#<Invariant>` 或 `.sh/.py` 脚本；未知形态在解析期失败。
- `ci/scripts/formal_manifest.py`：schema/owner/mode/路径/negative 语法的唯一事实来源；`--rows` 供两个 bash runner 消费；`check_formal_sync.py` import 同一模块。
- `ci/scripts/formal_tools.sh`：TLA jar（`TLA2TOOLS` → `/opt/formal/env.sh` → `formal/tools/`）与 CBMC 版本/哈希 pin、Java major 提取的唯一实现。
- `check_formal.sh`：按 manifest 行执行 proof/cover/function/tla；每片 `build/formal/<owner>/run-manifest.tsv` 与日志；缺工具在 `--strict` 下失败，版本/哈希不符无条件失败。
- `check_formal_negative.sh`：按 negative 条目跑 TLC 或声明脚本；保留 worktree 状态自检；`build/formal/<owner>/negative/` 落日志。
- `formal/cbmc/negative/modbus_guard_mutation.sh`：原 CBMC mutation 从 runner 搬出，按 manifest 声明调用。
- `formal/tla/negative/ArbiterCrosstalk.cfg`：原 `grep -v` 临时生成的串包 cfg 落成受跟踪文件并由 manifest 声明。
- `check_formal_sync.py`：未登记文件扫描（`git ls-files formal` + 允许集）、owner/shard 一致性、按 mode 的通用校验；covers 符号、sync 祖先与漂移告警保留。
- `ci/scripts/test_formal_manifests.py`（10 个反例）由 `run_guard_selftest.py` 调用。

## 2. 验证（本机独立复跑）

| 检查 | 结果 |
| --- | --- |
| `check_formal.sh --strict` | rc=0，1 shard / 4 rows，1012 properties，84083 states，21 s |
| `check_formal_negative.sh --strict` | rc=0，1 mutation + 2 TLA 反例，4 s |
| `check_formal_sync.py --root .` | rc=0，4 rows / 1 shard，0 warning |
| `check_formal_isolation.py --root .` | rc=0 |
| `run_guard_selftest.py` | rc=0，228/228 |
| `check_ci_gate_parity.py --root .` | rc=0，18 arch / 4 formal |
| `run_arch_gates.sh build 6ad00780` | rc=0（含 formal 正负例、sync、isolation、selftest、ARCH-GRAPH 强模式） |
| `test_formal_manifests.py` | 10/10 |
| 临时 DCU shard | 2 shards / 5 rows，`build/formal/dcu/run-manifest.tsv` 出现 `dcu_probe.c proof PASS 1` |
| vendored 回退 | 隐藏 `/opt/formal/env.sh` 后 jar 解析到 `formal/tools/`，strict 22 s 全绿 |
| 红例 | 未登记 tracked 文件、缺 manifest、property_count 不符、缺 artifact 均得到预期失败 |

## 3. 关键行为

- 分片发现：`formal/manifest/*.tsv`；owner 允许集 `{iems, dcu}`（`formal_manifest.OWNERS`），文件茎必须等于 owner；未知 owner、空分片、未知 mode、重复 `(artifact, mode)`、未知 negative 形态一律 fail closed。
- 计数与哈希：proof 行比对 `** 0 of N failed` 的 N 与 `property_count`；TLA 行比对 `states generated` 与 `state_count`，并按行校验 jar SHA 与 TLC 版本横幅；同一分片的 TLA 行 jar 哈希必须一致。
- 日志布局：`build/formal/<owner>/<artifact>.<mode>.log`、`build/formal/<owner>/tla/`、`build/formal/<owner>/negative/`、每片 `run-manifest.tsv`。
- 负例语法：`<cfg>#<Invariant>`（要求 cfg 含 `INVARIANT <Invariant>` 行）或 `.sh/.py` 脚本（环境变量 `ROOT_DIR`、`FORMAL_OUT_DIR`、`CBMC_BIN`、`ARTIFACT`，须自行写 `$FORMAL_OUT_DIR/negative/<name>.log`）。

## 4. 残余

见 `residual.md`。`formal/README.md` 已同步 18 列 schema、三种 CBMC mode、negative 语法、owner 机制、vendored jar/Java pin 与分片日志布局。
