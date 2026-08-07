# EdgeOS / EdgeC Ticket 路线图与实施计划（AGY 交接版）

> **Mr. WKT** — 本文档基于 2026-07-31 深度调查报告（`.agents/memory/edgeos-deep-investigation-report.md`）生成，用于交给 AGY 分 ticket 实施，并由 Codex 按本文“审核与验收协议”逐项审查完成度与质量。

---

## 0. 使用说明

1. **AGY 工作方式**：每个 Ticket 独立分支（`ticket/T-XXX`）→ 实现 → 自测 → 提交 PR/MR → 附测试证据与变更摘要；P0/P1 Ticket 未经审核不得合并。
2. **审核方式**：Codex 对每个 Ticket 执行“完成度核对 + 代码审查 + 实测验证 + 回归检查”，输出审核结论（Pass / Pass-with-notes / Fail）。
3. **排序依据**：先止血（安全/已知失败），再修地基（DataHub/契约），后上护栏（CI），最后做产品闭环；文档治理贯穿始终。
4. **未确认事项**：见 §6，AGY 遇到 §6 中未定项必须先停下提问，不得擅自扩大范围。

## 1. 总览（6 个阶段，41 个 Ticket）

| 阶段 | 主题 | Ticket | 优先级 | 目标周期 |
|---|---|---:|---:|---|
| Phase 0 | 安全与仓库止血 | T-001 ~ T-006 | P0 | Week 1 |
| Phase 1 | 核心质量修复（DataHub/Arbiter/Matrix） | T-101 ~ T-107 | P0/P1 | Week 1-3 |
| Phase 2 | 契约与点表治理 | T-201 ~ T-206 | P1 | Week 3-4 |
| Phase 3 | Zero-Mock 真实性改造 | T-301 ~ T-305 | P1 | Week 4-6 |
| Phase 4 | CI/CD 与工程护栏 | T-401 ~ T-407 | P1/P2 | Week 5-7 |
| Phase 5 | 产品闭环 | T-501 ~ T-506 | P2 | Week 6-9 |
| Phase 6 | 文档与治理 | T-601 ~ T-604 | P2 | 贯穿 |

优先级定义：P0=阻塞交付/安全红线；P1=质量与架构底线；P2=产品增强与体验。

### 1.1 已确认决策与三 Agent 并行分工（2026-08-01 更新）

已确认：以本文档为 Ticket 唯一权威（暂不建 GitHub Issues）；UDS/DoIP 保留并加固；Zephyr 不在本期范围（归 `/workspaces/applications` 管理）；由 3 个 agent 并行实施。

| Agent | 职责流 | Ticket 范围 | 关键交接 |
|---|---|---:|---|
| Agent-A | 安全与 Zero-Mock | T-001~T-006 → T-301~T-305 | T-001 新密钥/签名链 → Agent-C（T-506） |
| Agent-B | 核心可信与契约 | T-101~T-107 → T-201~T-206 | T-101/T-102 修复 → Agent-C（T-403）；T-201 → T-202~T-205 |
| Agent-C | CI/产品/治理 | T-401~T-407 → T-501~T-506 → T-601~T-604 | 接收 Agent-A/B 交付物后启用全量门禁 |

并行规则：
1. 每个 Agent 独立分支（`ticket/agent-<name>/T-XXX`）；同一文件冲突由 Agent-C 领衔协调。
2. 共享文件（CMake/CI/契约 schema）改动必须最小化并当日同步。
3. P0（Phase 0）不因并行而降级：Agent-A 的 T-001/T-002 优先于其后续任务。
4. 每个 Ticket 完成后按 §4 协议提交证据，Codex 逐项审核；审核不过的 Ticket 不得进入合并队列。

## 2. Ticket 模板与字段

每个 Ticket 必须包含以下字段；AGY 提交前逐项自检（勾选 DoD）。

```
### T-XXX 标题
- Epic / Priority / Estimate / Depends
- 背景（为什么做，来自调查报告哪一节）
- 问题证据（文件、行号、日志、命令输出）
- 目标与要求（做什么，不做什么）
- 实现要点（方案约束，符合 EdgeOS 宪法）
- 涉及文件
- 完成标准 DoD（勾选项）
- 验证命令
- 审核清单（Codex 审查时逐项核对）
```

---

# Phase 0 — 安全与仓库止血（P0）

### T-001 轮换并清除 Git 历史中的 WASM 签名私钥
- Epic: 安全基线 | Priority: P0 | Estimate: L | Depends: 用户授权 | **状态: 暂缓（用户 2026-08-01 指示“先不管它”，Agent-A 先执行 T-002 起；T-506 的签名依赖顺延）**
- 背景: `keys/edgec_signing_private.pem` 已被 Git 跟踪（调查报告 §9.1），私钥一旦外泄，WASM 签名链与 OTA 信任根失效。
- 问题证据: `git ls-files keys` 输出包含 `edgec_signing_private.pem`；最近写入提交 `cac62df`（2026-07-08）。
- 通俗解释: 私钥已留在 git 历史里；“清历史”= 重写提交记录并强制推送（影响所有协作者）。不重写则走替代方案：轮换新密钥并从当前版本移除，旧私钥视为已泄露。
- 目标与要求:
  - 离线生成新 RSA 密钥对；新私钥只放入受保护部署环境，仓库内只保留公钥。
  - 从 HEAD 与全部历史中移除私钥（`git filter-repo`/BFG）——需用户授权（§6 Q1）；不授权则改为“轮换 + 移除当前版本 + 声明旧密钥泄露”。
  - 重新签名全部受管 `.wasm`（`rehab_companion/game_feedback/ota_verifier` 及 `algo/wasm/demo`），更新 `.sig`。
  - 更新 `build.sh make-keys` 流程：默认生成到仓库外目录并提示“禁止提交私钥”。
  - 增加密钥泄露检测（配合 T-405 的 gitleaks）。
- 涉及文件: `keys/`、`build.sh`、`algo/wasm/**/*.wasm.sig`、CI 配置。
- 完成标准 DoD:
  - [ ] 新公钥/私钥对已生成，仓库中不存在任何私钥文件（`git grep` 与 `git rev-list --objects --all` 双重验证）。
  - [ ] 全部 `.wasm` 使用新私钥重新签名并通过验签测试（`test_wasm_signature`）。
  - [ ] `build.sh make-keys` 不再把私钥写进仓库目录。
  - [ ] gitleaks 扫描通过；README 增加密钥轮换说明。
- 验证命令: `git rev-list --objects --all | grep -i private`；`cd EdgeC && ./test.sh`；`gitleaks detect --source .`
- 审核清单: 历史是否真清干净；新密钥是否妥善外置；签名链测试是否真实通过；force-push 是否经用户确认。

### T-002 SmartBox UDS/DoIP 安全加固（未提交增量）
- Epic: 安全基线 | Priority: P0 | Estimate: M | Depends: -
- 背景: 工作区未提交的 `go/cmd/smart-box-gateway/main.go` 新增 DoIP/UDS 端点全部无鉴权，SecurityAccess 固定 seed、任意 key 可解锁，且移除了 OTA check 的 RBAC（调查报告 §9.2）。
- 问题证据: `git diff` 中 `startDoIPServer/handleDoIPConnection/processUDSService/handleUDSSession/handleUDSDTC/handleUDSSecurity/handleUDSRoutine/handleDoIPStatus`；`http.HandleFunc("/api/v1/device/ota/check", gateway.handleDeviceOTACheck)`。
- 目标与要求:
  - 恢复 `/api/v1/device/ota/check` 的 RBAC（RoleUser）。
  - 新增 5 个 UDS/DoIP REST 端点全部挂 RBAC + 审计日志。
  - DoIP `SecurityAccess(0x27)`：随机 seed、尝试次数限制、失败锁定窗口；禁止“任意 key 解锁”。
  - DoIP 帧解析增加最大 payload 长度（如 4096）、协议版本校验、超时与连接数上限。
  - UDS 会话/Routine/DTC 状态写回契约 Tag（12031-12035）必须通过 DataHub API，禁止全局变量直接当真相。
  - 为上述逻辑补 Go 单元测试（未授权 401、错误 key 403、超长 payload 拒绝、解锁限流）。
