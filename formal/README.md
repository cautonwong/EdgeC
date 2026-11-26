# EdgeC 形式化验证（Phase 0 CBMC + Phase 1 TLA+ + Phase 2 分片 runner）

本目录是形式化验证的入口。Phase 0 接 CBMC，覆盖 Modbus RTU 响应解码；Phase 1 接 TLA+，覆盖 arbiter 事务边界；Phase 2 把门禁改成 manifest 分片驱动，并 vendor 固定 TLC。Frama-C / Lean 见接入细则的后续阶段。

## 工件与覆盖

| 工件 | 覆盖 | 属性 |
| --- | --- | --- |
| `formal/cbmc/modbus_decode.c` | `foundation/src/protocol/modbus_codec.c` 的 `modbus_rtu_decode_frame_ex` | 有界输入(0..260 字节，全符号化)不越界；返回值只在 `{0, 1, -1, -2}`；成功时 `data_out` / `data_len` 在输入缓冲内且不覆盖 CRC 字段；CRC 不符被拒绝 |
| `formal/cbmc/negative/modbus_guard_mutation.sh` | 同上，删掉临时副本里的 `in->len < resp_len` 守卫 | CBMC 在 `_get_crc.pointer_dereference.*` 报 `pointer outside object bounds: FAILURE` |
| `formal/tla/Arbiter.tla` / `Arbiter.cfg` | `arbiter.c` 的 `arbiter_submit_cmd`、`_dispatch_next_locked`、`arbiter_on_io_done`、`arbiter_check_timeouts`；`master_engine.c` 的 `_master_run_command`、`_master_on_timeout`、`_process_req_result` | `NoDoubleSettle`、`NoDispatchWhileOwner`、`NoDispatchInRecovery`、`LateResponseNotAssignedToNext` |
| `formal/tla/negative/ArbiterNoRecovery.cfg` | 同上，只把 `RecoveryGate` 翻成 `FALSE` | TLC 报 `NoDispatchInRecovery` 反例；不复制 `.tla` |
| `formal/tla/negative/ArbiterCrosstalk.cfg` | 同上，去掉 `INVARIANT NoDispatchInRecovery` 一行 | TLC 报 `LateResponseNotAssignedToNext` 串包反例；不复制 `.tla` |
| `formal/tools/tla2tools.jar` | TLC 2026.11.15.002146 运行器 | 只读工具，不参与产品构建；哈希与版本见下 |
| `formal/manifest/iems.tsv` | 上述全部工件 | 18 列 manifest：mode/bounds/计数/negatives/sync/owner/实测时间/jar SHA256 |

manifest 以 owner 分片；本批只有 `iems.tsv`。DCU 分片由 dcu-verifier 维护，登记到 `formal/manifest/dcu.tsv` 后被门禁自动执行；本目录不写 DCU 工件。

## 工具 pin

- CBMC 5.12，`cbmc --version` 首行必须等于 `5.12 (cbmc-5.12)`。
- 二进制 SHA256 `ef72f83869f9cc1af6e3e1721fff9ab0cb3cbc40a98105b793648605c111ee0d`。
- 安装包：`cbmc_5.12-5_amd64.deb`，SHA256 `be7e049151cacb7511ebff2003ebc5e2d0f761cf705b6773eee0837ead94155a`；CI 先校验安装包，再安装，门禁再校验二进制哈希与版本。
- TLC `2026.11.15.002146 (rev: bc1b506)`；jar 已 vendor 到 `formal/tools/tla2tools.jar`（4,510,884 B，SHA256 `7e53aa636dac8a3cb0533f343115c0548e03fc0ce5edc7739becf58d3d0fb84f`），来源与升级步骤见 `formal/tools/README.md`。manifest 的 TLA 行记录同一个版本与哈希。
- Java pin：OpenJDK 17（CI 安装 `openjdk-17-jre-headless` 并校验 major；门禁在启用 TLA 时独立校验 Java major 17）。
- 解析顺序：调用方环境（`CBMC`/`TLA2TOOLS`）→ `/opt/formal/env.sh` → PATH / 仓库 `formal/tools/`。无论来源，运行前按 manifest 的 `jar_sha256` 与 CBMC pin 重新校验；版本或哈希不符一律失败，不受 `--strict` 影响。只有“工具缺失”由 `--strict` 决定失败或 SKIP。

## 运行

