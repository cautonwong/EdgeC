# Codex 深度评审：ADR-0008 防腐化宪章 + 反熵主计划（2026-08-16）

> **Mr. WKT** — 评审对象：`docs/adr/0008-anti-entropy-clean-architecture-charter.md` 与 `docs/plans/2026-08-16-clean-architecture-anti-entropy-master-plan.md`。
> 评审方法：文档逐条审读 + 与仓库现状、既有 ADR（0003~0007）、R1~R3 评审反馈交叉核对 + 实测基线验证（CTest 134、Omnis 10/10、依赖与目录结构抽查）。

---

## 0. 总体结论

**方向正确，值得批准，但“批准即执行”太早。** 建议先做三件事再开工：

1. 新增 **Phase 0：基线冻结与止血**（收口未提交 WIP、锁定 134/134 + 10/10 证据、修复 CI 参数、把 R3 未闭合项并入）。
2. 把计划中的 **断言、依赖白名单、验收门禁** 全部改成“可机械验证”（当前多个 DoD 是口头表述）。
3. 补齐 **契约治理（T-201/T-202/T-204/T-205）、安全 backlog、DDD 建模任务、时间与票粒度**——否则四个阶段跑完仍会在“同一棵树上长出第二棵森林”。

---

## 1. 文档质量与一致性

### 1.1 ADR-0008 的优点

- 四大致腐反模式抓得准（God File、依赖倒挂、二开污染、领域混乱），与仓库现状高度吻合。
- 目标架构（四层洋葱 + Ports & Adapters + 自注册 + Out-of-Tree）与 EdgeOS 现有四平面模型兼容，属于“收敛”而非“另起炉灶”。
- 治理铁律简洁可传播（OCP、纯内核、二开隔离、ACL）。

### 1.2 主要问题

1. **状态标注矛盾**：ADR 已写 `Status: Accepted`，但评审尚未完成、审批人刚收到计划。应改为 `Proposed`，经 Mr. WKT 批准后再改 `Accepted`。
2. **与 ADR-0005 的关系未交代**：ADR-0005 刚把 SHM/DDS/MQTT 三后端放进 `edge_core`；ADR-0008 又要求全部下沉 `edge_infra`。两者不是冲突，而是“先落地后收敛”，但必须写明 **ADR-0008 修订 ADR-0005 的部署位置**，否则后续实现会互相打架。
3. **与 ADR-0007（存储加固）的关系未交代**：SQLite/WAL/SnF 迁移到 `edge_infra` 时，0007 的崩溃一致性、WAL 默认值、缓冲韧性必须原样保留并补回归证据。
4. **“核心只读”与当前 monorepo 事实冲突**：`cmd/`、`modules/`、`src/` 都在同一仓库，二开产品也在树内。宪章没说明这些现有产品是“保留为参考产品”还是“逐步迁出”。必须二选一并写清楚。
5. **无证据断言**：宪章第 1 条说 `main.c`/`init.c` 有“上百个 #ifdef”，但当前 `cmd/iems/main.c` 等未发现该形态。建议改为“未来多产品必然出现”的风险推演，而不是把现状描述成事实。

## 2. 现状断言核实（实测）

| 计划/宪章断言 | 实测 | 结论 |
|---|---|---|
| CTest 134/134 基线 | `ctest -N` = 134（含 IDL 序列化测试） | ✅ 属实 |
| Matrix 10/10 基线 | 2026-08-15 实测 10/10（48s） | ✅ 属实（但需在基线冻结后复跑） |
| `edge_core` 静态链接 SQLite/PahoMQTT/CycloneDDS/zlog/cJSON | `modules/edge_core/CMakeLists.txt` 确认 | ✅ 属实 |
| 双重源码树（`src/` 与 `modules/` 并存、`edge_io` 跨目录引用） | `edge_io/CMakeLists.txt` 引用 `src/main_loop/*`、`src/platform/linux/*` | ✅ 属实 |
| CycloneDDS 子模块断链（R3 P0-1） | d7001ce 已改为 vendored 库优先 + 删除 gitlink | ✅ 已修复 |
| IDL 生成未接入 CMake（R3 P0-2） | d7001ce 已在 `tests/CMakeLists.txt` 增加 idlc custom target | ✅ 已修复 |
| “YAML 错误被吞” | R2 已让 loader 返回错误；validator 已加 strict | ✅ 基本属实 |
| “God File 上百 #ifdef” | 未在现网代码发现 | ⚠️ 无证据，建议改写 |

**基线风险**：仓库当前有大量未提交改动（DCU daemon/fusion/load-control 等新文件 + CI/gateway/CMake 修改）。计划宣称“在 134/134 与 10/10 之上零休克”，但基线正在漂移。执行前必须收口或冻结。

## 3. 架构可行性评审

### 3.1 自注册宏（Phase 3）——方向对，机制要写死