- 涉及文件: `go/cmd/smart-box-gateway/main.go`、`main_test.go`、`go/pkg/tags/tags.go`、`contracts/products/smart_box.yaml`。
- 完成标准 DoD:
  - [ ] 无 token 访问新端点全部 401；低权限角色访问敏感操作 403。
  - [ ] OTA check 恢复 RBAC 并有测试。
  - [ ] SecurityAccess 无法用固定/猜测 key 解锁；连续失败触发锁定。
  - [ ] DoIP 超长/畸形帧被拒绝且连接数受限。
  - [ ] `go test ./...` 全绿；新增用例覆盖上述场景。
- 验证命令: `cd EdgeC/go && go test ./...`；`curl -i http://localhost:8083/api/v1/ecu/uds/session`（无 token 应 401）。
- 审核清单: RBAC 是否真正生效；seed-key 是否安全；有无绕过路径；新增测试是否真实断言而非仅状态码。

### T-003 移除/外置硬编码密钥与口令
- Epic: 安全基线 | Priority: P0 | Estimate: S | Depends: -
- 背景: 源码中存在 3 处硬编码密钥/口令（调查报告 §9.3）。
- 问题证据: `stage_vpp_logger.c:66` `"vpp-audit-secret-2026"`；`secure_syslog_exporter.c:27` `"edge-guard-audit-secret-key-2026"`；`go/cmd/demo-algo/main.go:16` `"secret-token-123"`。
- 目标与要求:
  - HMAC/审计密钥改为配置或环境变量注入；生产模式缺密钥必须 fail-fast，禁止静默降级。
  - demo-algo 的 API Key 改为环境变量，示例文档同步。
  - 增加“密钥不得硬编码”的 CI 正则扫描（与 T-405 合并）。
- 涉及文件: `modules/edge_stream/src/stage_vpp_logger.c`、`modules/edge_security/src/secure_syslog_exporter.c`、`go/cmd/demo-algo/main.go`、配置文件。
- 完成标准 DoD:
  - [ ] `rg -n 'secret|api_key|password' modules go cmd --glob '!*.md' --glob '!*.yaml'` 不再出现业务硬编码值（测试 fixture 除外并显式标注）。
  - [ ] 配置缺失时程序启动失败并有明确错误日志。
  - [ ] 相关单元测试使用注入的测试密钥。
- 验证命令: `rg` 扫描；`go test ./...`；`ctest`。
- 审核清单: 是否仍有隐蔽硬编码；缺配置时是否 fail-fast；文档是否同步。

### T-004 仓库产物卫生：清除误入库的二进制与 DB
- Epic: 仓库治理 | Priority: P0 | Estimate: M | Depends: -
- 背景: Go 编译产物（`go/edge-gateway`、`go/edge-api-core`、`go/ui-backend`、`go/cmd/*/` 下 ELF）、WASM、DB 等被 Git 跟踪（调查报告 §9.8）。
- 问题证据: `git ls-files` 显示 `go/cmd/edge-api-core/edge-api-core`、`go/cmd/edge-gateway/edge-gateway` 等 ELF；`.gitignore` 允许 `*.a/*.so` 与 `.local/`（vendored 依赖属有意为之）。
- 目标与要求:
  - 从跟踪中移除构建产物（ELF、`*.db*`、`*.tgz`、`*.wasm`、临时文件），保留 vendored 静态库策略（需文档化）。
  - `.gitignore` 补充 `go/*` 二进制、`*.wasm`（受管签名产物走 CI 发布）、`*.tgz`、`*.db*`。
  - CI 增加“仓库内容白名单”检查：`git ls-files` 中不允许出现 ELF/DB/TGZ。
  - 新增文件必须从源码构建或由 CI 发布，仓库内不提交可执行产物。
- 涉及文件: `.gitignore`、CI、`git rm --cached` 清单。
- 完成标准 DoD:
  - [ ] `git ls-files | grep -E '\.(db|tgz|wasm)$'` 为空；ELF 白名单为空。
  - [ ] CI 白名单检查 job 存在且失败即阻断。
  - [ ] 干净 checkout + `./build.sh` 可复现全部产物。
- 验证命令: `git ls-files | grep -E '\.(db|tgz|wasm)$'`；`file` 抽查；CI job。
- 审核清单: 是否误删了有意 vendored 的库；构建能否从源码复现；CI 检查是否先失败后修复。

### T-005 收口未提交的 SmartBox 增量
- Epic: 仓库治理 | Priority: P0 | Estimate: S | Depends: T-002
- 背景: 当前分支工作区有 3 个修改文件 + 多个未跟踪文件（调查报告 §10）。
- 问题证据: `git status --short` 显示 `contracts/products/smart_box.yaml`、`go/cmd/smart-box-gateway/main.go`、`go/pkg/tags/tags.go` 修改；未跟踪 3 个新域 YAML、`radar_tinyml_operator.c`、发布 tgz。
- 目标与要求:
  - T-002 完成后，将 UDS/DoIP/雷达/FOC 增量作为一个有测试、有契约、有文档的提交合入 `feature/modular-refactor`（或按用户决策回退）。
  - 新域 YAML（`smartbox_radar/industrial_foc/automotive_uds`）纳入 `contracts/` 并确保编译器可生成；`tags.go` 必须是生成物且与 YAML 一致。
  - 发布 tgz 移出工作区（由 CI 生成），不提交。
- 涉及文件: 上述全部。
- 完成标准 DoD:
  - [ ] `git status` 干净（除有意保留项）。
  - [ ] `build.sh` + ctest + `go test ./...` 全绿。
  - [ ] 新域标签在 `build/products/smart_box/generated/contracts.yaml` 中可见且与 `tags.go` 一致。
- 验证命令: `git status`；`scripts/check_tag_consistency.sh`；ctest。
- 审核清单: 提交信息是否对应 Ticket；生成物与手动物是否混入；是否有未测试代码混入。

### T-006 分支合并策略：feature/modular-refactor → main
- Epic: 仓库治理 | Priority: P0 | Estimate: M | Depends: T-101~T-107 完成
- 背景: 当前在 `feature/modular-refactor` 且领先 origin；`main` 长期未同步（调查报告 §3/§10）。
- 问题证据: `git branch -a`；skill backlog 明确“Final merge of modular refactor branch back into main”。
- 目标与要求:
  - 定义并文档化合并门禁：P0/P1 完成、ctest/go/Matrix 全绿、无未提交改动、文档同步。
  - 合并策略采用非快进 PR（feature 分支 → main），保留审核记录。
  - 合并后立即打基线 tag（如 `v0.9.0-modular`），CI 全量验证。
