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

投入顺序：设计上减少误用 → 确定性代码约束与自动检查 → 验证（运行时 + 形式化）→ 文字规则。能在设计上消除的误用，先改设计；仍可能写错的地方，用可自动执行的检查让有问题的改动无法通过；复杂规则用运行时验证或形式化验证补证；改 AGENTS.md 等文档是最后的手段。本文件只承载需要判断力、且暂时无法自动化的内容；能用设计、lint / 编译器 / 测试 / 依赖图表达的，一律进代码、`ci/scripts/`、`.clang-format`、`.clang-tidy`，不在这里加文字。

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

### 验证强度（/verify 与 /interrogate）

`/verify` 用于任何“已修好 / 有改善”的声明：先写可证伪命题，跑 baseline（修复前或父提交）与 treatment（修复后）同一命令，比较原始产物，只给 `VERIFIED`、`NOT VERIFIED` 或 `INCONCLUSIVE`，并保留工件路径。没有 baseline 的结论按 `INCONCLUSIVE` 处理，不靠日志复述。

`/interrogate` 按风险分级，不一律每任务跑：

- 每任务必跑（进入集成分支前）：会删除、覆盖或迁移用户数据的；并发、事件循环、生命周期或资源所有权；解析器、协议帧或网络输入边界；安全、权限或崩溃恢复；单次 diff 超过约 500 行或跨模块。
- 可攒到每天或每批一次：纯重命名、移动、格式、注释；只加测试或文档；单文件小改且无数据、网络、并发影响，已有 focused 测试覆盖。
- 对抗评审只报能用运行证明的发现（命令、实际输出、按意图应得的输出）；verifier 逐条复跑，只保留可复现且违背意图的发现，再打回实现方。风格、命名与需要未来改动才成立的推测不计。
- 评审发现的缺陷与普通缺陷同规则：同一次改动里补可执行守卫或正反夹具，并登记 `ci/scripts/run_guard_selftest.py`。

### 架构约束

- 依赖方向：外层依赖内层；domain 不依赖框架、数据库、UI。用 `check_plane_isolation.py`、`check_component_boundary.py`、`check_product_build_graph.py` 强制，不在本文重复。
- 限界上下文之间通过消费者定义的端口交互；外部系统（表计、PLC、主站、云端）通过端口 + 适配器接入，端口头在 `foundation/include/`，实现放 `adapters/`。
- 目录结构表达边界（`modules/`、`adapters/`、`cmd/`、`foundation/`）；能用依赖图和 lint 强制的边界，不用文字重复。
- `modules/edge_core` 与受保护的 foundation 头文件只读；改动必须登记 `contracts/schema/core_readonly_whitelist.txt`。

### 变化分层（核心稳定，边缘多变）

架构设计的一个核心目标，是把变化限制在局部：高频变化发生在边缘，低频变化留在中心。细层是落位判断的尺度；目录大层仍是 `foundation/modules/adapters/cmd/web`，冲突时以现有守卫的定义为准。

| 变化层（由中心向外） | 本仓库落位 |
| --- | --- |
| 核心业务不变量 | `modules/edge_core`（只读） |
| Contract / Interface | `foundation/include/` 端口头、`contracts/` |
| Domain Model | `modules/` 领域组件 |
| Application / Use Case | `modules/` 应用服务；装配在 `cmd/` 组合根 |
| 业务策略 / Rule | 产品规则/策略模块（如 DCU 规则表、IEMS 策略） |
| Workflow / Agent / Skill | `modules/` 编排组件、`web/core` 的 app/skill 定义 |
| 客户定制 | 产品组合根与产品配置（`cmd/<product>`、`config/`、`web/products/`） |
| 外部系统适配器 | `adapters/` |
| UI / 页面 / 交互 | `web/`、产品 UI |

- 新代码按变化频率落位：能通过新增或修改边缘层（策略、适配器、UI、产品配置）满足需求，就不改中心。
- 高频需求若必须改核心不变量或契约，先按 finding 记录并走只读白名单评审，不得顺手改。
- 本节是判断类规则，无法由依赖图完全表达；可强制的依赖方向以现有确定性门禁为准，不在本文重复。

### 本仓库的确定性门禁（先查这些，别写文字规则）

- 编译告警：`ci/scripts/check_build_warnings.py`（对 CI 等价 clean build 日志；仓库源文件任何 warning 失败）
- 静态分析：`ci/scripts/run_fanalyzer.py`（`-fanalyzer` 跑全部第一方 C，新报告失败，已知误报进 `ci/baseline/fanalyzer.txt`）
- 字符串与边界：`check_strncpy_bounds.py`、`check_ftell_checked.py`、`check_net_buf_alloc_timeout.py`、`check_alloc_null.py`（构造器内分配结果先判空再解引用）
- 架构与资源：`check_plane_isolation.py`、`check_component_boundary.py`、`check_composition_root.py`、`check_public_structs.py`、`check_caller_allocated_storage.py`、`check_no_dynamic_memory.py`、`check_stack_usage.py`、`check_resource_discipline.py`
- 覆盖率：`ci/scripts/check_coverage.py`（按目录的 line coverage 下限，`ci/baseline/coverage_floor.txt`；下限只升不降）
- 运行时：ASAN/UBSAN/TSAN（CI matrix）、`ci/scripts/run_guard_selftest.py`
- 控制写与 CI 一致性：`check_iems_control_slave.py`（禁止把从站地址写死）、`check_protocol_selection.py`、`check_ci_gate_parity.py`、`check_test_io_eintr.py`、`check_privileged_test_runs.py`
- 状态行可观测性：`check_iems_stdout_line_buffered.py`（产品 stdout 必须 `setvbuf(_IOLBF)`；验收台架不得用 `stdbuf` 掩盖产品缓冲缺陷）
- 格式：`.clang-format`、`.clang-tidy`、`scripts/format.sh`