- C 静态库中“段符号自动注册”**不会自动生效**：链接器默认丢弃未引用段。必须明确：
  - 注册段名（如 `.edge_driver`）与 `KEEP()` 链接脚本；
  - 或 `__attribute__((constructor))` + 优先级；
  - 或目标链接时 `--whole-archive`（当前测试已部分使用，但产品二进制不是）。
- 需要定义：初始化顺序（`EDGE_INIT_PRIORITY` 的数值约定）、重复注册检测、模块卸载/热插拔语义。
- DoD 必须可验证：`nm` 检查符号、注册表顺序确定性、新增驱动“零核心改动”的集成测试。

### 3.2 纯内核（Phase 2）——依赖白名单要统一

- 宪章说 `edge_core` 只允许 C 标准库 + `libuv`；但 Phase 2 DoD 只提“不再链接 SQLite/MQTT”。
- `edge_core` 目前还链 `zlog`、`cJSON`、`CycloneDDS`。**请明确最终白名单**：
  - 建议：`libuv` 保留（事件循环是运行时底座）；`zlog` 下沉或允许（日志是横切关注点，需单独决策）；`cJSON`/SQLite/MQTT/DDS 全部下沉。
- 增加机械门禁：
  - CI 用 `nm -u`/`ldd` 校验 `edge_core.a` 的未定义符号集合；
  - 新增静态库尺寸任务（宪章 150KB 不变量）并纳入 CI 阈值。

### 3.3 SQLite/WAL/SnF 迁移（Phase 2）——影响面最大

- `sqlite_service` 被 StreamEngine、edge_io、DCU、device_twin、WAL/压实、SnF 广泛使用；直接搬文件 = 符号与初始化顺序全部重排。
- 建议拆三个子步：**2a 接口化**（定义 `storage_ops_t` + 注入点）→ **2b 迁移实现**（`edge_infra/storage`）→ **2c 清理**（删旧引用、更新 CMake/Kconfig）。
- 每一步都跑：`nm -g` 符号清单对比 + 134 CTest + Omnis 10/10 + 针对 WAL/快照/压实（ADR-0007）的专项用例。

### 3.4 Out-of-Tree SDK（Phase 4）——交付物不止 cmake 文件

- `EdgeOSConfig.cmake` 只是入口，真正难点：
  - 头文件、静态库、**第三方依赖**（libuv/SQLite/zlog/Paho/CycloneDDS）的安装与发布策略；
  - ABI 稳定与版本化：`PipeMsg` 64 字节、`edge_tag_meta_t`、ops 虚表、TagID 契约必须冻结并加编译期断言；
  - `find_package(EdgeOS)` 示例必须在 CI 里用“仅安装包”的干净环境编译，证明不依赖源码树；
  - 许可审查（CycloneDDS、QuickJS、WAMR、open62541 等）。

### 3.5 DDD / 界限上下文——计划最薄弱处

- 宪章讲了大道理，但主计划**没有任何领域建模任务**：界限上下文清单、聚合根、防腐层（ACL）映射、领域 Tag 命名空间（对应既有 T-202）全部缺失。
- 建议新增“Phase 3.5 领域建模与契约治理”或把任务并入 Phase 3/4：
  - DCU/IEMS/SmartBox/NexusLab 的界限上下文与共享内核目录；
  - ACL 层把 DLT698/Modbus 寄存器映射为领域实体（禁止原始地址穿透）；
  - TagID 分域与编译器强校验（复用 contract-compiler-v2 validator + T-202 规则）。

## 4. 阶段任务评审

| 阶段 | 优点 | 缺口 / 必须补 |
|---|---|---|
| Phase 1 双重源码树清理 | 目标清晰、DoD 可检查 | 先做基线 tag；`rg` 门禁防回潮；移动文件后 `nm` 对比 |
| Phase 2 核心解耦 | 方向正确、拆子目录合理 | 依赖白名单统一；150KB 门禁；SQLite/WAL/SnF 专项回归；CycloneDDS 保留 vendored fallback |
| Phase 3 自注册 + edge-cli | DX 价值高 | 链接机制写死；注册顺序/重复检测；`lint-contract` 复用 compiler-v2 validator；DX 测试入 CI |
| Phase 4 Out-of-Tree | 补齐二开闭环 | SDK 安装/依赖/许可/ABI 策略；CI 用“仅安装包”验证；示例项目独立构建 |
| （缺失）Phase 0 基线冻结 | — | 收口未提交 WIP；134/134+10/10 证据存档；修 CI 参数（COVERAGE/Go 版本）；R3 未闭合项（T-002 剩余、ASan 矩阵验证） |

## 5. 关键缺失清单（必须补）

