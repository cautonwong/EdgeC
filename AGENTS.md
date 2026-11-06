platform: linux-app

## Agent skills

### Issue tracker

GitHub issues via `gh`; external PRs are not a triage surface. See `docs/agents/issue-tracker.md`.

### Triage labels

Five canonical triage roles use their default label names. See `docs/agents/triage-labels.md`.

### Domain docs

Multi-context: `CONTEXT-MAP.md` at the root points to per-context `CONTEXT.md` files. See `docs/agents/domain.md`.

## 选择流程

- 全新模块、不依赖已有逻辑 → /grill-embedded → /to-spec-embedded
- 在已有代码上增加或修改功能(含多客户分支) → /survey-existing → /grill-change → /to-spec-change
- 混合情形按"在已有代码上改动"处理
- 重构(行为不变、结构改变) → /survey-existing → /plan-refactor;不与功能变更、缺陷修复混在同一个 ticket
- 分支收敛、重构、既有缺陷修复单独立项,不与功能变更混在同一个 ticket
- 所有 spec 经人工评审后,才由评审人打 ready 标签
- `/tests-from-design` 生成的测试经人工评审后才开始实现;实现阶段不得修改这些测试,确需修改先改 spec 并重新评审