- 涉及文件: 仓库分支、`docs/adr/`、CI。
- 完成标准 DoD:
  - [ ] main 与 modular-refactor 无功能差异；PR 记录完整。
  - [ ] main 上全量验证（ctest+go+omnis）通过。
  - [ ] 合并策略文档化（CONTRIBUTING 或 ADR）。
- 验证命令: `git log main..feature/modular-refactor`；CI 状态。
- 审核清单: 是否有未完成 Ticket 被带入 main；历史是否被无谓重写。

---

# Phase 1 — 核心质量修复（P0/P1）

### T-101 修复 Matrix 多点一致性测试（test_iems_multi_tag_consistency）
- Epic: Matrix 质量 | Priority: P0 | Estimate: M | Depends: -
- 背景: Omnis 历史日志中该用例 FAILED（调查报告 §8）。
- 问题证据: `omnis_test_results.log`：注入多值后 `VOLTAGE_PHASE_A=220.5` 通过但整体 FAILED，具体失败标签未打印。
- 目标与要求:
  - 复现并定位根因（疑似 shadow 写时序/多点原子性/轮询覆盖）。
  - 增强测试日志：断言全部标签并输出每个标签实际值。
  - 修复后 `--omnis` 9/9 全绿并保持稳定（连跑 3 次）。
- 涉及文件: `cmd/iems/matrix/test_e2e_flow.py`、`modules/edge_core/src/datahub.c`、`modules/edge_io/src/engines/master_engine.c`。
- 完成标准 DoD:
  - [ ] 根因写入手册/注释；测试输出每个标签断言结果。
  - [ ] `./test.sh --omnis` 连续 3 次 9/9 通过。
- 验证命令: `cd EdgeC && ./test.sh --omnis`。
- 审核清单: 修复是否只改测试而不修产品；是否掩盖时序问题；是否引入 sleep 类脆弱断言。

### T-102 修复 ADC→继电器保护 E2E（test_adc_to_protection_e2e）
- Epic: Matrix 质量 | Priority: P0 | Estimate: M | Depends: -
- 背景: 历史日志显示 RMS 已检测到 278.54V，但 Reflex 保护未触发、继电器未动作（调查报告 §8）。
- 问题证据: `omnis_test_results.log`：`❌ Protection failed to trigger or relay did not respond`。
- 目标与要求:
  - 定位保护管线未触发点（pipeline trigger、阈值、DataHub 写回、继电器驱动）。
  - 增加端到端时序日志（RMS 变化 → pipeline 触发 → 指令下发 → relay 状态）。
  - 修复后与 T-101 一起 9/9 稳定通过。
- 涉及文件: `modules/edge_stream/src/stream_engine.c`、`modules/edge_io/src/engines/master_engine.c`、`cmd/iems/main.c`、`cmd/iems/matrix/test_high_freq.py`。
- 完成标准 DoD:
  - [ ] 过压→保护动作闭环日志完整；连续 3 次 omnis 全绿。
  - [ ] 新增一个针对“检测到过压但未动作”的回归断言。
- 验证命令: `./test.sh --omnis`；`tail build/matrix_edgec.log`。
- 审核清单: 保护是否真实执行（非模拟）；阈值/死区是否符合契约；是否绕过 DataHub 直改状态。

### T-103 DataHub WAL/初始化并发修复（审计 C1/C3/C4）
- Epic: DataHub | Priority: P0 | Estimate: M | Depends: -
- 背景: 调查报告 §4.2 确认 2026-07-23 审计的并发问题仍在。
- 问题证据: `datahub_replay_wal()` 无锁写 shadow；`datahub_init_from_fd()` shadow mmap 失败时 ring 已映射未回收；`compact_now` 对 save 结果处理不完整。
- 目标与要求:
  - `replay_wal` 全程持 `shadow_lock`（或引入独立 replay 锁）并校验 tag 映射。
  - `init_from_fd` 所有失败路径统一回滚（munmap/close/清理）。
  - `compact_now` 检查每一步返回值并记录失败计数/日志。
  - 新增单元测试：并发 replay+read；失败注入（mmap/ftruncate 返回失败）；重复 init/destroy 无泄漏（ASan）。
- 涉及文件: `modules/edge_core/src/datahub.c`、`tests/test_datahub.c`、`tests/test_datahub_shadow.c`。
- 完成标准 DoD:
  - [ ] ctest 新增用例全绿；ASan 下无 leak/UAF。
  - [ ] 代码评审确认所有失败路径已回滚。
- 验证命令: `ctest -R test_datahub`；`./test.sh --asan`。
- 审核清单: 锁粒度是否合理（不引入长时间持锁）；失败注入是否真实；是否有重复解锁/泄漏。

### T-104 DataHub 压实持锁 I/O 优化
- Epic: DataHub | Priority: P1 | Estimate: M | Depends: T-103
- 背景: `datahub_save_snapshot()`/`compact_now()` 在 shadow_lock 内做 fwrite/文件操作（调查报告 §4.2）。
- 问题证据: `datahub.c` L247-263、L315-334。
- 目标与要求:
  - 快照写入临时文件后原子 rename；写盘期间不持 shadow_lock（用版本号/RCU 或分块快照）。
  - WAL truncate 在锁外完成，并保证“先快照后清 WAL”的崩溃一致性。
  - 增加压实期间写入延迟指标（可复用 metrics 12001-12004）。
  - 性能测试：压实过程中 10kHz 写入不阻塞。
- 涉及文件: `modules/edge_core/src/datahub.c`、`datahub.h`、`tests/test_datahub_dynamic_chunk.c`。
- 完成标准 DoD:
  - [ ] 压实期间写延迟 p99 不劣化（提供基准数据）；崩溃恢复测试（kill -9 后 WAL+快照重建一致）。
  - [ ] 新测试覆盖“压实中写入”与“压实失败后 WAL 不丢”。
- 验证命令: `ctest`；手工 kill -9 恢复脚本。
- 审核清单: 原子性是否真正保证；是否有内存拷贝代价过高；指标是否真实。

### T-105 ShmRing FIFO 唤醒时序修复
- Epic: DataHub | Priority: P1 | Estimate: M | Depends: -
- 背景: `shmring_create()` 立即以 `O_WRONLY|O_NONBLOCK` 打开 FIFO，无读者时 `ENXIO` 失败导致 event 通道失效（调查报告 §4.2）。
- 问题证据: `shmring.c` L96-124；`watcher.go` 依赖 `/tmp/edgeos_ev_%s`。
- 目标与要求:
  - 方案 A（推荐）：改用 `eventfd`（内核级，无需读者先于创建者）。
  - 方案 B：FIFO 延迟打开 + 重试/通知协议；需证明无丢失。
  - 无论哪种方案，C 写入后 Go watcher 必须被唤醒（E2E 测试：启动顺序 C 先、Go 后）。
  - 保留旧 API 兼容（`shmring_get_event_fd`）。
- 涉及文件: `modules/edge_core/src/shmring.c`、`shmring.h`、`go/pkg/shm/watcher.go`、测试。
- 完成标准 DoD:
  - [ ] C 先启动、Go 后挂载的用例下事件不丢；`test_shmring` + Go watcher 测试通过。
  - [ ] 无读者时创建不失败、无 ENXIO 噪声。
- 验证命令: `ctest -R shmring`；`go test ./pkg/shm`；手动启动时序脚本。
- 审核清单: eventfd 数量/生命周期管理；读端关闭后写端行为；是否影响 Zephyr 构建。