```bash
source /opt/formal/env.sh                 # 可选；容器/CI 不需要
./ci/scripts/check_formal.sh --strict     # 主门禁：执行所有 manifest 行，缺工具失败
./ci/scripts/check_formal.sh              # 本地非严格：缺工具打印 SKIP
./ci/scripts/check_formal_negative.sh --strict
python3 ci/scripts/check_formal_sync.py --root .
python3 ci/scripts/check_formal_isolation.py --root .
python3 ci/scripts/formal_manifest.py --root . --rows   # 调试：打印解析后的行
```

`check_formal.sh` 与 `check_formal_negative.sh` 先调用 `formal_manifest.py` 枚举 `formal/manifest/*.tsv`；manifest 缺失、表头/字段数不符、未知 owner/mode/negative 形态或工件缺失都会在运行前失败。日志与 `run-manifest.tsv` 按分片写到 `build/formal/<owner>/`：正例日志为 `<artifact-stem>.<mode>.log`，TLA 副本与日志在 `build/formal/<owner>/tla/`，负例日志在 `build/formal/<owner>/negative/`；每片的 `run-manifest.tsv` 记录 `artifact mode status count seconds`。这些文件都在 `.gitignore` 的 `build*/` 下，不写受跟踪文件。`check_formal_negative.sh` 只改 `mktemp -d` 下的生产源副本和 TLA+ 临时 spec/cfg；结束时校验 `git status --porcelain --untracked-files=normal` 为空。`.gitignore` 的 `/build` 忽略 `run_arch_gates.sh` 在仓库根创建的 `build` 符号链接（`build*/` 只匹配目录，覆盖不到链接）。脚本启动时在 `mktemp -d` 临时仓库套用本仓库 `.gitignore` 自检该判定：仅有 `build` 链接算干净，真实未跟踪文件算脏。

本地全量门禁 `ci/scripts/run_arch_gates.sh <build> <base-ref>` 已包含上述 formal 步骤（非严格），CI 的 `formal` job 用 `--strict` 并上传 `build/formal/` 日志。

## 实测时间（本机，CBMC 5.12，2026-11-24）

| 范围 | unwind | properties | 主入口耗时 |
| --- | --- | --- | --- |
| 0..16 | 17 | 1012 | 4 s |
| 0..64 | 65 | 1012 | 13 s |
| 0..260 | 261 | 1012 | 44–85 s |

- 主门禁取 0..260：全范围实测 < 5 分钟总预算；`crc_reject` 具体入口 2 s，vacuity cover 3 s，负例 5 s。
- FORMAL-P2 的 manifest runner 在本 worktree（WSL2，Release 构建、机器较空闲）实测：正例 21–22 s（proof 18 s、cover 1 s、function 1 s、TLA 1 s），负例 4 s。绝对秒数随机器负载变化。
- 总预算：`check_formal.sh` 总上限 300 s，CBMC 主证明 240 s、辅助运行各 60 s，TLC 120 s。
- 三档的 property 总数相同（1012）；门禁解析 `** 0 of 1012 failed` 的 1012，与 manifest 的 `property_count` 比对，断言被删或分支丢失会失败。
- 机器负载会影响绝对秒数；P0 时代同一台机器上重复运行在 44–85 s 区间。

TLA+（同一台机器，Java 17，`-workers auto`）：

| 运行 | 结果 | 状态数 | 耗时 |
| --- | --- | --- | --- |
| 正例 `Arbiter.cfg` | `No error has been found` | 84,083 generated / 30,538 distinct，depth 15 | 2 s |
| 负例 `ArbiterNoRecovery.cfg` | `Invariant NoDispatchInRecovery is violated` | depth 8 最短反例 | 1 s |
| 负例 `ArbiterCrosstalk.cfg`（去掉 `NoDispatchInRecovery`） | `Invariant LateResponseNotAssignedToNext is violated` | depth 9 反例 | 1 s |

正例状态数写进 manifest 的 `state_count`，门禁运行时比对；负例状态数随并行 worker 的停止位置变化，不入门禁。单次 TLC 上限 120 s，整条 `check_formal.sh` 仍在 300 s 总预算内（FORMAL-P2 实测 21–22 s，含 CBMC 三行）。

## TLA+ Arbiter 事务模型（Phase 1）