1. **Phase 0 基线冻结**（最重要）。
2. **安全与 Zero-Mock backlog 范围声明**：私钥历史、UDS/DoIP、硬编码密钥、WASM 失败回退——不进本计划就写“另行跟踪”，不能消失。
3. **契约与 Tag 治理任务**：编译器校验、TagID 命名空间、C/Go 单源、Blob 签名（T-201~T-205）。
4. **依赖/尺寸/头文件方向门禁**：CI 可机械执行。
5. **时间、人力、票粒度、里程碑**：建议按 3 个 agent 拆工作流，每个 Phase 拆 Ticket（复用既有模板）。
6. **文档同步任务**：ADR-0008 生效后更新 README/EDGEOS_DEV_HANDBOOK/SDK_TIERS。
7. **CI 现状修复**：`-DENABLE_COVERAGE=ON` vs `COVERAGE`、Go 1.21 vs `go.mod 1.25.5`、ASan job 是否真的在矩阵里生效——计划说“全量测试持续全绿”，但当前 CI 配置本身还没全绿。
8. **回滚与证据协议**：每阶段一个基线 tag；每 Ticket 附 DoD 勾选 + 测试日志 + 符号表对比。

## 6. 风险登记

| 风险 | 等级 | 缓解 |
|---|---|---|
| 未提交 WIP 与重构并行 → 基线漂移 | 高 | Phase 0 先冻结/收口，禁止重构与功能开发混提 |
| 自注册宏在静态库被裁剪 | 高 | 链接脚本/whole-archive 方案先行验证，写进 DoD |
| SQLite/WAL/SnF 迁移破坏崩溃一致性 | 高 | 2a/2b/2c 拆分 + ADR-0007 专项回归 |
| 纯内核白名单不统一 → 宪章成空文 | 中 | CI 依赖符号门禁 + 150KB 门禁 |
| Out-of-tree SDK 依赖分发/许可 | 中 | Phase 4 前完成许可与安装策略评审 |
| 领域建模缺失 → Tag 空间继续混乱 | 中 | 补 Phase 3.5，复用 T-202 |
| 计划规模过大、一次性批准 | 中 | 分阶段批准：先批 Phase 0+1 |

## 7. 修改建议（按优先级）

### Must（批准前必须改）
1. ADR-0008 状态改 `Proposed`；明确与 ADR-0005/0007 的修订关系。
2. 新增 Phase 0：基线冻结 + 证据存档 + CI 修复 + R3 未闭合项收口。
3. 统一依赖白名单（含 zlog/cJSON）并给出 CI 机械门禁（`nm -u`、尺寸、`rg` 路径检查）。
4. 自注册宏补链接机制、注册顺序、重复检测、`nm` DoD。
5. 补 DDD/契约治理任务（界限上下文、ACL、TagID 分域、编译器复用）。
6. 明确二开产品是“参考产品留树内”还是“逐步迁出”。
7. 修正无证据断言（God File）与测试基线说明（134/134 需在冻结点复跑）。

### Should（执行前补）
8. 每阶段拆 Ticket + 时间估算 + 三 agent 工作流。
9. Out-of-tree 的 ABI/依赖/许可/CI 独立构建验证。
10. 安全与 Zero-Mock backlog 显式入表（或声明另行跟踪）。

### Nice（后续）
11. edge-cli 接入 compiler-v2 validator；生成模板纳入 DX 测试。
12. README/手册/SDK 分层文档随阶段同步。

## 8. 建议修订后的执行顺序

```
Phase 0  基线冻结与止血（2-4 天）
Phase 1  双重源码树清理
Phase 2  核心解耦 + edge_infra（2a 接口化 → 2b 迁移 → 2c 清理）
Phase 3  自注册 + edge-cli + DX 测试
Phase 3.5 领域建模与契约治理（可与 3 并行）
Phase 4  Out-of-Tree SDK 导出与治理
```

每个 Phase 出口 = 基线 tag + `./build.sh` + `./test.sh`（134）+ `./test.sh --omnis`（10）+ `check_tag_consistency.sh` + `check_repo_clean.sh` + ASan 全绿。

## 9. 需要 Mr. WKT 拍板的问题

1. 是否批准“Phase 0 冻结”先行？未提交的 DCU daemon/fusion 代码如何处理（先提交、先回退、还是冻结不动）？
2. 二开产品最终是否全部 Out-of-Tree？现有 `cmd/*` 是参考产品还是迁出对象？
3. 安全/Zero-Mock 既有 backlog 是否纳入本计划 Phase 0，还是单独排期？
4. 时间与人力预算（建议至少 6-8 周，3 个 agent 并行）？
5. 是否接受链接脚本/`__attribute__((constructor))` 机制进入工具链约束（影响交叉编译与 Zephyr）？

## 10. 结论

ADR-0008 与反熵主计划**方向正确、架构主张与仓库现状高度匹配，具备批准价值**；但当前版本更像“愿景白皮书 + 粗粒度路线图”，缺少 Phase 0、机械门禁、契约/DDD/安全衔接和票粒度。**建议：批准架构方向，退回修订计划；修订后先执行 Phase 0 + Phase 1，再逐阶段验收。**

---

*本评审由 Codex 基于 2026-08-15/16 仓库快照完成；基线数字（134/134、10/10）需在 Phase 0 冻结后复跑存档。*