### T-106 Arbiter 队列生命周期与 double-free 修复
- Epic: 控制平面 | Priority: P1 | Estimate: M | Depends: -
- 背景: skill backlog 记录 Arbiter 高负载回调并发 double-free/SIGSEGV；`arbiter_destroy()` 不释放队列/zombie（调查报告 §4.3）。
- 问题证据: `arbiter.c` L161-165；`cmd_msg_t` 生命周期由 executor/回调共同释放。
- 目标与要求:
  - 明确 `cmd_msg_t` 所有权：提交者/执行者/清理回调各阶段唯一归属，禁止双重 free。
  - `arbiter_destroy()` 释放全部队列与 zombie；pause 状态下可安全销毁。
  - 编写高负载压力测试（多线程 submit + 回调互斥释放 + 随机 pause/resume），ASan/TSan 全绿。
- 涉及文件: `modules/edge_core/src/arbiter.c`、`arbiter.h`、`tests/test_arbiter.c`。
- 完成标准 DoD:
  - [ ] 压力测试 30 秒无 crash/leak（ASan）；TSan 无数据竞争。
  - [ ] destroy 后无悬挂回调；新增生命周期文档。
- 验证命令: `ctest -R test_arbiter`；`./test.sh --asan`；TSan 构建。
- 审核清单: 所有权是否单一明确；是否引入锁内回调；压力测试是否可复现旧 bug（先红后绿）。

### T-107 Sanitizer 全量验证并纳入 CI
- Epic: 质量护栏 | Priority: P1 | Estimate: M | Depends: T-103/T-106
- 背景: task.md 声称 ASan 完成，但 CI 未跑 sanitizer；TSan 路径存在 Arbiter 风险（调查报告 §8）。
- 问题证据: `test.sh` 有 `--asan`；`CMakeLists.txt` 有 `ENABLE_ASAN/ENABLE_TSAN`；CI 无对应 job。
- 目标与要求:
  - 建立 ASan job（全量 ctest，`detect_leaks=1`）与 TSan job（核心用例子集，因 TSan 与部分用例不兼容需白名单）。
  - 修复所有发现；不能修复的必须开 ticket 并禁用对应用例（带原因）。
  - 生成 artifact 报告。
- 涉及文件: CI、`test.sh`、`CMakeLists.txt`、测试白名单。
- 完成标准 DoD:
  - [ ] ASan 127/127 无告警；TSan 白名单用例全绿。
  - [ ] CI 有独立 sanitizer job 且失败阻断。
- 验证命令: `./test.sh --asan`；本地 TSan 构建。
- 审核清单: 是否有用例被悄悄跳过；sanitizer 发现的问题是否都有修复或显式追踪。

---

# Phase 2 — 契约与点表治理（P1）

### T-201 契约编译器校验增强
- Epic: 契约治理 | Priority: P1 | Estimate: L | Depends: -
- 背景: 编译器不校验 TagID 范围/名称唯一性/type 合法性/binding 引用（调查报告 §4.5）。
- 问题证据: `edgec-tag-system-audit.md`；`injectBinding()` 静默跳过未定义引用。
- 目标与要求:
  - 编译期校验：ID 范围与段、名称唯一、type ∈ 白名单、单位必填、binding 引用必须存在、include 必须存在。
  - 错误输出含文件/行/修复建议；`--strict` 默认开启。
  - 为 compiler 新增单元测试（`tools/contract-compiler-v2` 内，含正/反例）。
  - 生成器对“未知字段”报 warning（防拼写错误）。
- 涉及文件: `tools/contract-compiler-v2/internal/**`、`contracts/**`、测试。
- 完成标准 DoD:
  - [ ] 构造 10+ 个非法契约用例全部被拒；合法契约通过。
  - [ ] `go test ./...`（compiler 目录）全绿；`build.sh` 在坏 YAML 下失败并输出定位信息。
- 验证命令: `cd tools/contract-compiler-v2 && go test ./...`；手工坏 YAML。
- 审核清单: 校验规则是否覆盖调查报告列出的全部缺口；错误信息是否可操作；是否误伤现有合法契约。

### T-202 TagID 命名空间与 DCU OAD 治理
- Epic: 契约治理 | Priority: P1 | Estimate: M | Depends: T-201
- 背景: 常规 TagID 与 DCU Hex OAD 在同一 uint32 空间混用，`dcu_router.c` 硬编码双编码（调查报告 §4.5）。
- 问题证据: `dcu_router.c`、`contracts/bindings/dlt698/dcu_interface.yaml`。
- 目标与要求:
  - 定义并文档化 ID 段：业务 1000-19999、Metrics 12000-12999、DCU/OAD 独立段（或经编译器映射）。
  - 删除 `dcu_router.c` 硬编码 OAD 双编码，改由契约映射表生成。
  - 增加段冲突检测（编译器）+ 回归测试。
- 涉及文件: `contracts/**`、`modules/edge_io/src/dcu_router.c`、`cmd/dcu/**`、compiler。
- 完成标准 DoD:
  - [ ] DCU 构建与 2000 表仿真通过；无硬编码 OAD。
  - [ ] 段规则写入 `contracts/README.md` 并被编译器强制。
- 验证命令: `ctest -R dcu`；`rg '0x20000200|OAD' cmd modules --glob '!*.md'`。
- 审核清单: 是否破坏已有设备地址兼容；映射是否单源；是否有运行时迁移需求。

### T-203 修复未定义 Tag 引用并加全库校验
- Epic: 契约治理 | Priority: P1 | Estimate: S | Depends: T-201
- 背景: `EV_CHARGE_POWER`、`BATTERY_TEMP_MAX` 等引用未定义（调查报告 §4.5）。
- 问题证据: `contracts/bindings/dlms/generic_ess.yaml`、`contracts/bindings/can/catl_bms.yaml`。
- 目标与要求:
  - 补全缺失 Tag 定义或删除孤儿引用（以产品实际需要为准）。
  - CI 增加“全契约引用完整性”检查（编译器校验即覆盖）。
- 涉及文件: 上述 YAML 及引用方。
- 完成标准 DoD:
  - [ ] 全库无未定义 Tag 引用；`build.sh` 对全部产品契约通过。
- 验证命令: `rg -n 'EV_CHARGE_POWER|BATTERY_TEMP_MAX' contracts`；build。
- 审核清单: 补定义是否与产品语义一致（不引入假点表）。

### T-204 消除 C/Go 点表双源 drift
- Epic: 契约治理 | Priority: P1 | Estimate: M | Depends: T-201
- 背景: `go/pkg/tags/tags.go` 为生成物但被跟踪，历史上出现手工修改风险；`check_tag_consistency.sh` 非致命（调查报告 §4.5/§7）。
- 问题证据: `scripts/check_tag_consistency.sh`；`build.sh` 中 `|| echo "⚠️ non-fatal"`。
- 目标与要求:
  - 决策并落地：方案 A 生成物不入库（CI 构建时生成）；方案 B 入库但 CI 校验与生成结果一致。
  - `check_tag_consistency.sh` 改为 CI 强制（失败阻断）。
  - 清理 `tags.go` 手工编辑痕迹，全部由编译器生成。
- 涉及文件: `go/pkg/tags/tags.go`、`build.sh`、`scripts/check_tag_consistency.sh`、CI。
- 完成标准 DoD:
  - [ ] CI 中 C/Go 标签数量与内容一致；手工改 `tags.go` 会被 CI 拦截。
  - [ ] 文档说明生成物归属。
- 验证命令: 故意改 `tags.go` 一个条目 → CI 红；恢复后绿。
- 审核清单: 方案是否最小侵入；是否影响 IDE 开发体验（需文档）；是否误删用户本地生成物。

