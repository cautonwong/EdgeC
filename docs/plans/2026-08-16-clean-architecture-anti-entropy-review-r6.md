# Codex 六审：V5.1 终审 + Ticket 推进红线与验收标准宪章

> **Mr. WKT** — 评审对象：主计划 V5.1（54 个 AEP 工单）与 ADR-0008 V5。本审两件事：①确认 R1~R5 全部闭环；②发布《Ticket 推进红线与验收标准宪章》（可直接作为 AGY 与 Codex 的共同规则）。

---

## 0. 六审结论

**V5.1 已完整吸收一审至五审全部意见，评审闭环。**

| 五审遗留 | V5.1 状态 |
|---|---|
| ADR-0008 未同步 | ✅ 已升 V5：状态 `Proposed`、写明修订 ADR-0005、继承 ADR-0007、含 ADR 生命周期 |
| AEP-001 默认提交 WIP | ✅ 已改为“根据 Mr. WKT 指令（全部提交/部分提交/移出）” |
| TagID 分域迁移策略 | ✅ 五审意见已体现（需在 AEP-352 执行时再验证存量兼容） |
| 合规/认证缺失 | ✅ AEP-512/513 已补 |
| 性能预算缺失 | ✅ AEP-514 已补 |
| 上游推送规范 | ✅ AEP-515 已补 |
| MISRA 试点 | ✅ AEP-516 已补 |

**批准建议**：V5.1 架构与执行框架通过；首期下达 Phase 0 + Phase 1（AEP-001~006、AEP-101~105，共 11 张）；执行过程必须遵守本文《红线与验收标准宪章》。

---

# 附：《EdgeOS Ticket 推进红线与验收标准宪章》（V1.0）

## 1. 红线总纲（Red Lines）

红线分三级：**致命（FATAL）＝一票否决，立即停止；严重（HIGH）＝不得合并，必须整改；警告（WARN）＝限期处理，超期升级。**

### 1.1 全局红线（所有 Ticket 通用）

| # | 红线 | 级别 | 判定方式 |
|---|---|---|---|
| R-01 | 未冻结基线（Phase 0 未完成）不得启动 Phase 1+ | FATAL | 基线 Tag 不存在即阻断 |
| R-02 | 禁止把未评审 WIP 混入重构提交 | FATAL | PR 文件清单与 AEP 范围不一致即拒 |
| R-03 | 禁止手改生成物（contracts.c/tags.go/registry/IDL） | FATAL | CI 重生成 diff 非空即失败 |
| R-04 | 禁止 Mock/硬编码假数据冒充物理数据（Zero-Mock） | FATAL | 代码审查 + anti-mock 扫描 |
| R-05 | 禁止硬编码密钥/口令/TagID | FATAL | `rg` 扫描 + 审查 |
| R-06 | 禁止削弱测试或门禁（删测试、放宽阈值、跳过 CI、注释失败用例） | FATAL | CI 配置 diff 审查 |
| R-07 | 禁止在 CI 红时合并；CI 红超 48h 必须上报 | FATAL | CI 状态 + 台账 |
| R-08 | 禁止修改 `modules/edge_core` 白名单外文件；核心修改双人评审，豁免必须留痕 | HIGH | CODEOWNERS + 审查记录 |
| R-09 | 单个提交必须对应单个 Ticket，禁止大杂烩提交（默认 ≤500 行，例外注明） | HIGH | `git show --stat` |
| R-10 | 禁止 `git push --force` 到共享分支（历史重写需 Mr. WKT 批准） | HIGH | reflog/推送日志 |
| R-11 | 禁止把“只改测试不改产品”当作完成（历史教训 T-101/T-102） | HIGH | 审查：测试必须能复现产品行为 |
| R-12 | 安全红线不得回退：RBAC、UDS 解锁、加密、私钥管理、OTA 校验 | FATAL | 安全专项测试 |
| R-13 | 数据红线不得破坏：WAL/快照/压实/SnF 语义（ADR-0007） | FATAL | 专项回归 |
| R-14 | 二开能力验收必须“纯安装环境构建通过”，不能用源码树凑数 | HIGH | CI 独立 job |

### 1.2 阶段红线

| 阶段 | 红线 |
|---|---|
| Phase 0 | 未打 `baseline-2026-08-16` Tag 不得开工 Phase 1；证据未归档不得宣称“基线全绿” |
| Phase 1 | `src/` 未清零不得进入 Phase 2；移动文件后 `nm -g` 符号差异必须为 0 |
| Phase 2 | 接口未定义（storage_ops_t/datahub_ops_t）不得先搬文件；WAL/SnF 专项不过不得宣布完成 |
| Phase 3 | 生成式注册表未通过 `nm` 完整性断言不得替代手工 extern；DX 测试不绿不得发布 edge-cli |
| Phase 3.5 | 存量 TagID 迁移方案未证明“零破坏”不得执行分域；跨产品同名冲突未查重不得放行 |
| Phase 4 | 独立示例未在纯安装环境通过，SDK 导出不得宣布完成；ABI golden 未锁定不得发大版本 |
| Phase 5 | 每个治理机制必须有“可执行证据”，不允许只交文档 |

## 2. 验收标准（Acceptance Criteria）