- 代码锚点：`arbiter.c:181`（`_dispatch_next_locked` 的恢复窗口）、`:313`（`arbiter_on_io_done` 的超时窗口）、`:403`（`arbiter_check_timeouts` 的窗口写入）、`master_engine.c:237`（`_master_run_command` 的发送、deadline 重置、`current_cmd`）。行号只作参考，生产代码改动会漂移；模型注释以函数名和不变量名为准。
- 每一步对应一个可观察步骤：提交/排序、派发+发送、时间推进、arbiter 扫描超时、引擎定时器超时、正常应答、错误结算、迟到应答到达、stutter。
- 模型覆盖：队列、owner、优先级、deadline、恢复窗口、迟到应答、引擎已发送/已读取。抽象掉且不能派发或结算的路径：pause、帧间静默、独占总线、队列满丢弃；单总线、单 owner。
- 常量即范围：`Cmds={1,2}`、`Priorities={0,1,2}`、`Timeout=2`、`LateBound=1`、`MaxQueue=2`、`MaxTime=6`、`RecoveryGate=TRUE`。模型内 `ASSUME LateBound < Timeout`，结论只覆盖迟到上界小于恢复窗口的情形。
- 负例：`negative/ArbiterNoRecovery.cfg` 只把 `RecoveryGate` 置 `FALSE`，最短反例违反 `NoDispatchInRecovery`；`negative/ArbiterCrosstalk.cfg` 在同一变体上去掉 `INVARIANT NoDispatchInRecovery` 一行，暴露 9 步串包反例（`LateResponseNotAssignedToNext`）。两个 cfg 都入库，`check_formal_negative.sh` 按 manifest 的 `<cfg>#<Invariant>` 条目把它们连同 `.tla` 复制到 `mktemp` 再跑；仓库工作树不被改动。
- 残余：超过恢复窗口、或下一帧已经发出之后才到的迟到应答仍会串包；模型不证明活性/公平性，也不覆盖多总线相互影响。详见 `docs/evidence/FORMAL-P1/residual.md`。

## Harness 设计与 assumptions

- 直接 `#include` 生产 `modbus_codec.c`，没有 test double。`fprintf` / `fflush` 由 CBMC 5.12 的 libc 模型解析，`net_buf_simple` 直接使用生产头文件；未退到纯函数方案。
- 输入缓冲是长度恰为 `in->len` 的 VLA。这样“读超过 `in->len`”在 CBMC 里是真实的对象越界，负例才有意义；若用固定 260 字节缓冲，内层长度守卫被删后仍读在数组内，负例不会红。
- 断言标签：
  - `modbus_decode: return code in contract`
  - `modbus_decode: output pointer inside input`
  - `modbus_decode: output length within input`
  - `modbus_decode: output data before CRC field`
  - `modbus_decode: valid frame accepted` / `modbus_decode: corrupted CRC rejected`（`crc_reject` 具体入口）
- vacuity：主入口有 `__CPROVER_cover(rc == 0)`，cover 运行要求 `[main.coverage.1] ... SATISFIED`，成功路径不可达即失败。
- assumptions：`hi_crc = false`（标准 Modbus 低字节先发）。`hi_crc = true` 变体未在本 harness 符号化覆盖，见“残余”。
- 负例：`formal/cbmc/negative/modbus_guard_mutation.sh` 只删临时副本里的 `if (in->len < resp_len) { return 1; }`。该守卫防的是 `_get_crc` / CRC 计算越界读；删掉后 CBMC 在 `_get_crc.pointer_dereference.*` 报 `pointer outside object bounds: FAILURE`。后面的全局 `in->len < frame_len` 检查仍会拒绝该帧，所以 harness 的成功路径断言不可能因这次 mutation 失败；负例锚定 CBMC 指针属性标签，而不是 `main.assertion.*`。脚本由负例 runner 以 `ROOT_DIR`、`FORMAL_OUT_DIR`、`CBMC_BIN`、`ARTIFACT` 调用，日志写 `build/formal/<owner>/negative/modbus_guard_mutation.log`。

## Manifest 与 sync

`formal/manifest/<owner>.tsv` 是门禁的唯一事实来源；`ci/scripts/formal_manifest.py` 是唯一实现字段表、owner/mode 集合、路径规则和 negative 语法的模块，shell runner 只消费它的 `--rows` 输出。改 schema 只改这个模块。