### T-205 契约版本化 / Blob 签名 / 热切换加固
- Epic: 契约治理 | Priority: P1 | Estimate: L | Depends: T-201
- 背景: ADR-0002 定义了 Blob 与热加载，`contract_instance_t` 已有雏形，但版本/签名/热切换缺少验证（调查报告 §4.5）。
- 问题证据: `contract_instance_t`、`datahub_switch_schema()`、`docs/adr/0002`。
- 目标与要求:
  - Blob 增加 schema_id/version/签名（复用 T-001 新密钥）；加载时验签。
  - 实现/验证 `pipeline_prepare/switch/reload` 原子切换；旧版本保留回滚。
  - 热切换测试：运行中替换契约，数据不中断、无泄漏。
- 涉及文件: `tools/contract-compiler-v2/internal/backend/blob_backend.go`、`modules/edge_core/src/contract_registry.c`、`modules/edge_stream/src/pipeline_loader.c`、测试。
- 完成标准 DoD:
  - [ ] 坏签名/坏版本 Blob 被拒绝并有审计日志；热切换用例通过（ASan）。
- 验证命令: `ctest -R pipeline_hot_reload`；新增 blob 测试。
- 审核清单: 签名链路是否与 T-001 一致；回滚是否真实可用；是否有进程重启兜底。

### T-206 契约编译器 V1 退役
- Epic: 契约治理 | Priority: P2 | Estimate: S | Depends: T-201~T-205
- 背景: `tools/contract-compiler/`（V1）与 `tools/contract-compiler-v2`（V2）并存（调查报告 §4.5）。
- 问题证据: 目录与 README 引用。
- 目标与要求:
  - 确认 V1 无生产引用后删除或标记 deprecated；更新所有脚本/文档引用。
- 涉及文件: `tools/contract-compiler/`、README、docs。
- 完成标准 DoD:
  - [ ] 全库无 V1 引用；文档说明仅 V2 受支持。
- 验证命令: `rg 'contract-compiler(?!-v2)' --pcre2`。
- 审核清单: 删除前是否确认无隐藏依赖（如旧脚本、CI、教程）。

---

# Phase 3 — Zero-Mock 真实性改造（P1）

### T-301 OPC UA 引擎真实化（或显式降级）
- Epic: Zero-Mock | Priority: P1 | Estimate: L | Depends: -
- 背景: `opcua_engine.c` 是显式 mock：无真实连接、write 直接回调（调查报告 §9.4）。
- 问题证据: `is_connected = true`、`// Mocked for interface implementation`。
- 目标与要求:
  - 方案 A（推荐）：用 open62541 实现真实 connect/browse/read/subscribe/write，Matrix 侧增加 OPC UA Actor；UaExpert 可读 `GRID_POWER`。
  - 方案 B（如短期无硬件/联调）：将 OPC UA 标记 EXPERIMENTAL，默认不编译，移除假连接，文档明确不支持。
  - 无论方案，禁止“假成功”状态。
- 涉及文件: `modules/edge_io/src/engines/opcua_engine.c`、CMake、契约（protocol: OPC_UA）、Matrix。
- 完成标准 DoD:
  - [ ] 方案 A：E2E 测试连接/订阅/读写通过；方案 B：无 mock 代码且文档标注。
  - [ ] `test_io_http`/相关测试不依赖假连接。
- 验证命令: 方案 A `ctest -R opcua`（新增）；方案 B `rg -n 'is_connected = true|Mocked' modules/edge_io`。
- 审核清单: 是否引入新的假数据；真实链路是否可复现；默认构建行为是否明确。

### T-302 WASM 算子失败语义改造
- Epic: Zero-Mock | Priority: P1 | Estimate: M | Depends: -
- 背景: `rehab_companion.c` 与 `radar_tinyml_operator.c` 在 `host_get_tag` 失败时回退硬编码默认值（调查报告 §9.5）。
- 问题证据: `if (host_get_tag(...) != 0) { acc = 1.0; }` 等。
- 目标与要求:
  - `host_get_tag` 失败 → 设置错误/告警 Tag（如 12001 系统状态、错误码），跳过该 tick 计算，禁止使用假值。
  - `ext_log` 输出结构化错误（tag id、错误码）。
  - 新增 WASM 测试：模拟 host API 失败，断言无假值写回。
- 涉及文件: `modules/edge_stream/src/wasm_runtime/rehab_companion.c`、`radar_tinyml_operator.c`、`wasm_mgr.c`、测试。
- 完成标准 DoD:
  - [ ] 失败路径不产生业务输出；错误可见；测试覆盖。
- 验证命令: `ctest -R wasm`；代码审查。
- 审核清单: 是否有其他 wasm 存在同类回退；错误是否会被上层吞掉。

### T-303 雷达/FOC/UDS 新算子真实化与契约绑定
- Epic: Zero-Mock | Priority: P1 | Estimate: L | Depends: T-302
- 背景: 新增 `radar_tinyml_operator.c` 为启发式公式（呼吸率=16+amp*40、FOC iq=err*0.05+0.8、手势阈值），且依赖 Tag 12021-12035 无物理来源（调查报告 §9.6）。
- 问题证据: `radar_tinyml_operator.c` 全文；未跟踪新域 YAML。
- 目标与要求:
  - 为雷达/FOC/UDS 数据建立真实或 Matrix 仿真数据源（契约 binding + Actor），数据可溯源。
  - 启发式公式替换为规格化算法（雷达：2D FFT+CFAR 或文档化降级；FOC：SimpleFOC 闭环或取消虚假计算；UDS：真实诊断状态机）。
  - 若属实验特性：默认关闭（Kconfig/契约裁剪），显式标记 `experimental`。
- 涉及文件: `modules/edge_stream/src/wasm_runtime/radar_tinyml_operator.c`、`contracts/schema/domains/*`、`cmd/smart_box`、测试。
- 完成标准 DoD:
  - [ ] 每个新 Tag 均有来源（物理/仿真 Actor）且 E2E 可验证；无魔法常数公式冒充算法。
  - [ ] 实验特性默认关闭且有文档。
- 验证命令: `./test.sh --omnis`（smart_box 剧本）；`rg -n '0\.05|\* 40' modules/edge_stream`。
- 审核清单: 是否把“演示”标成“完成”；数据链路是否真正打通；默认行为是否安全。

### T-304 审计密钥管理（VPP/Syslog HMAC 配置化）
- Epic: Zero-Mock/安全 | Priority: P1 | Estimate: S | Depends: T-003
- 背景: VPP 审计与 syslog HMAC 使用硬编码密钥且无轮换机制（调查报告 §9.3）。
- 问题证据: `stage_vpp_logger.c`、`secure_syslog_exporter.c`。
- 目标与要求:
  - 密钥从配置文件/环境注入；支持轮换（版本化 key id）；签名验证侧同步。
  - 文档说明密钥生命周期。
- 涉及文件: 上述源文件、`config/`、`docs/`。
- 完成标准 DoD:
  - [ ] 单元测试覆盖“旧 key 轮换后新签名验证通过、旧签名按策略失效”。
  - [ ] 无硬编码密钥。
- 验证命令: `ctest -R vpp|syslog`（如存在）；`rg`。
- 审核清单: 轮换是否会破坏存量日志验证；配置默认值是否安全。

