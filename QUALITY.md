# EdgeOS & EdgeC 生产级软件质量宣言 (QUALITY.md)

本文档是 **EdgeOS & EdgeC** 工业边缘操作系统的生产级软件质量声明，深度借鉴国际顶级开源通信中间件 **eprosima Fast-DDS** 与 **ROS 2 REP-2004 Quality Level 1** 严苛认证标准，结合工控储能行业高可靠性与硬实时要求制定。

---

## 1. 版本控制与稳定性政策 (Version Policy)

### 1.1 语义化版本 (SemVer)
- EdgeOS / EdgeC 严格遵循 [Semantic Versioning 2.0.0](https://semver.org/) 规范；
- 主版本号（Major）：仅允许在架构大版本升级时进行破坏性改动；
- 次版本号（Minor）：向后兼容的功能新增与非破坏性契约扩展；
- 修订版本号（Patch）：向后兼容的缺陷修复与安全性加固。

### 1.2 公共 API 稳定性
- 公共 API 集中于 `include/` 与各模块 `include/` 目录；
- **不透明指针物理封装 (Opaque Encapsulation)**：所有核心聚合根必须使用不透明指针隐藏内部结构，通过 `*_storage_t` 与 `*_create_in` 暴露内存契约，避免 ABI 破坏。

---

## 2. 变更控制与门禁流程 (Change Control Process)

### 2.1 变更申请 (Pull Requests Only)
- 任何功能新增、算法重构或缺陷修复必须通过独立 GitHub Pull Request 提交；
- 严禁直接向 `main` 主干分支推送未经审查的代码。

### 2.2 对抗式同行评审 (Adversarial Gatekeeper Review)
- 所有 PR 必须经过架构守卫（Gatekeeper）的对抗式独立复现与审查；
- 必须遵循“无失败测试不修复 (Reproduce Before Fix)”原则。

### 2.3 CI 自动化全量红线拦截
每个 PR 必须 100% 自动化通过以下 CI 质量门禁。全部 11 个守卫既跑反驳夹具，也跑真实代码树：

| 守卫 | 规则 | 当前状态 |
| :--- | :--- | :--- |
| `check_plane_isolation.py` | 四平面单向依赖（路径规则 + 模块头文件所有权） | 4 项跟踪债务 (#414) |
| `check_aggregate_boundary.py` | 聚合根不透明 + 禁止跨模块访问内部字段 | 5 项跟踪债务 (#413) |
| `check_composition_root.py` | 装配入口排他（`ci/composition_roots.txt`） | 5 项跟踪债务 (#417) |
| `check_caller_allocated_storage.py` | 存储对齐 + `_Static_assert` + 放置构造器零堆分配 | **0 违规** |
| `check_no_dynamic_memory.py` | 执行路径（`*_proc` / `*_step` / `*_run` / …）零堆分配 | 6 项跟踪债务 (#412) |
| `check_stack_usage.py` | 自动变量缓冲区 ≤ 512 B | 5 项跟踪债务 (#412) |
| `check_open_closed_plugins.py` | 禁止 `switch` / `strcmp` 巨型分发 | 1 项跟踪债务 (#415) |
| `check_single_responsibility.py` | 文件 ≤ 1500 行，函数 ≤ 200 行 | **0 违规** |
| `check_app_isolation.py` | 算法层不得触达 RTOS 内部 | **0 违规** |
| `check_dependencies.py` | 第三方依赖 40 位 SHA 锁定 | **0 违规** |
| `check_exemptions.py` | 豁免声明集中化 | **0 违规** |

另有：
1. **全量 C 单元测试 (206+ Tests, 100% Green)**；
2. **门禁反驳自测试验 (`run_guard_selftest.py`, 22 项灵敏度断言 + 11 个守卫的真实代码扫描)**；
3. **裁剪构建矩阵**：`CONFIG_EDGE_INFRA_DDS=n` / `CONFIG_EDGE_INFRA_MQTT=n` 必须可链接 (#416)。

#### 跟踪债务的记账方式 (Shrink-only ledger)
`ci/baseline/<guard>.txt` 是**唯一**的豁免来源，格式 `<path>|<RULE_ID>|<scope>|<owning-issue>`：
- 守卫发现账本中不存在的违规 → 失败（新增债务必须当 PR 修掉，或显式加账并说明问题）；
- 账本条目不再复现 → 告警，要求 `--update-baseline` 把账本向下收；
- 账本条目必须挂 issue，不允许匿名豁免。

当前账本总量 26 条，全部指向 #412 / #413 / #414 / #415 / #417。**该文件只允许缩小。**

---

## 3. 架构原则与物理真实性 (Architectural Pillars)

### 3.1 消费者定义接口 (Consumer-Defined Ports)
- 领域与控制层（Consumer）定义抽象操作表（Ops/Callbacks），底层硬件驱动（Producer）向其适配，严禁业务层反向依赖底层传输驱动。
- 由 `check_plane_isolation.py` 的 `CROSS_CONTEXT` 规则强制：模块只能包含自己拥有的头文件，或 `MODULE_ALLOWED` 白名单内的上层头文件。

### 3.2 聚合根不透明物理封装 (Aggregate Root Opaque Encapsulation)
- 公共头文件严禁暴露聚合根状态结构体字段；
- 外部使用统一遵循 **Caller-Allocated Storage** 模式，实现 0 堆内存碎片。
- 由 `check_aggregate_boundary.py` 强制：`OPAQUE_ROOT` 从结构体形状自动识别聚合根（不依赖硬编码类型名），`FIELD_ACCESS` 按类型解析拦截 `X->内部字段`。
- **已知债务**：`datahub_t` / `datahub_router_t` / `stream_engine_t` 仍在公共头中完整暴露，账本指向 #413。

### 3.3 组合根与零动态内存 (Composition Root & Zero-Alloc)
- 仅允许在 `modules/edge_bootstrap/` 与 `ci/composition_roots.txt` 列出的产品入口执行装配；
- **豁免只允许出现在 `ci/baseline/` 中**，且每条必须挂 issue、只能缩减。代码内不得再有硬编码白名单（`check_exemptions.py` 负责校验声明集中化）。

---

## 4. 测试与验证体系 (Testing & Quality Gates)

```
                            EdgeOS 测试金字塔
    ┌────────────────────────────────────────────────────────┐
    │ 1. 契约与模糊测试 (Fuzzing / Contract Verification)     │
    │ 2. 端到端物理仿真 (OMNIS Matrix Simulation)           │
    │ 3. 确定性实时死线测试 (Realtime Deadline & Circuit Breaker)│
    │ 4. 动态防御矩阵 (ASAN / TSAN / UBSAN Sanitizers)       │
    │ 5. 单元测试与微基准 (206+ CTests, 95%+ Coverage Goal)  │
    └────────────────────────────────────────────────────────┘
```

### 4.1 动态防御矩阵 (Sanitizers)
- **ASAN (AddressSanitizer)**：自动检测堆栈越界与悬垂指针；
- **UBSAN (UndefinedBehaviorSanitizer)**：检测浮点除零、严格边界溢出与未对齐指针访问；
- **TSAN (ThreadSanitizer)**：拦截多线程共享状态竞争与死锁隐患。

### 4.2 确定性实时性保障
- 关键任务均配置 AEP-720 死线监控器与 WCET 耗时统计；
- 控制线程禁止任何可能导致优先级反转或非确定性阻塞的系统调用。

---

## 5. 依赖声明与隔离 (Dependencies)

| 依赖库 | 类别 | 质量要求与隔离策略 |
| :--- | :--- | :--- |
| `libuv` | 事件循环与异步 I/O | 严格封装于控制平面与基础设施适配层 |
| `cJSON` / `QuickJS` | 配置与脚本运行时 | 隔离在 `edge_stream` / `edge_mgmt` |
| `CycloneDDS` / `PahoMQTT` | 工业总线与遥测 | 100% 通过消费者虚表动态注入，严禁头文件污染 |
| `SQLite3` | 边缘持久化 | 仅用于审计与配置存储，时序高速路径走 SHM |

---

_EdgeOS Quality Declaration - Aligned with Fast-DDS REP-2004 Level 1 Standards_
