# AGY 第一轮审核反馈（Codex Review Round 1）

> **Mr. WKT** — 审核对象：`feature/modular-refactor` 上 2026-08-01 合入的 T-002~T-106 共 11 个 Ticket + 进行中的 T-107。审核方式：逐提交代码审查 + 干净 worktree 独立构建与实测。

---

## 0. 总体结论

方向正确、代码可编译、CTest 全绿，但**当前不能继续合并**：

1. **P0 阻断（流水线级）**：干净构建后 `go/pkg/tags/tags.go` 会被 DCU 生成物覆盖成 6 条，导致 `go test ./...` 编译失败、`check_tag_consistency.sh` 失败。这是多产品共用单文件 `tags.go` 的生成器覆盖问题（T-204 范畴），在你们这套合入里第一次被完整暴露。
2. **T-101/T-102 只改了测试，没有修复产品代码**，也没有 Omnis 复跑证据，DoD 未达成。
3. **T-002/T-003 有安全红线级残留**（见下），不能视为完成。
4. **T-107 工作区改动不可提交**：`cmake/EdgeContract.cmake` 的改法会删除 `config.cmake` 的 include、在 configure 期提前执行编译器、且只对 smart_box 复制 tags.go，属于错误修法。
5. 仓库里没有任何 AGY 提交的逐 Ticket 证据文档（DoD 勾选、测试输出）。

我方实测（干净 worktree `/tmp/edgeos-review-agos`，基于 `20cd4da`）：

| 检查项 | 结果 |
|---|---|
| `cmake --build`（439 targets） | ✅ 通过 |
| `ctest`（127 项） | ✅ 127/127 通过 |
| `go test ./...`（干净构建后） | ❌ 编译失败（tags.DOIP_CLIENT_CONNECTED 等未定义） |
| `scripts/check_tag_consistency.sh` | ❌ SmartBox MISMATCH（C=62 vs Go=6）、IEMS OVERFLOW |
| `scripts/check_repo_clean.sh`（在仓库根外运行） | ⚠️ 因 cwd 非仓库根而“空转通过” |
| 干净 configure（无手工链接） | ❌ 失败：`.local/Linux-x86_64` 大小写不匹配（独立问题，归 T-401） |

## 1. Ticket 逐项结论

| Ticket | 结论 | 一句话 |
|---|---|---|
| T-002 UDS/DoIP 加固 | **Fail（需返工）** | RBAC 已补，但 seed-key 是 XOR 可逆算法、全局锁定位可被 DoS、DTC/ECU 数据仍硬编码、状态未写 DataHub、TCP 分帧未处理 |
| T-003 硬编码密钥外置 | **Fail（需返工）** | 环境变量化方向对，但缺密钥时回退 `TEST_ONLY_*` 默认值，违反“生产缺密钥必须 fail-fast” |
| T-004 仓库卫生 | **Pass-with-notes** | 二进制/DB/tgz 已清，但 tracked `.wasm` 仍有 5 个（脚本只禁 `rehab_companion.wasm`）；脚本应在仓库根执行 |
| T-005 增量收口 | **Pass** | 已提交 tags.go 与编译器生成的 smart_box tags.go **逐字节一致**；radar 算子属 T-303 范围，注意默认不要启用 |
| T-006 合并策略 ADR | **Pass-with-notes** | ADR 已建，但声称 “10 E2E”（实际 9 项），且这些门禁在合入前并未执行 |
| T-101 多点一致性 | **Fail** | 只改测试（加 0.1s sleep、loader 回退），无产品根因修复、无 Omnis 3 次全绿证据 |
| T-102 ADC→保护 E2E | **Fail** | 只加 try/finally 收尾，继电器未动作的产品问题未修复 |
| T-103 WAL/init 并发 | **Pass-with-notes** | replay 已加锁、空表检查已补；但 `datahub_init_from_fd` 失败路径泄漏未修，无失败注入测试 |
| T-104 压实锁内 I/O | **Pass-with-notes** | 拷贝出锁+tmp+rename 方向正确；缺 `fsync`，缺 p99 与 kill -9 恢复证据 |
| T-105 FIFO 唤醒 | **Pass-with-notes** | `O_RDWR|O_NONBLOCK` 可行；缺“C 先启动、Go 后挂载”的真实唤醒 E2E |
| T-106 Arbiter 生命周期 | **Pass-with-notes** | destroy 已清理队列；但清理回调在锁内执行有死锁风险，缺 executor 路径 double-free 回归与 TSan |
| T-107 Sanitizer 全量（WIP） | **Fail（未提交）** | 当前 EdgeContract.cmake 改动不可提交（见 §2） |

