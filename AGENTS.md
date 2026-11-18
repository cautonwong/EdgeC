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

## AI Coding 协作约束（信任金字塔）

投入顺序：确定性代码约束 → 验证 → 文字规则。本文件只承载需要判断力、且暂时无法自动化的内容；能用 lint / 编译器 / 测试 / 依赖图表达的，一律进 `ci/scripts/`、`.clang-format`、`.clang-tidy`，不在这里加文字。

### 元规则

- 修 agent 的问题，优先改成确定性检查（lint / 类型 / 测试 / 依赖图规则），而不是在本文档加一行；确需判断力的才写这里，并说明为何无法自动化。
- 规则少而高信号：不为一次性 bug 添加永久规则（agent 会把小修正过度泛化）。
- 修复必须消除整类问题（根因），禁止留 workaround；无法根因修复时停下来问人。
- 代码库是 agent 的输入：发现反模式或过时代码，先清理再扩展，否则会被下一个 agent 当作既定模式复制。
- 规则集中在本文件，其他文档只链接不复述，避免规则漂移。

### 完成定义

- 完成 = 静态分析 + 测试通过 + 运行时验证证据（测试输出 / ASAN/UBSAN/TSAN 日志 / 不变量检查 / 形式化模型）。没有验证证据不算完成。
- 验证覆盖用户视角：feature map 以 `docs/design/`（含 `app-platform/10-product-points.md`）和各产品点位表为共享词汇表，禁止模糊指代。
- 核心流程用不变量检查或状态机模型（如 `docs/design/BATCH_OWNERSHIP.md` 的 BATCH 生命周期、`net_buf` 池不变量）；关键算法用属性测试 / 模糊测试（`tests/test_fuzz_*.c`、`fuzz/`）。

### 架构约束

- 依赖方向：外层依赖内层；domain 不依赖框架、数据库、UI。用 `check_plane_isolation.py`、`check_component_boundary.py`、`check_product_build_graph.py` 强制，不在本文重复。
- 限界上下文之间通过消费者定义的端口交互；外部系统（表计、PLC、主站、云端）通过端口 + 适配器接入，端口头在 `foundation/include/`，实现放 `adapters/`。
- 目录结构表达边界（`modules/`、`adapters/`、`cmd/`、`foundation/`）；能用依赖图和 lint 强制的边界，不用文字重复。
- `modules/edge_core` 与受保护的 foundation 头文件只读；改动必须登记 `contracts/schema/core_readonly_whitelist.txt`。

### 本仓库的确定性门禁（先查这些，别写文字规则）

- 编译告警：`ci/scripts/check_build_warnings.py`（对 CI 等价 clean build 日志；仓库源文件任何 warning 失败）
- 静态分析：`ci/scripts/run_fanalyzer.py`（`-fanalyzer` 跑全部第一方 C，新报告失败，已知误报进 `ci/baseline/fanalyzer.txt`）
- 字符串与边界：`check_strncpy_bounds.py`、`check_ftell_checked.py`、`check_net_buf_alloc_timeout.py`、`check_alloc_null.py`（构造器内分配结果先判空再解引用）
- 架构与资源：`check_plane_isolation.py`、`check_component_boundary.py`、`check_composition_root.py`、`check_public_structs.py`、`check_caller_allocated_storage.py`、`check_no_dynamic_memory.py`、`check_stack_usage.py`、`check_resource_discipline.py`
- 覆盖率：`ci/scripts/check_coverage.py`（按目录的 line coverage 下限，`ci/baseline/coverage_floor.txt`；下限只升不降）
- 运行时：ASAN/UBSAN/TSAN（CI matrix）、`ci/scripts/run_guard_selftest.py`
- 格式：`.clang-format`、`.clang-tidy`、`scripts/format.sh`

### 迁移清单（代码落地、自动化就位后删除对应条目）

- [x] import 边界 → `check_plane_isolation.py` / `check_component_boundary.py` / `check_product_build_graph.py`
- [x] 禁用 API 与写法 → `check_build_warnings.py` / `check_strncpy_bounds.py` / `check_ftell_checked.py` / `check_app_host_no_exec.py` / `check_net_buf_alloc_timeout.py` / `check_alloc_null.py`
- [x] 命名与格式 → `.clang-format` / `.clang-tidy`
- [x] 并发与性能预算 → CI 独立 `Run Performance Gates` 步骤（`EDGE_PERF_GATE=1`，阈值 `EDGE_PERF_P99_MS` / `EDGE_PERF_AVG_US`）；默认全量测试只打印延迟、不因调度噪声变红

## 角色与分工

假如你是`reviewer`或`verifier`,负责派活 调度 验证 验收 合并 和清理

假如你是`dev`,日常提交不必逐个跑`ocr`(单次成本高);收工时对当日全部改动跑一次`ocr review`, 其使用方法见`/home/edge/ocr-open-code-review-usage-2026-11-18.md`

## 收工流程

当用户说"收工"时,你要按照以下步骤做:
- 该合并的合并
- 跑一遍本地CI
- 对当日全部改动跑一次 `ocr review --audience agent`,建议 `--from <当日起点>`(日常提交不逐次跑)
- 该清理的清理
- 编写详细的交接文档到/home/edge目录下 文件名带有项目名+日期
- 制作相对于远端最新的.patch 要求别人apply你的patch之后能跟你本地一模一样


## 错误 → 守卫

每修一个错误，在同一次改动里加一条可执行的守卫或正反夹具，并登记到 `ci/scripts/run_guard_selftest.py`。只写交接文档不算完成。