### 2.1 通用 DoD（每个 Ticket 必须全部勾选）

```text
□ 分支命名 ticket/AEP-xxx；提交信息含 AEP 编号
□ 变更范围与 AEP 定义一致（无夹带）
□ ./build.sh 通过
□ ./test.sh 通过（当前 134/134，基线冻结后以归档数为准）
□ ./test.sh --omnis 通过（10/10）
□ cd go && go test ./... 通过
□ scripts/check_tag_consistency.sh 通过
□ scripts/check_repo_clean.sh 通过
□ 涉及核心模块：ASan/TSan 相关 job 通过
□ 无 R-01~R-14 红线违反
□ 证据包落盘（见 §2.3）
□ 文档/ADR/词汇表按需同步
```

### 2.2 阶段出口门禁（Gate）

| 阶段 | 出口 = 全部满足 |
|---|---|
| Phase 0 | 基线 Tag + 证据日志 + CI 参数修复 + WIP 决策落地 + 红线台账建立 |
| Phase 1 | `src/` 零引用 + 死树删除 + `nm`/`rg` 门禁入 CI + 134/134 + 10/10 |
| Phase 2 | core 依赖白名单 + `nm -u` 门禁 + 尺寸报告（WARN 阈值）+ WAL/SnF 专项 + 三产品构建 |
| Phase 3 | 生成式注册表 + main.c 无 extern 注册 + DX 测试 + 生成物可信链 |
| Phase 3.5 | CONTEXT-MAP/词汇表 + Tag 分域校验 + ACL 测试 + 场景用例 |
| Phase 4 | 纯安装环境示例 + ABI golden + SBOM/许可 + migrate 演示 |
| Phase 5 | 10 项机制每项有 CI/流程证据 |

### 2.3 证据包（每个 Ticket 提交时必须附带）

```text
docs/evidence/<AEP-xxx>/summary.md     # 改了什么、为什么、如何满足 DoD
docs/evidence/<AEP-xxx>/tests.log      # ctest/go/omnis 关键输出
docs/evidence/<AEP-xxx>/redlines.md    # 红线自查表
docs/evidence/<AEP-xxx>/diffstat.txt   # git show --stat
```

## 3. Ticket 状态机与推进流程

```text
Backlog → In Progress → Submit for Review → Codex Review → Done
                                    ↓                        ↑
                                Fail / Pass-with-notes ─────┘
                                    ↓
                                Blocked（需 Mr. WKT 决策）
```

规则：

1. **进入 In Progress**：必须有明确指派（AGY-A/B/C）与分支 `ticket/AEP-xxx`。
2. **Submit for Review**：必须附证据包；无证据包一律退回。
3. **Codex Review**：按 §4 协议执行；结论 Pass / Pass-with-notes / Fail。
4. **Fail**：退回同一分支整改，禁止开新分支掩盖历史。
5. **Blocked**：需要 Mr. WKT 决策（如 AEP-001 WIP 处理、AEP-352 迁移方案）；Blocked 期间不得绕过。
6. **Done**：合并到 `feature/modular-refactor` 后立即跑一次全量 CI；合并后 24h 内无红灯才算稳定。
7. **同一阶段内 Ticket 可并行**；跨阶段必须 Gate 全绿。

## 4. Codex 审核协议

每张 Ticket 按双轴审核：

**Standards 轴**：EdgeOS 宪法（四平面、Zero-Mock、Consumer-Driven、非侵入）+ 仓库规范 + Fowler 气味基线 + 红线扫描。

**Spec 轴**：对照 AEP 原文与依赖链，检查：缺什么、多做了什么（scope creep）、做错了什么。

评分：

| 维度 | 分值 |
|---|---|
| 完成度（DoD 勾选率） | 0-100 |
| 代码质量 | A-F |
| 测试充分性 | A-F |
| 红线违反 | 0 次=通过；1 次 FATAL=直接 Fail |

结论：`Pass`（可合并）／`Pass-with-notes`（可合并但限期整改）／`Fail`（不得合并）。

## 5. 异常与回滚

1. **红灯即停**：任何 FATAL 红线或 CI 红，立即停止该 Ticket 推进。
2. **回滚**：每 Ticket 原子提交；回滚 = `git revert <commit>`，禁止 `git reset --hard`（除非 Mr. WKT 明确批准）。
3. **48h 规则**：CI 红超 48h 未修复 → 自动升级为 Blocked 并通知 Mr. WKT。
4. **基线保护**：`baseline-2026-08-16` Tag 只读；任何回退到基线前的操作需批准。

## 6. 报告与指标

每周一产出《AEP 推进周报》：

- 状态分布（Done / In Progress / Review / Blocked）；
- 红线违反台账（次数、级别、整改情况）；
- CI 绿/红时长与失败样本；
- 熵指标（复杂度、耦合、TODO 密度、生成物漂移次数）；
- 下一周计划与阻塞清单。

## 7. 签署与生效

本宪章随 V5.1 一并生效，适用对象：AGY（执行）、Codex（审核）、Mr. WKT（决策）。如需修改红线，必须走 ADR 流程，禁止口头豁免。

---

*六审完成：V5.1 评审闭环；红线宪章 V1.0 可直接作为 AGY 派工与 Codex 验收的共同规则。*
