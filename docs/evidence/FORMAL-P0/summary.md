# FORMAL-P0 证据包总结

- **任务**: FORMAL-P0 形式化验证 Phase 0（共享脚手架 + Modbus CBMC 试点）
- **分支/基线**: `worktree-formal-p0` @ `2bbf729e`（feat/iems tip）
- **状态**: 通过（本机 CBMC 5.12 严格模式）
- **产品代码改动**: 零

## 1. 交付

- `formal/README.md`、`formal/cbmc/modbus_decode.c`、`formal/manifest/iems.tsv`
- `ci/scripts/check_formal.sh`、`check_formal_negative.sh`、`check_formal_sync.py`、`check_formal_isolation.py`
- `.github/workflows/ci.yml` 的 `formal` job（PR 触发、CBMC 5.12 pin、10 分钟 timeout、日志上传）
- 所有 repo 级源码扫描 guard 通过 `ci/scripts/c_source_scan.py` 的共享 `SHARED_PRUNE_DIRS` 排除 `formal/`
- `run_arch_gates.sh` 与 CI 架构门禁步骤包含 formal（本地非严格，CI `formal` job 严格）

## 2. 门禁结果（本机实测）

| 检查 | 结果 | 耗时 |
| --- | --- | --- |
| `check_formal.sh --strict` | rc=0，1012 properties，CBMC 5.12 版本+哈希通过 | 44–85 s（含辅助运行） |
| `check_formal_negative.sh --strict` | 脚本 rc=0（判定通过）；变异副本 CBMC rc=10，命中 `_get_crc.pointer_dereference.*: FAILURE` | 5 s |
| `check_formal_sync.py` | rc=0，1 个工件，0 warning | <1 s |
| `check_formal_isolation.py` | rc=0，产品构建图/源码零引用 | <1 s |

unwind 三档：0..16 / 17 → 4 s；0..64 / 65 → 13 s；0..260 / 261 → 44–85 s；property 总数均为 1012。主门禁取 0..260。

工具 pin 复核：从 archive.ubuntu.com 下载的 `cbmc_5.12-5_amd64.deb` SHA256 = `be7e0491...94155a`（与规范一致）；解包出的 `/usr/bin/cbmc` SHA256 = `ef72f838...c111ee0d`，与本机安装一致。

## 3. 回归验证

| 检查 | 结果 | 耗时 |
| --- | --- | --- |
| `ctest --output-on-failure`（354 用例，含 `app_platform_e2e`） | 354/354 passed | 326.92 s |
| `python3 ci/scripts/run_guard_selftest.py` | rc=0（含 `check_formal_isolation.py` 正反夹具与 dropped formal job 夹具） | 38 s |
| `python3 ci/scripts/check_ci_gate_parity.py --root .` | rc=0（15 arch + 4 formal gates） | <1 s |
| `./ci/scripts/check_formal.sh --strict`（提交后复跑） | rc=0，1012 properties | 45 s |
| `./ci/scripts/check_formal_negative.sh --strict`（提交后复跑） | 脚本 rc=0；CBMC rc=10 | 5 s |
| `./ci/scripts/run_arch_gates.sh build 2bbf729e` | rc=0 | 55 s |

### 3.1 负例清洁检查修复（arch gates 内假红）

`run_arch_gates.sh` 在仓库根没有 `build` 时创建 `build` 符号链接，供硬编码 `<root>/build` 的 `check_binary_sizes.sh` 使用。`.gitignore` 的 `build*/` 只匹配目录，链接因此被 `git status --porcelain` 记为 `?? build`，负例在 CBMC 判定通过后误报 worktree 不干净。修复：`.gitignore` 增加 `/build` 覆盖根链接；负例的清洁检查使用 `git status --porcelain --untracked-files=normal`，启动时在 `mktemp -d` 临时仓库套用本仓库 `.gitignore` 自检（仅有 `build` 链接算干净，真实未跟踪文件算脏）。删掉 `/build` 的变异副本会在自检第一步失败。

| 场景 | 结果 |
| --- | --- |
| 正常 worktree，无 `build` 链接 | 负例 rc=0（4 s） |
| 仓库根仅有 gate 创建的 `build` 链接 | 负例 rc=0（3 s）；其他 `git status` 消费者也不再看到 `?? build`（修复前 rc=1） |
| 仓库根有真实未跟踪文件 | 负例 rc=1，打印该文件 |
| `.gitignore` 删掉 `/build` 的变异副本 | 自检 rc=1：`gate build link is not ignored by .gitignore` |
| `./ci/scripts/run_arch_gates.sh build 2bbf729e` | rc=0（83 s） |
| `./ci/scripts/run_arch_gates.sh <仓库外构建目录> 2bbf729e`（仓库根无 `build`，由 gate 创建链接） | rc=0（62 s） |
| `./ci/scripts/check_formal.sh --strict`（修复后） | rc=0，1012 properties（39 s） |
| `python3 ci/scripts/run_guard_selftest.py`（修复后） | rc=0（8 s） |
| `ctest --test-dir build --output-on-failure`（修复后） | 354/354 passed（276.42 s） |

早先 `check_formal_isolation.py` 会扫描仓库内构建目录，worktree 路径含 `formal` 时会把 CMake 生成文件里的绝对路径误报为产品引用。该缺陷已修：guard 改为只枚举 `git ls-files` 的受跟踪文件。复验：路径含 `formal` 的临时仓库放入未跟踪的 `build-ci/CMakeFiles/CMakeSystem.cmake`，rc=0；同一仓库向受跟踪源码加入 `#include "formal/..."`，rc=1 并打印文件行号。链接场景仍用仓库外构建目录验证。

## 4. 结论与残余

- 未发现生产解码器违反断言；无 finding 需要开守卫。
- 证明为有界证明：`in->len <= 260`、`hi_crc = false`、单次调用。
- 负例 anchor 是 CBMC 自动指针属性（不是 harness 断言）：删掉内层长度守卫后，全局长度检查仍会拒绝该帧，成功路径断言不可能失败，但越界读已经发生，CBMC 在 `_get_crc` 报指针越界。细节见 `residual.md`。

## 5. 证据索引

- 命令：`commands.md`
- 实测时间：`runtime.md`
- 负例输出：`negative-output.txt`
- 残余与 assumptions：`residual.md`