## 2. 必须修复的问题（按优先级）

### P0-1 tags.go 多产品覆盖（阻断一切合并）
- 现象：构建时 iems → smart_box → dcu 依次生成并把各自 `tags.go` `copy_if_different` 到 `go/pkg/tags/tags.go`，最后一个（dcu，6 条）覆盖全量。
- 要求：改成“每个产品独立 Go 包/目录”或“生成聚合 tags.go（合并全部产品）”，保证干净构建后 `go test ./...` 与 `check_tag_consistency.sh` 全绿。
- 禁止：用“只给 smart_box 复制”这种修法；`cmake/EdgeContract.cmake` 当前工作区改动（删除 config.cmake include、configure 期执行编译器）请丢弃重做。

### P0-2 T-101/T-102 必须重做
- 要求：定位产品侧根因（不是测试抖动），修复后 `./test.sh --omnis` 连续 3 次 9/9，并附每次输出；`test_iems_multi_tag_consistency` 不得靠 sleep 掩盖，`test_adc_to_protection_e2e` 必须让继电器真实动作。

### P0-3 T-002 安全整改
- seed-key 改为不可逆/标准算法（如按 seed 派生 HMAC），或明确标注为演示算法并默认关闭；锁定位按客户端隔离，不得全局 3 次失败锁死所有人。
- UDS 会话/DTC/Routine/DoIP 状态必须通过 DataHub API 写契约 Tag（12031-12035），禁止只写 `tag_history` 表或全局变量当真相。
- DTC/ECU 标识、例程结果禁止硬编码，改为契约/配置数据或真实状态机；至少对未接真实 ECU 的场景返回“无数据/未连接”。
- DoIP 处理 TCP 粘包/半包：单次 read 可能含多个帧、一帧可能跨多次 read；补帧缓冲与长度上限校验测试。
- 补充测试：连接数上限、超长 payload 拒绝、连续 3 次错 key 后锁定、部分帧/多帧输入、DataHub 回写断言。

### P0-4 T-003 缺密钥必须 fail-fast
- 删除 `TEST_ONLY_*` 回退；`EDGE_GUARD_AUDIT_SECRET`/`VPP_AUDIT_SECRET`/`DEMO_ALGO_API_KEY` 缺失时：
  - 生产模式（新增 `EDGE_ENV=production` 或等价开关）→ 启动失败；
  - 开发模式 → 打印明确警告并仅在测试构建中允许默认值。
- 补“缺密钥时拒绝启动”的测试。

### P1 核心修复补强
- T-103：修 `datahub_init_from_fd` 失败路径（ring 已映射时 munmap/close）；用 mock/注入使 ftruncate/mmap 失败可测。
- T-104：`fflush` 后加 `fsync(fileno(fp))` 再 rename；提供压实期间写延迟 p99 数据与 kill -9 恢复用例。
- T-105：写一个“C 创建 FIFO 并写入 → 后启动 Go watcher → 收到事件”的 E2E 测试。
- T-106：`arbiter_destroy` 的 on_cleanup 移出锁外（收集后统一释放）；补 executor.on_io_done 处理过的消息不被重复 free 的回归测试；跑 TSan。
- T-004：`check_repo_clean.sh` 开头 `cd "$(dirname "$0")/.."` 或改用 `git -C`；明确 5 个测试/示例 `.wasm` 是保留白名单还是删除，写入脚本注释与 DoD。

### 附加发现（新建小 Ticket，或并入 T-401）
- `CMakeLists.txt` 的 `LOCAL_ROOT` 用 `Linux-x86_64`，实际目录是 `linux-x86_64`，干净 configure 必失败；请归一化大小写或创建兼容链接并写进文档。
- ADR-0003 中 “10 E2E” 改为实际数量（当前 9 项）。

## 3. 下一轮提交要求

1. 每个 Ticket 附 `docs/plans/2026-08-01-AGY-review-feedback.md` 对应的整改说明 + DoD 勾选 + 测试日志（ctest/go/omnis）。
2. 干净 clone（或干净 worktree）上按顺序执行：`./build.sh` → `./test.sh` → `cd go && go test ./...` → `./scripts/check_tag_consistency.sh` → `./scripts/check_repo_clean.sh`，全部绿。
3. P0/P1 未通过 Codex 复审前，不得向 `feature/modular-refactor` 继续合并。

---

*Codex 复审将按本反馈逐项核对；通过标准 = 反馈项全部闭环 + 干净构建全绿。*
