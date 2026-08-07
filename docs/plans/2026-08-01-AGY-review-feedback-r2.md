# AGY 第二轮审核反馈（Codex Review Round 2）

> **Mr. WKT** — 审核对象：`ticket/T-107` 工作区未提交整改（2026-08-02 快照）。审核方式：逐文件差异审查 + 将全部未提交改动复制到独立 worktree 后干净构建、全量测试与 Omnis 实测。

---

## 0. 实测结果（干净 worktree，应用全部未提交改动）

| 检查项 | 结果 |
|---|---|
| CMake configure（编译器预构建后） | ✅ 通过；Validator 三产品全部 PASSED |
| `cmake --build`（442 targets） | ✅ 通过 |
| `ctest`（127 项） | ✅ 127/127 通过 |
| `go test ./...` | ✅ 全部 ok（smart-box-gateway 实际运行 6.05s） |
| `tools/contract-compiler-v2 go test`（含新 Validator） | ✅ 通过 |
| `scripts/check_tag_consistency.sh` | ✅ PASSED（SmartBox 62 / IEMS 28 / DCU 6 ≤ 聚合 Go 66） |
| `scripts/check_repo_clean.sh` | ✅ PASSED（已修复仓库根定位 + wasm 白名单） |
| `./test.sh --omnis` | ✅ **10/10 通过（58.77s）**，含此前失败的 `test_iems_multi_tag_consistency` 与 `test_adc_to_protection_e2e`（继电器真实动作） |

上一轮的 P0 阻断（tags.go 覆盖导致 go test 失败）**已实质解决**：聚合脚本生成 66 条全量 tags.go，Go 编译与点表一致性均绿。T-101/T-102 在本次实测中 10/10 全绿。

## 1. Ticket 逐项结论（第二轮）

| Ticket | 结论 | 说明 |
|---|---|---|
| T-101 多点一致性 | **Pass（待稳定性证据）** | 干净 Omnis 实测 PASSED；仍需连续 3 次全绿 + 根因说明（为什么第一轮失败） |
| T-102 ADC→保护 E2E | **Pass（待稳定性证据）** | 实测继电器动作 PASSED；仍需 3 次复跑证据，并说明根因是测试污染还是产品缺陷 |
| T-002 UDS/DoIP | **Fail（仍有红线残留）** | 分帧/锁定位/算法有改进，但见 §2.2 |
| T-003 密钥外置 | **Fail（部分）** | 见 §2.3 |
| T-004 仓库卫生 | **Pass** | 脚本 cd 修复 + wasm 白名单文档化，实测通过 |
| T-005 增量收口 | **Pass** | 无变化，维持上一轮结论 |
| T-006 合并策略 | **Pass（微调）** | ADR 中 “10 E2E” 与本次实测一致（10 项），无需再改 |
| T-103 DataHub 并发 | **Pass-with-notes** | init 失败路径已补 ring 清理；`datahub_init_from_fd` 泄漏仍未修，无失败注入测试 |
| T-104 压实 I/O | **Pass-with-notes** | 已加 `fsync`；仍缺 p99 数据与 kill -9 恢复用例 |
| T-105 FIFO 唤醒 | **Pass-with-notes** | 无新改动；仍缺“C 先启动、Go 后挂载”真实唤醒 E2E |
| T-106 Arbiter 生命周期 | **Pass-with-notes** | 清理已移出锁外；仍缺 executor 路径 double-free 回归与 TSan 证据 |
| T-107 Sanitizer（含大量新整改） | **未完成** | 工作区改动远超 sanitizer 范围；新增聚合/校验/编译流程问题见 §2.1；ASan/TSan CI job 尚未提交 |
| T-201 编译器校验（新增提前实现） | **Pass-with-notes** | Validator + 单测质量不错；跨产品同名 Tag 未校验（见 §2.1-b） |

## 2. 仍需修复的问题

### 2.1 P0：聚合 tags.go 的确定性隐患（当前必须修）

当前 `scripts/aggregate_go_tags.py` 能工作，但有两个硬伤：