- 18 列，顺序固定：`artifact property covers tool bounds unwind max_len negative sync owner runtime property_count state_count jar_sha256 mode cfg function assertions`。所有字段必填；该 mode 不使用的字段写 `n/a`。
- owner 允许集是模块内常量 `{iems, dcu}`；`<owner>.tsv` 的文件名必须等于每行的 `owner`。新增 owner 要改 `formal_manifest.OWNERS`、增加 `<owner>.tsv` 与工件，并补 sync 自测。
- `mode ∈ {proof, cover, function, tla}`。同一 `(artifact, mode)` 全局只能出现一次；同一 artifact 可按 mode 拆成多行。
- CBMC 行（`tool` 含 `CBMC 5.12`）：`unwind` / `max_len` 为正整数且 `unwind ≥ max_len + 1`；`bounds` 必须记录 `--unwind`。`proof` 行要求 `property_count` 为正整数、`assertions` 为 `n/a`；`cover` / `function` 行 `property_count` 为 `n/a`，`assertions` 必填；`function` 行还要求 `function` 是 C 标识符。`cfg` / `state_count` / `jar_sha256` 为 `n/a`。
- TLA 行（`tool` 含 `TLC <version>`）：`mode=tla`，`artifact` 以 `.tla` 结尾，`cfg` 是 `formal/` 下存在的正规化路径，`state_count` 为正整数，`jar_sha256` 为 64 位 hex 且同一分片的所有 TLA 行一致；`unwind` / `max_len` / `property_count` / `function` 为 `n/a`。
- `assertions` 是分号分隔的 Python 正则；`cover` / `function` 行运行时逐条要求命中日志。其他 mode 必须写 `n/a`。
- `negative` 是 `n/a` 或分号分隔条目，只有两种合法形态：`<cfg 路径>#<Invariant 名>`（cfg 在 `formal/` 下、存在，且文件里有一行 `INVARIANT <名>`；只允许出现在 TLA 行）或 `<脚本路径>`（`.sh`/`.py`，位于 `ci/scripts/` 或 `formal/` 下、存在）。其他形态直接失败。
- 路径一律拒绝绝对路径、`..`、反斜杠和非正规写法；artifact/cfg 必须在 `formal/` 下，negative 脚本允许 `ci/scripts/` 或 `formal/`。
- `covers` 写 `文件:符号`，多个用 `;` 分隔。`sync` 是最后一次确认工件与代码一致的 commit；CBMC 与 TLA+ 行均对齐到 `a253c4db`。
- `check_formal_sync.py` 用同一模块校验所有 manifest，并额外校验：`covers` 文件/符号存在、`sync` 是 HEAD 祖先、`covers` 文件在 sync 之后有改动则告警（第一阶段告警，两个迭代周期后转硬失败）；扫描 `git ls-files formal`，除行引用 + `formal/manifest/*.tsv` + `formal/README.md` + `formal/tools/**` 之外的跟踪文件一律失败。
- `check_formal.sh` 按行执行：CBMC `proof` 解析 `** 0 of N failed` 并与 `property_count` 比对；`cover` 跑 `--cover cover --unwind <unwind>` 并校验 `assertions`；`function` 跑 `--function <function>` 并校验 `assertions`；TLA 行复制 `.tla`+`cfg` 后校验 jar 哈希、TLC 版本横幅、`No error has been found`、`states generated == state_count`。每片写 `build/formal/<owner>/run-manifest.tsv`（`artifact mode status count seconds`）。
- `check_formal_negative.sh` 按行执行 `negative` 条目：`cfg#Inv` 用 TLC 跑该行 `.tla` + cfg，要求非零、`Invariant <Inv> is violated`、有反例 trace；脚本条目用对应解释器执行并检查退出码与非空日志。

## 如何新增 harness / owner

1. 在 `formal/cbmc/` 新建一个带 `main()` 的 `.c`，直接 `#include` 生产 `.c`；输入用符号值，缓冲用与 `in->len` 等长的 VLA；每个断言写唯一标签；成功路径加 `__CPROVER_cover`。TLA+ 工件放 `formal/tla/`，正例 cfg 与负例 cfg 分开放。
2. 在 `formal/manifest/<owner>.tsv` 增加行：`artifact / property / covers / tool / bounds / unwind / max_len / negative / sync / owner / runtime / property_count / state_count / jar_sha256 / mode / cfg / function / assertions`。CBMC 主证明写 `mode=proof`；成功路径 vacuity 写 `mode=cover` 并在 `assertions` 填 cover 正则；具体入口写 `mode=function`。TLA+ 写 `mode=tla` 并填 `cfg` / `state_count` / `jar_sha256`。第一次运行后把 `** 0 of N failed` 的 N 填进 `property_count`。`unwind` 必须 ≥ `max_len + 1`。
3. 负例写在 `negative` 列：TLA+ 用 `<cfg>#<Invariant>`（cfg 必须带该 `INVARIANT` 行）；CBMC mutation 写一个位于 `formal/` 或 `ci/scripts/` 的 `.sh`/`.py`，按 `ROOT_DIR`/`FORMAL_OUT_DIR`/`CBMC_BIN`/`ARTIFACT` 契约实现，失败退出非零并写日志。不要再改 runner 脚本登记 harness。
4. 新增 owner：在 `ci/scripts/formal_manifest.py` 的 `OWNERS` 加名字，创建 `formal/manifest/<owner>.tsv` 与工件；`check_formal_sync.py` 的未登记文件扫描和 `test_formal_manifests.py` 会覆盖新分片。
5. 跑 `./ci/scripts/check_formal.sh --strict`、`./ci/scripts/check_formal_negative.sh --strict`、`python3 ci/scripts/check_formal_sync.py --root .`、`python3 ci/scripts/test_formal_manifests.py`；确认 `git status` 干净。新行会被 runner 自动发现，无需改 `check_formal.sh`。

