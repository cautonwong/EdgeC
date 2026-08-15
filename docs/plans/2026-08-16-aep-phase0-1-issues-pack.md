# AEP Phase 0 + Phase 1 —— GitHub Issues 派工包（11 张）

> 依据 `docs/agents/issue-tracker.md`：仓库 Issues 使用 GitHub + `gh` CLI。
> 当前环境 `gh` 未安装且无 Token；本文件为**可直接导入的票包**。gh 就绪后按 §12 批量创建。
> 标签规范见 `docs/agents/triage-labels.md`。

---

## 通用标签

- Phase 0：`ready-for-agent`、`p0`
- Phase 1：`ready-for-agent`、`p1`
- AEP-001：`needs-info`（等待 Mr. WKT 决策）、`p0`

## 1. AEP-001 收口未提交 WIP

- 标签：`ready-for-agent`、`p0`、`needs-info`
- 依赖：无
- 目标：按 Mr. WKT 指令处理约 25+ 个未提交文件（DCU/IEMS/edge_bsp_hal/SunSpec 等），建立基线快照。
- 决策待定：A=全部提交；B=仅提交评审过的；C=移出冻结。
- DoD：
  - [ ] 按决策结果执行，`git status` 干净或清单化
  - [ ] 创建 Tag `baseline-2026-08-16`
  - [ ] 证据：`git log --oneline -5`、`git tag -n1`

## 2. AEP-002 基线证据归档

- 标签：`ready-for-agent`、`p0`
- 依赖：AEP-001
- DoD：
  - [ ] `./build.sh` 通过
  - [ ] `./test.sh`：CTest 134/134
  - [ ] `./test.sh --omnis`：10/10
  - [ ] `cd go && go test ./...` 通过
  - [ ] 产出 `docs/baselines/2026-08-16-base-134.log`

## 3. AEP-003 修复 CI 参数

- 标签：`ready-for-agent`、`p0`
- 依赖：AEP-002
- DoD：
  - [ ] `-DENABLE_COVERAGE=ON` → `-DCOVERAGE=ON`
  - [ ] ASan/TSan 矩阵实际运行 134 项
  - [ ] 覆盖率 job 产出 gcda 证据
  - [ ] CI 全绿截图/日志归档

## 4. AEP-004 R3 未闭合项收口（T-002 剩余）

- 标签：`ready-for-agent`、`p0`
- 依赖：无
- 范围：HMAC 密钥外置、DataHub 回写、去硬编码 DTC/ECU/Routine、补 Go 测试。
- DoD：
  - [ ] `go/cmd/smart-box-gateway/main_test.go` 新增用例全绿
  - [ ] `rg` 无硬编码密钥
  - [ ] UDS 状态经 DataHub 写契约 Tag

## 5. AEP-005 评审意见双向映射

- 标签：`ready-for-agent`、`p0`
- 依赖：无
- DoD：
  - [ ] `docs/plans/TICKET_MAPPING.md` 落盘（R1~R6 ↔ AEP ↔ T-xxx）

## 6. AEP-006 核心目录只读门禁原型

- 标签：`ready-for-agent`、`p0`
- 依赖：AEP-001
- DoD：
  - [ ] CODEOWNERS 配置生效
  - [ ] 改动 `modules/edge_core` 的 PR 触发警告标记
  - [ ] 门禁演示截图/日志

## 7. AEP-101 平移 4 个存活文件

- 标签：`ready-for-agent`、`p1`
- 依赖：AEP-001
- 文件：`src/main_loop/northbound_mgr.c`、`sched_engine.c`、`sched_port.c`、`src/platform/linux/io_executor_linux.c` → `modules/edge_io/src/`
- DoD：
  - [ ] `rg 'src/main_loop|src/platform' modules/` 输出为 0
  - [ ] CTest 134 通过；`nm -g` 符号对比无缺失

## 8. AEP-102 重构 edge_io CMake

- 标签：`ready-for-agent`、`p1`
- 依赖：AEP-101
- DoD：
  - [ ] `edge_io/CMakeLists.txt` 无 `${CMAKE_SOURCE_DIR}/src`
  - [ ] CTest 134 通过

## 9. AEP-103 死代码审计报告

- 标签：`ready-for-agent`、`p1`
- 依赖：AEP-101
- DoD：
  - [ ] `docs/reports/dead_code_audit.md` 输出（引用分析 + 删除清单）

## 10. AEP-104 删除旧 src/

- 标签：`ready-for-agent`、`p1`
- 依赖：AEP-103
- DoD：
  - [ ] 删除后 build + CTest 134 + Omnis 10 全绿
  - [ ] `git status` 无 src/ 残留

## 11. AEP-105 路径与符号回归门禁

- 标签：`ready-for-agent`、`p1`
- 依赖：AEP-104
- DoD：
  - [ ] `scripts/check_no_old_src.sh` 加入 CI
  - [ ] `nm -g` 符号对比脚本加入 CI
  - [ ] 故意引入跨目录引用 → CI 红（演示证据）

---

## 12. 批量创建命令（gh 就绪后执行）

```bash
# 在 /workspaces/edgeos/EdgeC 下执行；每条使用 heredoc 传入 body
gh issue create --title "AEP-001 收口未提交 WIP（等待 Mr. WKT 决策）" --label "ready-for-agent,p0,needs-info" --body "见 docs/plans/2026-08-16-aep-phase0-1-issues-pack.md §1"
# …… 依此类推（AEP-002 ~ AEP-105）
```

> 也可由 Codex 在 `gh auth login` 完成后一键批量创建；创建后回写 Issue 编号到 `TICKET_MAPPING.md`。

---

*票包完毕：11 张全部含标签、依赖、DoD 与证据要求；AEP-001 待 Mr. WKT 决策 WIP 处理方式。*