### T-305 Anti-Mock 审计工具与 CI 门禁
- Epic: Zero-Mock | Priority: P1 | Estimate: M | Depends: T-302/T-303
- 背景: Zero-Mock 是项目宪法，但缺少机械执行手段（调查报告 §9.4-9.6）。
- 问题证据: 多文件存在 mock/占位值。
- 目标与要求:
  - 开发 `scripts/anti_mock_check.sh`：扫描可疑模式（`= 220.0`、`is_connected = true`、`Mocked`、失败回退常量），输出文件:行。
  - Matrix 测试增加“数据溯源断言”：每个断言值必须来自 Actor 注入或计算链（helper 统一封装）。
  - CI 门禁：新提交不得新增 anti-mock 告警（存量告警白名单 + 到期日）。
- 涉及文件: `scripts/`、`cmd/*/matrix/`、CI。
- 完成标准 DoD:
  - [ ] 扫描器可运行且存量清单可追踪；CI 增量阻断。
  - [ ] 新增 E2E 测试全部走溯源 helper。
- 验证命令: `./scripts/anti_mock_check.sh`；CI。
- 审核清单: 白名单是否会掩盖新问题；扫描规则误报率；是否真正“机械可执行”。

---

# Phase 4 — CI/CD 与工程护栏（P1/P2）

### T-401 CMake/构建参数对齐
- Epic: CI/CD | Priority: P1 | Estimate: S | Depends: -
- 背景: CI 传 `-DENABLE_COVERAGE=ON` 而选项是 `COVERAGE`；`build.sh` 传 `-DENABLE_UI=ON` 而选项是 `ENABLE_WEB_BUILD`（调查报告 §7）。
- 问题证据: `ci.yml`、`build.sh`、`CMakeLists.txt`。
- 目标与要求:
  - 统一选项命名；不支持旧名（或提供兼容别名并在日志 warning）。
  - `build.sh`/CI/文档三处一致。
- 涉及文件: `CMakeLists.txt`、`build.sh`、`test.sh`、`ci.yml`、README。
- 完成标准 DoD:
  - [ ] `cmake -L` 输出与文档一致；CI 覆盖真正生效（验证有 gcda 产物）。
- 验证命令: `cmake -LA | grep -E 'COVERAGE|WEB'`。
- 审核清单: 是否破坏旧命令行兼容；是否有人依赖旧名。

### T-402 CI 增加 Go 构建与测试 Job
- Epic: CI/CD | Priority: P1 | Estimate: S | Depends: T-401
- 背景: CI 不构建/不测试任何 Go 服务（调查报告 §7）。
- 问题证据: `ci.yml` 无 `go build/test` 步骤。
- 目标与要求:
  - 新增 job：`go build ./...` + `go vet ./...` + `go test ./...`（含 smart-box-gateway）。
  - Go 版本与 `go.mod` 对齐（1.25.x 或按工具链策略锁定）。
  - 产物（gateway/ui-backend/edge-ops）编译验证。
- 涉及文件: `ci.yml`、`go.mod`、`go.sum`。
- 完成标准 DoD:
  - [ ] PR 上 Go job 全绿；任一 Go 包编译错误会阻断合并。
- 验证命令: `cd go && go build ./... && go vet ./... && go test ./...`。
- 审核清单: Go 版本策略是否明确；是否引入网络下载不稳定依赖。

### T-403 CI 增加 Matrix/Omnis Job
- Epic: CI/CD | Priority: P1 | Estimate: L | Depends: T-101/T-102
- 背景: Matrix 是产品级 E2E，但 CI 从未执行（调查报告 §7/§8）。
- 问题证据: `ci.yml` 无 omnis 步骤；历史 2 项失败未纳入门禁。
- 目标与要求:
  - x86_64 job 增加 `./test.sh --omnis`，超时 30 分钟，失败输出 `matrix_edgec.log` artifact。
  - 失败重试策略：不自动重试掩盖 flaky，先修根因。
  - 结果以 PR comment/check 呈现。
- 涉及文件: `ci.yml`、`test.sh`、`tools/matrix/`。
- 完成标准 DoD:
  - [ ] 连续 3 次 CI omnis 全绿后才可视为稳定。
- 验证命令: CI run。
- 审核清单: 超时/资源是否足够；日志是否完整可诊断；是否被当作“参考”而非门禁。

### T-404 覆盖率门禁落地
- Epic: CI/CD | Priority: P1 | Estimate: M | Depends: T-401
- 背景: task.md 声称覆盖门禁完成，但 CI 未启用且阈值不可见（调查报告 §7/§8）。
- 问题证据: CI 参数不匹配；`test.sh --coverage` 存在但 CI 无门禁。
- 目标与要求:
  - 用 gcovr/lcov 生成报告并设置阈值：`datahub.c`、`arbiter.c`、`shmring.c` 行/分支 ≥90%（按调查报告目标），全仓 ≥ 现基线。
  - 阈值写入 CI 常量与文档；低于阈值阻断。
  - 报告上传 artifact；README 加徽章。
- 涉及文件: `ci.yml`、`test.sh`、`CMakeLists.txt`、`docs/`。
- 完成标准 DoD:
  - [ ] CI 覆盖 job 真实生效（gcda 非空）且阈值检查可红可绿（用故意降覆盖验证）。
- 验证命令: `./test.sh --coverage`；CI。
- 审核清单: 阈值是否合理而非表演；是否只测“容易的代码”。

### T-405 安全扫描（gitleaks/CodeQL/依赖漏洞）
- Epic: CI/CD | Priority: P1 | Estimate: M | Depends: T-001/T-003
- 背景: 私钥曾入库、硬编码密钥存在，但无自动扫描（调查报告 §9）。
- 问题证据: 调查报告 §9 各项。
- 目标与要求:
  - gitleaks 扫描（含历史，可配置阈值）；CodeQL（C/Go）；`govulncheck`。
  - 高/严重告警阻断合并；低危进缺陷台账。
- 涉及文件: `ci.yml`、`.gitleaks.toml`。
- 完成标准 DoD:
  - [ ] 模拟提交密钥 → CI 红；移除后绿。
- 验证命令: `gitleaks detect`；CI。
- 审核清单: 扫描范围是否含历史；误报白名单是否受控。

### T-406 发布流水线（签名/SBOM/Release）
- Epic: CI/CD | Priority: P2 | Estimate: L | Depends: T-004
- 背景: SmartBox 发布包在本地手工打 tgz 且未跟踪；CI 有 cpack/Release 但无签名/SBOM（调查报告 §5/§7/§10）。
- 问题证据: `smart_box_release_v1.0.0.tgz` 未跟踪；`ci.yml` release job。
- 目标与要求:
  - 发布统一走 CI：tag → 构建 → 测试 → 打包 → 签名（代码签名）→ SBOM（syft）→ GitHub Release。
  - SmartBox 发布脚本 `scripts/package_smart_box.sh` 纳入流水线并校验内容清单。
  - 发布说明自动生成 + 人工编辑。
- 涉及文件: `ci.yml`、`scripts/package_smart_box.sh`、Docker。
- 完成标准 DoD:
  - [ ] 打 tag 后可一键产出可复现、可验证的发布包；校验 SHA256。
- 验证命令: CI 发布 run；`tar -tzf` 对比清单。
- 审核清单: 签名私钥存放（HSM/CI secret）；SBOM 是否真实；发布包是否包含密钥/日志。

