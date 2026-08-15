# AGY 派单指令：Phase 0 + Phase 1（11 张 Ticket）

> **Mr. WKT / Codex → AGY** — 生效日期 2026-08-16。请严格按本文工作纪律执行；验收人为 Codex，最终决策人为 Mr. WKT。

---

## 1. 任务清单（GitHub Issues）

| Issue | Ticket | 标题 | 标签 | 状态 |
|---|---|---|---|---|
| [#67](https://github.com/cautonwong/EdgeC/issues/67) | AEP-001 | 收口未提交 WIP | p0, ready-for-agent | ✅ 已解锁：Mr. WKT 决策=全部提交（A） |
| [#68](https://github.com/cautonwong/EdgeC/issues/68) | AEP-002 | 基线证据归档 | p0 | 依赖 AEP-001 |
| [#69](https://github.com/cautonwong/EdgeC/issues/69) | AEP-003 | 修复 CI 参数 | p0 | 依赖 AEP-002 |
| [#70](https://github.com/cautonwong/EdgeC/issues/70) | AEP-004 | R3 未闭合项收口（T-002 剩余） | p0 | **可立即开工** |
| [#71](https://github.com/cautonwong/EdgeC/issues/71) | AEP-005 | 评审意见双向映射 | p0 | **可立即开工** |
| [#72](https://github.com/cautonwong/EdgeC/issues/72) | AEP-006 | 核心目录只读门禁原型 | p0 | 依赖 AEP-001 |
| [#73](https://github.com/cautonwong/EdgeC/issues/73) | AEP-101 | 平移 4 个存活文件 | p1 | 依赖 AEP-001 |
| [#74](https://github.com/cautonwong/EdgeC/issues/74) | AEP-102 | 重构 edge_io CMake | p1 | 依赖 AEP-101 |
| [#75](https://github.com/cautonwong/EdgeC/issues/75) | AEP-103 | 死代码审计报告 | p1 | 依赖 AEP-101 |
| [#76](https://github.com/cautonwong/EdgeC/issues/76) | AEP-104 | 删除旧 src/ | p1 | 依赖 AEP-103 |
| [#77](https://github.com/cautonwong/EdgeC/issues/77) | AEP-105 | 路径与符号回归门禁 | p1 | 依赖 AEP-104 |

**启动顺序**：
1. 立即开工：AEP-004、AEP-005（无依赖）；
2. AEP-001 已解锁（决策 A=全部提交）→ AEP-002 → AEP-003；
3. 依赖 AEP-001 的任务（AEP-006、AEP-101~105）在 AEP-001 完成后按序启动；
4. **禁止在 AEP-001 未决策时自行决定 WIP 去向（提交/回退/冻结）。**

## 2. 工作纪律（红线，违反即返工）

完整红线见 `docs/plans/2026-08-16-clean-architecture-anti-entropy-review-r6.md`。以下为必须遵守的摘要：

### FATAL（一票否决，立即停止）
- 不手改生成物（contracts.c / tags.go / 注册表 / IDL）；CI 重生成 diff 非空即失败；
- 不 Mock / 不用硬编码假数据冒充物理数据（Zero-Mock）；
- 不硬编码密钥 / 口令 / TagID；
- 不削弱测试或门禁（不删测试、不放宽阈值、不跳过 CI、不注释失败用例）；
- 不在 CI 红时合并；CI 红超 48h 必须上报；
- 不破坏 WAL/快照/压实/SnF 语义（ADR-0007）；
- 不回退安全红线（RBAC、UDS 解锁、加密、私钥、OTA 校验）。

### HIGH（不得合并，必须整改）
- 不修改 `modules/edge_core` 白名单外文件；核心修改需双人评审，豁免必须留痕；
- 每个提交只对应一个 Ticket，禁止大杂烩（默认 ≤500 行，例外注明）；
- 不 `git push --force` 到共享分支；
- 不用“只改测试不改产品”冒充修复（历史教训 T-101/T-102）；
- 二开/生成物相关验收必须在干净/纯安装环境可复现。

## 3. 提交与分支规范

- 分支：`ticket/AEP-xxx`（一个 Ticket 一个分支，禁止复用）；
- 提交信息：`feat|fix|docs|test(AEP-xxx): 一句话说明`，正文附 DoD 勾选；
- 合并：仅 `feature/modular-refactor`；未经 Codex Pass 不得合并；
- 回滚：`git revert`，禁止 `git reset --hard`（除非 Mr. WKT 批准）。

## 4. 验收与证据要求（每个 Ticket 提交时必须附带）

```text
docs/evidence/<AEP-xxx>/summary.md     # 改了什么、为什么、DoD 逐项勾选
docs/evidence/<AEP-xxx>/tests.log      # build/ctest/go/omnis 关键输出
docs/evidence/<AEP-xxx>/redlines.md    # 红线自查表（逐条）
docs/evidence/<AEP-xxx>/diffstat.txt   # git show --stat
```

验收命令基线（当前）：`./build.sh`、`./test.sh`（CTest 134）、`./test.sh --omnis`（10）、`cd go && go test ./...`、`scripts/check_tag_consistency.sh`、`scripts/check_repo_clean.sh`。

## 5. 协作与上报

- 每张 Ticket 完成后：附证据包 → 提交 PR → 通知 Codex 复审；
- 评审结论：`Pass`（合并）/ `Pass-with-notes`（合并+限期整改）/ `Fail`（退回同一分支整改）；
- 阻塞（依赖缺失、需求歧义、红线冲突）：立即在 Issue 留言并标记 `needs-info`，**不要自行扩权**；
- 每周一输出《AEP 推进周报》（状态分布、红线台账、CI 红绿、阻塞清单）。

## 6. 特别提醒

1. AEP-001 已由 Mr. WKT 决策为 A=全部提交；执行时清理临时日志、按规范提交并打 Tag `baseline-2026-08-16`；
2. AEP-004 属于安全收口：HMAC 密钥必须走环境变量、UDS 状态必须走 DataHub、DTC/ECU/Routine 禁止硬编码；
3. AEP-101 平移文件后必须做 `nm -g` 符号对比，防止“能编译但符号丢失”；
4. AEP-105 的回归门禁要先证明“故意违规会红”，再宣布完成。

---

*派单完毕。Phase 0 已解锁；Codex 将按红线宪章逐票验收。*