### 迁移清单（代码落地、自动化就位后删除对应条目）

- [x] import 边界 → `check_plane_isolation.py` / `check_component_boundary.py` / `check_product_build_graph.py`
- [x] 禁用 API 与写法 → `check_build_warnings.py` / `check_strncpy_bounds.py` / `check_ftell_checked.py` / `check_app_host_no_exec.py` / `check_net_buf_alloc_timeout.py` / `check_alloc_null.py`
- [x] 命名与格式 → `.clang-format` / `.clang-tidy`
- [x] 并发与性能预算 → CI 独立 `Run Performance Gates` 步骤（`EDGE_PERF_GATE=1`，阈值 `EDGE_PERF_P99_MS` / `EDGE_PERF_AVG_US`）；默认全量测试只打印延迟、不因调度噪声变红

## 角色与分工

---
`iems-verifier`
- 掌管`feat/iems`分支,是`iems-v3`迁移任务(`/home/edge/migration/`)的唯一负责人/设计者,负责给`iems-dev`派活/调度,负责验证->验收(合并到`feat/iems`和清理)
- 派活`一任务一worktree`
- 以`poteto-mode`的夜间模式持续调度,除非被喊停`收工`或者任务全部达成

---
`dcu-verifier`
- 掌管`feat/dcu`分支,是任务(`docs/plans/2026-11-21-dcu-698-production-plan.md)`)的唯一负责人/设计者,负责给`dcu-dev`派活/调度,负责验证->验收(合并到`feat/dcu`和清理)
- 派活`一任务一worktree`
- 以`poteto-mode`的夜间模式持续调度,除非被喊停`收工`或者任务全部达成

---
`edgec-verifier`
- 掌管`main`分支,是此monorepo的唯一负责人
- 确保`main`永远保持干净,不在`main`上直接开发
- 规划monorepo的路线和计划
- 负责此monorepo的架构设计和质量保证
- 接受并处理到`main`的合入请求
- 跑此monorepo的全量CI
- 收工流程见下面的`收工流程`
---
`iems-dev`
- `iems-verifier`的dev,遵守纪律和按照`poteto-mode`推进`iems-verifier`安排的任务
- 在/tmp下以worktree工作,独立上下文,不能碰main和别的分支

---
`dcu-dev`
- `dcu-verifier`的dev,遵守纪律和按照`poteto-mode`推进`dcu-verifier`安排的任务
- 在/tmp下以worktree工作,独立上下文,不能碰main和别的分支

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

### 规则 → 执行者

| 规则 | 执行者（可复现） |
| --- | --- |
| 产品状态行接管道必须及时可见，验收不得用外部 wrapper 掩盖 | `check_iems_stdout_line_buffered.py`（正反夹具 + live）；`cmd/iems/main.c` 的 `setvbuf(stdout, NULL, _IOLBF, 0)` |
| 同一协议族的量纲与帧规则只能有一处来源，禁止按传输分支复制 | `tests/test_iems_task_loader.c::test_tag_scale_modbus_family_is_one_calibration`；`tests/test_modbus_*` 的 RTU/TCP 对拍 |
| 构建不得改写已跟踪文件 | `cmake/WebBuild.cmake` 用 `npm ci`；干净 worktree 构建后 `git status` 为空（`506297c9` 已 A/B 验证：旧 `npm install` 弄脏锁文件） |
| 合并后必须在目标分支重跑守卫（单批通过不等于合并后通过） | `run_guard_selftest.py` 在合并后的 main 上跑；SRP 1500 行曾在三批合并后触发 |
| 行为、日志、启动语义变更必须同步同文件注释、设计文档状态与验收合同文字 | 人工 review（判断类，无可执行的真相来源） |
| 形式化属性必须有界、可重跑、带负例；属性违反记 finding，不得为过而放宽断言 | `./ci/scripts/check_formal.sh --strict` 与 `./ci/scripts/check_formal_negative.sh --strict`；manifest `formal/manifest/<owner>.tsv` |
| 已实现态设计文档必须引用真实文件/符号；目标态/“不存在”列举必须在同行带标记 | `ci/scripts/check_doc_references.py`（正反夹具 + live，已登记 `run_guard_selftest.py`） |
| 隐式函数声明不得通过编译（干净/增量构建一致） | `-Werror=implicit-function-declaration`、`-Werror=implicit-int`（`edge_common_interface`），CI clean build 的 `check_build_warnings.py` |
| 禁止对着旧构建产物宣布 gates 绿；构建目录必须属于当前 worktree 且无待执行工作 | `ci/scripts/check_build_freshness.py`（`run_arch_gates.sh` 与 CI arch gate 步骤在预算/符号检查之前调用；正反夹具在 `run_guard_selftest.py`） |
| 关键不变量测试必须对真实缺陷变异变红；变异后测试仍绿即门禁失败 | `ci/scripts/run_mutation_guards.py` + `ci/mutation_guards.tsv`（`--selftest` 正反自测；manifest 每条先证红后证绿） |
| 注释删除范围：只删当前 diff 新增的注释；删除既有 why 注释须在证据里逐条给理由 | 人工 review + no-comments 流程（判断类，无可执行的真相来源） |
| 守卫的 tests/fixtures 豁免必须相对扫描目标判断，禁止匹配绝对路径子串（检出目录名含 tests/guards/fixtures 时守卫会静默跳过或假红） | `ci/scripts/check_guard_path_independence.py`（正反夹具 + live，已登记 `run_guard_selftest.py`）；用 `c_source_scan.rel_scan_parts` / `skip_subtree`、`guard_baseline.is_selftest_fixture` |