### T-407 Docker 镜像修正与多架构验证
- Epic: CI/CD | Priority: P2 | Estimate: M | Depends: T-401
- 背景: `Dockerfile.prod` 依赖打包与运行态未验证；CI 已有多架构 buildx（调查报告 §7）。
- 问题证据: `docker/Dockerfile.prod`、ROADMAP 提及依赖打包待更新。
- 目标与要求:
  - 镜像包含全部运行库；非 root 运行；健康检查；最小化层。
  - amd64/arm64 构建后各跑冒烟（`iems --version`、health API）。
- 涉及文件: `docker/Dockerfile.prod`、`docker_build.sh`、CI。
- 完成标准 DoD:
  - [ ] 两架构镜像冒烟通过；镜像内无源码/密钥。
- 验证命令: `docker run --rm <image> iems --version`。
- 审核清单: 是否引入构建期密钥；体积是否可控。

---

# Phase 5 — 产品闭环（P2）

### T-501 SmartBox 功能矩阵逐项验收
- Epic: SmartBox | Priority: P2 | Estimate: L | Depends: T-002/T-005
- 背景: 发布 README 宣称对照 Excel 功能完成度 100%，但缺少可复现的逐项测试矩阵（调查报告 §5/§9）。
- 问题证据: `SmartBox-v1.0.0-Release/README.md`；`cmd/smart_box/matrix/*.py` 仅 4 个测试。
- 目标与要求:
  - 将 Excel 功能清单（康复训练、监护、语音、投屏、相册、慢病、支付等）映射为可执行测试（C/Go/Python/前端冒烟）。
  - 无法在无硬件环境验证的项标记 `HW_REQUIRED` 并给出人工验收步骤。
  - 更新功能矩阵：每项状态=通过/部分/未验证+证据。
- 涉及文件: `cmd/smart_box/`、`docs/`、`SmartBox-v1.0.0-Release/README.md`。
- 完成标准 DoD:
  - [ ] 矩阵中 0 个“宣称完成但无证据”项；CI 运行可自动项。
- 验证命令: `cd cmd/smart_box && pytest matrix -q`。
- 审核清单: 是否用“模拟器通过”冒充“硬件通过”；矩阵是否可追溯。

### T-502 SmartBox 云依赖联调（心跳/OTA/日志/DLNA/FHIR）
- Epic: SmartBox | Priority: P2 | Estimate: L | Depends: T-501
- 背景: 发布说明列明云平台 API 未配套（调查报告 §5）。
- 问题证据: 发布 README “云端服务端及管理页面依赖”。
- 目标与要求:
  - 定义云端 API 契约（心跳 30min、OTA 下载、日志上传、断网缓存补发），提供 mock server 供本地联调。
  - DLNA 投屏、FHIR 导出、相册上传补齐 E2E。
- 涉及文件: `go/cmd/smart-box-gateway/main.go`、`cmd/smart_box/matrix/`、`docs/`。
- 完成标准 DoD:
  - [ ] 与 mock cloud 联调全链路通过；真实云 API 对接点文档化待甲方提供。
- 验证命令: `pytest matrix/test_medical_dlna.py` 等。
- 审核清单: mock 是否被误当生产；断网缓存/补发是否真实。

### T-503 IEMS 演示三合一（峰谷套利/过压/谐波）
- Epic: IEMS | Priority: P2 | Estimate: M | Depends: T-101/T-102
- 背景: ROADMAP_SPRINT 规划 Demo 三合一，当前仅脚本雏形（调查报告 §5）。
- 问题证据: `demo/peak_shaving.sh`、`demo/run_all.sh`。
- 目标与要求:
  - 一键脚本：峰谷套利（MPC 收益对比）、过压保护（注入→RMS→relay）、谐波分析（THD）。
  - 输出预期截图/数据文件与量化对比。
- 涉及文件: `demo/`、`cmd/iems/`、`docs/`。
- 完成标准 DoD:
  - [ ] 干净环境一键可跑；三场景断言通过；README 含预期输出。
- 验证命令: `./demo/run_all.sh`。
- 审核清单: 演示是否用真实计算；收益对比是否可复现。

### T-504 DCU 2000 表 E2E 与 OAD 编译流验收
- Epic: DCU | Priority: P2 | Estimate: L | Depends: T-202
- 背景: DCU 历史提交声称 2000 表闭环，但当前 `dcu_router.c` 有 TODO 且 OAD 硬编码（调查报告 §5）。
- 问题证据: `dcu_router.c:102,107` TODO；`cmd/dcu` 测试。
- 目标与要求:
  - 补齐 TODO：地址列表解析、指定表计更新。
  - 编译流验证：`manifest.yaml → compiler → .h/.edgec → DCU 加载`。
  - 2000 表压力 E2E 作为可选 CI 长任务（nightly）。
- 涉及文件: `cmd/dcu/**`、`modules/edge_io/src/dcu_*.c`、契约。
- 完成标准 DoD:
  - [ ] `ctest -R dcu` 全绿；2000 表 nightly 通过；无 TODO 残留（或显式 backlog）。
- 验证命令: `ctest -R dcu`；nightly。
- 审核清单: 轮询吞吐/内存是否真实达标；是否存在“只跑通不复用”。

### T-505 Web HMI 完整性验收
- Epic: HMI | Priority: P2 | Estimate: M | Depends: T-401
- 背景: README 声称 8 个模块大部分完成，但缺少自动化 UI 验证；`Scheduling/DeviceList` 标“部分完成”（调查报告 §5/§7）。
- 问题证据: `EdgeC/README.md` §4.4 状态表；`web/products/`。
- 目标与要求:
  - 对 Dashboard/Contracts/Pipelines/DataCurves/SystemConfig/Scheduling/Devices/Storage 页面逐一验收。
  - 补 Playwright 冒烟：页面加载、SSE 数值跳动、配置读写。
  - 补齐缺失/占位页面（Scheduling、DeviceList）。
- 涉及文件: `web/products/iems`、`web/products/smart_box`、`go/cmd/ui-backend`、`go/cmd/edge-gateway`。
- 完成标准 DoD:
  - [ ] 冒烟测试覆盖 8 页面；README 状态与实际一致。
- 验证命令: `npx playwright test`（新增）。
- 审核清单: 前端是否在用真实 API 而非 mock SSE；是否把“页面存在”当“功能完成”。

### T-506 WASM App Store 上传 API 与签名链全流程
- Epic: WASM | Priority: P2 | Estimate: L | Depends: T-001/T-205
- 背景: task.md 维度八规划 `POST /api/v1/apps/upload` + App Store，当前只有本地加载（调查报告 §5/§9）。
- 问题证据: `go/cmd/edge-gateway/main.go` 无 upload；`wasm_mgr.c` 有加载逻辑。
- 目标与要求:
  - 网关实现上传端点：鉴权、大小限制、签名验证（新密钥）、病毒扫描（可选）、版本管理。
  - UI “App Store”面板：上传/启停/回滚。
  - 热替换测试：运行中替换版本，旧版可回滚。
- 涉及文件: `go/cmd/edge-gateway/main.go`、`modules/edge_stream/src/wasm_runtime/wasm_mgr.c`、`web/`。
- 完成标准 DoD:
  - [ ] 未签名/坏签名 wasm 被拒；上传→验证→热加载→回滚 E2E 通过。
- 验证命令: `curl -F app=@virus.wasm` 应 4xx；E2E 测试。
- 审核清单: 签名信任根是否与 T-001 一致；回滚是否原子；资源上限是否防 DoS。


---

# Phase 6 — 文档与治理（P2，贯穿）