## Nightly

Phase 0 的主门禁已经覆盖完整的 0..260 范围，且总时间 < 5 分钟，因此本 harness 没有被挪到 nightly 的部分；16 / 64 两档只用于验证 unwind 与耗时曲线。后续 harness 若在全范围超过 5 分钟预算，按同一模式拆分：PR 门禁取实测 ≤5 分钟的最大 `FORMAL_MAX_LEN` / `--unwind`，全范围放 nightly，bounds 与两档耗时都写进 manifest 与 README。仓库当前没有 nightly workflow；接入 nightly 时复用 manifest runner 的环境解析与判定逻辑，新增分片只需登记 manifest，不改调用点。

## 残余与限制

- 证明是**有界**的：CBMC 只覆盖 `in->len <= 260`、`hi_crc = false`、单次调用；不覆盖多帧滑动窗口、调用方缓冲容量、`hi_crc = true`、以及 `net_buf_simple` 中 `data`/`size` 与 `len` 不一致的调用方式。
- CBMC 对 `fprintf` 的 libc 模型只保证可解析；日志内容与真实 stderr 行为不在证明范围内。
- `formal/` 不参与产品构建；`check_formal_isolation.py` 防止产品构建图或源码反向引用。
- TLA+ 证明是**有界**的：单总线、两个命令、`MaxTime=6`，且 `ASSUME LateBound < Timeout`。超过恢复窗口（`timeout_ms`）或下一帧发出之后到达的迟到应答仍会被当成下一事务的应答；模型没有放宽这条不变量，只是把结论限定在声明的上界内。
- TLA+ 不覆盖 pause、帧间静默、独占总线、队列满丢弃/优先级剔除、多总线交错，也不证明活性或公平性（没有 `PROPERTY`）。这些路径不派发也不结算命令，不能改变四条安全不变量。
- 负例依赖 CBMC 自动指针属性标签 `_get_crc.pointer_dereference.*`，以及 `assertions` 正则对 CBMC/TLC 日志格式的匹配；升级 CBMC/TLC 后标签或横幅变化会让负例/断言先失败，这是有意的（版本 pin 也要求同步更新）。
- vendored jar 让仓库多 4.5 MB 二进制；升级要同时改 `formal/tools/`、manifest 的 `tool`/`jar_sha256`、README 与证据。CI 的 Java 由 apt 安装 `openjdk-17-jre-headless`，只 pin major 17 与发行版，不 pin 补丁级哈希；门禁会拒绝非 17 的 JRE。
- 所有分片在同一次 `check_formal.sh` 中串行执行，共用 300 s 总预算；新增 owner 若把总时长推过预算，要先扩预算或增加按 owner 选择，不能靠不登记来跳过。
- 本阶段未发现生产代码违反断言；若未来出现 finding，按接入细则 §5.6 记入 `docs/evidence/FORMAL-<ID>/` 并补“错误 → 守卫”。

## 证据

`docs/evidence/FORMAL-P0/`：summary、commands、runtime、negative-output、residual。
`docs/evidence/FORMAL-P1/`：summary、commands、runtime、正向/负向 TLC 输出、反例到代码步骤的映射、residual。
`docs/evidence/FORMAL-P2/`：summary、commands、runtime、residual，`strict-positive.txt`、`strict-negative.txt`、`sync-isolation-parity.txt`、`guard-selftest.txt`、`manifest-tests.txt`、`shard-proof.txt`、`red-cases.txt`、`toolchain.txt`、`vendored-fallback.txt`，以及最终 tip 的 `*-final.txt`、`arch-gates-final.txt`、`tip.txt`。
