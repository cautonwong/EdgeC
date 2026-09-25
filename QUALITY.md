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
每个 PR 必须 100% 自动化通过以下 CI 质量门禁：
1. **全量 C 单元测试 (206+ Tests, 100% Green)**；
2. **四平面单向依赖隔离门禁 (`check_plane_isolation.py`)**；
3. **零动态内存红线门禁 (`check_no_dynamic_memory.py`, 0 malloc/calloc in domain code)**；
4. **调用方存储对齐与大小断言门禁 (`check_caller_allocated_storage.py`)**；
5. **门禁反驳自测试验 (`run_guard_selftest.py`, 12/12 灵敏度测试 100% 通过)**。

---

## 3. 架构原则与物理真实性 (Architectural Pillars)

### 3.1 消费者定义接口 (Consumer-Defined Ports)
- 领域与控制层（Consumer）定义抽象操作表（Ops/Callbacks），底层硬件驱动（Producer）向其适配，严禁业务层反向依赖底层传输驱动。

### 3.2 聚合根不透明物理封装 (Aggregate Root Opaque Encapsulation)
- 公共头文件严禁暴露状态结构体具体字段；
- 外部使用统一遵循 **Caller-Allocated Storage** 模式，实现 0 堆内存碎片。

### 3.3 组合根与零动态内存 (Composition Root & Zero-Alloc)
- 仅允许在唯一的组合根（`main.c` / `composition_root.c`）执行静态装配；
- 生产代码全面废除白名单豁免机制 (Zero-Exemptions Policy)。

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