1. **跨产品同名 Tag 冲突被静默吞掉**。DCU 的 Hex OAD 标签与 IEMS/SmartBox 同名但不同 ID：
   - `VOLTAGE_PHASE_A`：1001 vs 536871424
   - `CURRENT_PHASE_A`：1002 vs 536936960
   - `ENERGY_ACTIVE_IMPORT`：3001 vs 1049088
   聚合后的 `Metadata` 里两者都在，但 `const` 字典按“后处理者覆盖”合并，最终 `tags.VOLTAGE_PHASE_A` 指向哪个 ID **取决于 glob 遍历顺序**（我实测本机为 1001，但顺序无保证）。换台机器/文件系统可能变成 536871424，Go 侧语义静默错乱。
   - 修复要求：对生成文件 `sorted()`；遇到“同名不同 ID”直接报错退出（或按产品命名空间生成独立 Go 包，推荐后者，配合 T-202 治理）。
2. **三个产品的生成命令都会调用 aggregate 写同一源文件**：`-j` 并行时存在并发写风险。修复要求：聚合改为单一 custom target，`DEPENDS` 三个产品的生成目录，串行执行一次。

### 2.2 P0：T-002 安全红线残留

- **HMAC 密钥仍硬编码在源码**：`computeSecurityKey` 使用常量 `"EdgeOS_DoIP_UDS_SecretKey_2026"`。应从环境变量/配置文件注入；否则 seed-key 形同虚设。
- **per-client 状态表无限增长**：`secAccessMgr.clients` 以 `RemoteAddr`（含随机端口）为 key，永不清理，可被反复连接造成内存膨胀。需在连接断开时删除对应状态，或改用有界 LRU。
- **DoIP readBuf 无上限**：只校验单帧 payload ≤4096，但半包攻击可让 `readBuf` 无限累积。需加总缓冲上限（如 64KB）并超限断开。
- **状态仍未写入 DataHub**：`recordTagValue` 只写 SQLite `tag_history`，未走 DataHub shadow；12031-12035 的“真相”仍是全局变量 + DB。按 DoD 必须通过 DataHub API 写契约 Tag。
- **DTC/ECU 标识/例程结果仍硬编码**（`P09012F`、`EdgeC_ECU`、Routine 恒成功）——Zero-Mock 红线。未接真实 ECU 时应返回“未连接/无数据”，禁止编造故障。
- **新增测试不足**：`main_test.go` 未覆盖连接数上限、超长 payload、3 次失败锁定、分帧/多帧、DataHub 回写。

### 2.3 P1：T-003 生产 fail-fast 仍不完整

- `secure_syslog_exporter.c`：生产模式下缺密钥只是“本次拒绝导出”，不是启动失败——会造成静默审计空洞。应在启动初始化阶段检查并 fail-fast。
- `go/cmd/demo-algo/main.go` 仍是 `TEST_ONLY_demo_algo_key` 无条件回退，未按 `EDGE_ENV` 区分。
- 缺“生产缺密钥 → 启动失败”的测试。

### 2.4 P1：构建流程的干净克隆问题

- `cmake/EdgeContract.cmake` 在 configure 期执行 `contract-compiler`，但该二进制在干净克隆中不存在（被 gitignore），configure 会失败。需在 configure 前保证编译器已构建（如 `build.sh` 先 `go build` 编译器），或把 execute_process 改为“存在才执行 + 缺失时给出明确指引”。
- `config.cmake` 仍会生成但已不再被 `include`。若产品级 `CONFIG_*` 不再需要请删除生成；若需要请恢复 include，避免静默丢失。
- `EdgeContract.cmake` 中 include 路径用了 `${SYSTEM_NAME_LOWER}-${CMAKE_HOST_SYSTEM_PROCESSOR}`（宿主架构），aarch64 交叉编译会错误地引用 x86_64 头文件目录；应使用函数内已有的 `${ARCH}`。

### 2.5 P2：仍缺的证据

- T-101/T-102：Omnis 连续 3 次全绿输出。
- T-103：`datahub_init_from_fd` 失败路径修复 + 失败注入测试。
- T-104：压实期间写延迟 p99 数据 + `kill -9` 恢复用例。
- T-105：C 先启动、Go 后挂载的事件唤醒 E2E。
- T-106：executor.on_io_done 双重释放回归测试 + TSan 运行记录。
- T-107：ASan/TSan CI job 与全量报告。

## 3. 下一轮提交要求

1. 修复 §2.1 聚合确定性问题后，先提交“点表聚合 + 校验”部分（可拆为 T-204/T-201 独立提交），再提交 T-107。
2. 提交时附：DoD 勾选、`ctest`/`go test`/Omnis 输出、每个反馈项的对照说明。
3. P0/P1 未通过复审前不得向 `feature/modular-refactor` 合并。

---

*复审结论依据 2026-08-02 干净 worktree 实测；聚合脚本确定性问题的复现方法已附在 §2.1。*
