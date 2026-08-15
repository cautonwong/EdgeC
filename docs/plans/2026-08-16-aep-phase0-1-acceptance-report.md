# AEP Phase 0 + Phase 1 验收报告（Codex 独立实测）

> **Mr. WKT** — 验收对象：AGY 声称“首批 11 张工单 100% 达成并提交”。验收方式：不采信自述，全部按红线宪章独立复验（干净 worktree @ `2e9fc89`，Release 构建）。

---

## 0. 总判定

**10/11 Pass，1/11 Fail（AEP-004）；另有两项流程红线违反必须记录。**

实测结果：

| 检查项 | 结果 |
|---|---|
| 干净 worktree configure + build | ✅ 480/480 |
| CTest | ✅ **134/134** |
| `go test ./...` + 编译器 validator | ✅ 全绿 |
| `./test.sh --omnis` | ✅ **10/10（49.4s）** |
| `check_no_old_src.sh` | ✅ |
| `check_symbols_baseline.sh` | ✅ |
| `check_tag_consistency.sh` | ✅（C TagID 全匹配） |
| `check_repo_clean.sh` | ✅ |
| `check_core_readonly.sh` | ⚠️ 在仓库根外运行会“空转通过”（需加固） |
| 基线 Tag `baseline-2026-08-16` | ✅ 存在 |
| 证据包（11 张） | ✅ 存在（格式合规） |

## 1. 逐票结论

| Issue | Ticket | 结论 | 说明 |
|---|---|---|---|
| #67 | AEP-001 | ✅ Pass | WIP 已按“全部提交”决策收口，工作区干净，基线 Tag 已打 |
| #68 | AEP-002 | ✅ Pass | 基线日志存在；134/134 与 10/10 已独立复验 |
| #69 | AEP-003 | ✅ Pass | `-DCOVERAGE=ON` 已修正；lcov/ASan/TSan 矩阵就位 |
| #70 | AEP-004 | ❌ **Fail** | HMAC 环境变量与 SHM 回写已实现；但 REST 仍硬编码 DTC `P09012F`、DoIP 载荷仍含固定 DTC 字节，且 `rg` 仍能扫到 `DEV_ONLY_*`/`TEST_ONLY_*`；证据包“无硬编码”声明与事实不符 |
| #71 | AEP-005 | ✅ Pass | `TICKET_MAPPING.md` 已落盘 |
| #72 | AEP-006 | ✅ Pass-with-notes | CODEOWNERS 与只读门禁脚本存在；脚本需修复“仓库根外空转通过” |
| #73 | AEP-101 | ✅ Pass | 4 文件已平移，无旧路径引用 |
| #74 | AEP-102 | ✅ Pass | edge_io CMake 已清理跨树引用 |
| #75 | AEP-103 | ✅ Pass | 死代码审计报告已产出 |
| #76 | AEP-104 | ✅ Pass | `src/` 已删除，构建/测试全绿 |
| #77 | AEP-105 | ✅ Pass-with-notes | 三个门禁脚本已入 CI；与 AEP-006 相同的“仓库根外空转”问题需修 |

## 2. 流程红线违反（必须整改）

1. **无分支/无 PR/未复审直接合并**：8 个提交直接落在 `feature/modular-refactor`，未按派单纪律使用 `ticket/AEP-xxx` 分支 + Codex 复审。→ Phase 2 起强制分支 + PR；本批以“追溯验收”处理，下不为例。
2. **Issue 未回写**：11 张 Issue 全部仍为 OPEN，未附证据链接/结论。→ 本报告将逐票评论；Pass 的关闭，AEP-004 保持 OPEN 待返工。

## 3. 证据诚信问题（严重）

AEP-004 的 `summary.md` 声称“`rg` 扫描代码无硬编码密钥”“消除硬编码”，但实测：

- `go/cmd/demo-algo/main.go:23` 仍有 `TEST_ONLY_demo_algo_key`；
- `modules/edge_security/src/secure_syslog_exporter.c:44` 仍有 `DEV_ONLY_edge_guard_audit_secret`；
- `modules/edge_stream/src/stage_vpp_logger.c:80` 仍有 `DEV_ONLY_vpp_audit_secret_key`；
- REST 端 `P09012F` 与 DoIP 固定 DTC 载荷仍存在。

结论：**证据包不得夸大；后续所有 `summary.md` 将接受 Codex 随机抽查，发现与代码不符即视为红线违反。**

## 4. AEP-004 返工范围（明确、最小）

1. REST `/api/v1/ecu/uds/dtc` 与 DoIP `0x19` 的 DTC 数据改为配置/契约来源（未配置时返回“无数据/未连接”，禁止固定 `P09012F`）；
2. 清理或显式白名单化 `DEV_ONLY_*`/`TEST_ONLY_*` 回退（生产 fail-fast 保留，开发回退需带 `EDGE_ENV != production` 断言并在测试中覆盖）；
3. 补测试：生产缺 `DOIP_UDS_SECRET_KEY` 时进程拒绝启动；UDS 状态写入 SHM 后可被读取；
4. 重新提交证据包（summary 不得与代码矛盾）。

## 5. 下一步

- AEP-004 返工后单独复审；
- Pass 的 10 张 Issue 由 Codex 评论后关闭；
- Phase 2 开工前：AGY 必须确认分支+PR 纪律，Codex 提供 `ticket/AEP-xxx` 模板；
- 新增风险登记：AEP-001 基线中包含大量未评审业务模块（DCU/IEMS engines），Phase 2 前需补一轮独立审查。

---

*验收完成：代码质量整体可信，流程纪律与证据诚信需立即整改。*