### T-601 文档真实性校准
- Epic: 文档 | Priority: P2 | Estimate: M | Depends: 各阶段
- 背景: 多处文档宣称“完成/100%/生产级”与实测不符（调查报告 §11）。
- 问题证据: README/发布 README vs OPC UA mock、Matrix 失败、CI 缺失。
- 目标与要求:
  - 全库状态性声明审计：README、PRD、task.md、release README、skill。
  - 统一状态词汇：`Implemented/Partially/Experimental/Planned/HW_REQUIRED`，每条声明可追溯测试。
  - 建立“文档-代码”一致性检查（CI 可扫关键声明）。
- 涉及文件: 全库 `.md`。
- 完成标准 DoD:
  - [ ] 无“完成但无证据”声明；每产品 README 状态与 CI/测试一致。
- 验证命令: 人工审计 + `rg` 抽查。
- 审核清单: 是否矫枉过正（删掉有价值愿景）；是否引入新夸大。

### T-602 ADR 与决策记录补齐
- Epic: 文档 | Priority: P2 | Estimate: S | Depends: -
- 背景: 仅有 ADR-0001/0002；本期大量决策未记录（调查报告 §11）。
- 目标与要求:
  - 新增 ADR：密钥管理与历史清理；UDS/DoIP 安全模型；Zero-Mock 机械门禁；CI 治理与门禁；契约版本化。
- 涉及文件: `docs/adr/`。
- 完成标准 DoD:
  - [ ] 每个重大决策有 ADR（状态/日期/备选方案/理由/影响）。
- 验证命令: 目录清单。
- 审核清单: ADR 是否反映最终实现而非空谈。

### T-603 AGY 交接/审查协议落地
- Epic: 治理 | Priority: P2 | Estimate: S | Depends: -
- 背景: 用户要求“AGY 实施 → Codex 审核”，需要制度化（本计划 §4）。
- 目标与要求:
  - 在 `CONTRIBUTING.md` 或 `docs/agents/issue-tracker.md` 固化：Ticket 模板、分支规范、提交信息、DoD、审核流程、分数卡。
  - 提供审核命令速查表。
- 涉及文件: `CONTRIBUTING.md`（新建）、`docs/agents/`。
- 完成标准 DoD:
  - [ ] AGY 可按文档独立领取 Ticket；审核流程可执行。
- 验证命令: 走查一个样例 Ticket。
- 审核清单: 是否与实际工具链（本仓库）一致。

### T-604 缺陷台账/里程碑/标签体系
- Epic: 治理 | Priority: P2 | Estimate: S | Depends: T-603
- 背景: 无集中缺陷台账；问题散落 review.md/audit/日志（调查报告 §11）。
- 目标与要求:
  - 建立里程碑（M0 止血 / M1 核心 / M2 护栏 / M3 产品）与标签词汇（P0-P2、security、zero-mock、ci、docs…）。
  - 以本文档为准，在本地 `docs/tickets.md` 维护 41 个 Ticket 状态索引（暂不创建 GitHub Issues）。
- 涉及文件: GitHub Issues / `docs/tickets.md`。
- 完成标准 DoD:
  - [ ] 所有 Ticket 有唯一编号、负责人状态；入口单一。
- 验证命令: 页面/文件抽查。
- 审核清单: 标签是否被滥用；状态是否及时更新。

---

## 4. 审核与验收协议（Codex 对 AGY 的审查）

### 4.1 每次提交必须附带的证据
- 变更摘要：改了哪些文件、为什么、如何满足 DoD。
- 测试证据：`ctest` / `go test` / Matrix 输出（关键段落）。
- 自审结论：逐条勾选 DoD；未满足项必须显式列出。

### 4.2 审查流程
1. **完成度核对**：对照 DoD 逐项检查（不是“看起来做了”，而是有证据）。
2. **代码审查**：关注是否违反四平面隔离、Zero-Mock、Consumer-Driven、非侵入式集成、硬编码 TagID 等宪法。
3. **实测验证**：在干净构建目录执行本 Ticket 的验证命令；P0/P1 必须重跑全量 ctest + go test；涉 Matrix 的 Ticket 重跑对应剧本。
4. **回归检查**：确认未破坏其他 Ticket（依赖链见 §5）。
5. **打分**：完成度 0-100、代码质量 A-F、测试充分性 A-F、风险评级；结论 Pass/Pass-with-notes/Fail。

### 4.3 红线（一票否决）
- 引入新的硬编码假数据/占位符。
- 修改 `modules/hal/`、`rtos/` 原厂代码。
- 绕过 DataHub 直写 SHM/全局变量充当真相。
- P0 安全 Ticket 无测试或未通过审核即合并。
- 删除/绕过现有测试或 CI 门禁。

## 5. 里程碑与依赖

| 里程碑 | 包含 Ticket | 出口条件 |
|---|---|---|
| M0 止血 | T-001~T-006 | 无已知安全红线；工作区干净；可合并基线 |
| M1 核心可信 | T-101~T-107、T-201~T-206 | ctest/go/omnis 全绿；DataHub/Arbiter 修复有测试；契约校验生效 |
| M2 护栏 | T-301~T-305、T-401~T-407 | Zero-Mock 门禁生效；CI 覆盖 Go/Matrix/Sanitizer/安全扫描 |
| M3 产品 | T-501~T-506、T-601~T-604 | 三产品验收矩阵可复现；文档与事实一致 |

主要依赖链：
- T-002 ← T-005；T-001 ← T-506/T-205；T-101/T-102 ← T-403；T-103 ← T-104/T-107；T-201 ← T-202~T-205；T-302 ← T-303/T-305；T-401 ← T-402~T-407。

## 6. 决策确认与待确认问题

### 6.1 已确认决策（2026-08-01）

1. **Ticket 载体**：以本文档为准，暂不在 GitHub Issues 创建（T-604 只维护本地 `docs/tickets.md` 状态索引）。
2. **UDS/DoIP 功能去向**：保留并加固，T-002 按此执行。
3. **Zephyr 适配**：不在本期范围（T-507 已移除）；Zephyr 相关工作在 `/workspaces/applications` 独立管理。
4. **实施方式**：三个 agent 并行，分工见 §1.1。

### 6.2 仍需确认

1. **T-001 是否允许重写 Git 历史？**
   通俗解释：私钥已经出现在历史提交中；“清历史”= 用 `git filter-repo`/BFG 重写提交记录并强制推送，所有协作者需要重新克隆/同步。
   - 选项 A（推荐）：允许重写历史，彻底清除私钥痕迹。
   - 选项 B：不重写，轮换新密钥并从当前版本移除；历史中的旧私钥视为已泄露（风险残留但可控）。
2. **三 Agent 资源边界**：每个 Agent 同时并行几个 Ticket？每个 Ticket 允许几轮迭代后必须上报？

## 7. 附录：验证命令速查

```bash
cd /workspaces/edgeos/EdgeC
./build.sh                        # 全量构建（契约+WASM+C+Web）
./test.sh                          # 127 CTest
./test.sh --omnis                  # Matrix E2E 9 项
./test.sh --asan                   # ASan 全量
cd go && go test ./...             # Go 测试
cd tools/contract-compiler-v2 && go test ./...   # 编译器测试
git ls-files | grep -E '\.(db|tgz|wasm)$'        # 产物卫生
rg -n 'secret|api_key|password' modules go cmd --glob '!*.md' --glob '!*.yaml'  # 密钥扫描
```

---

*计划结束。AGY 从 Phase 0 开始按 Ticket 顺序领取；Codex 按 §4 协议逐项审核。*
