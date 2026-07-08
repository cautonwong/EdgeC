# ADR-0002: 语义契约编译器云端化演进

**状态**：已批准 (Approved)  
**日期**：2026-07-06  
**决策者**：EdgeC 架构团队  
**Grilling 决议**：2026-07-06 通过 4 项设计决策（见"设计决策"章节）

## 前提依赖

本 ADR 的全部 Phase 以 **Consumer-Driven Refactor** 完成为前提。`arbiter` → `io_executor`、`northbound_mgr` → `datahub`、`policy_syncer` → `mqtt_client` 三个解耦点必须在 Phase 1 启动前完成，因为热加载需要在每个解耦边界做原子切换。

## 背景

当前 EdgeC 的语义契约（Semantic Contract）编译流程是**本地编译时**模式：

```
编辑 YAML → Go 编译器 → 生成 C 头文件 → 重新编译 → 重启进程
```

这个流程有一个根本矛盾：EdgeC 宣称自己是"软件定义设备"（换一个 YAML 就换一种设备行为），但实际上换 YAML 需要完整的工具链环境和进程重启。

目前已经登记的需求冲突：

1. **热加载需求**：IEMS 现场工程师希望远程调整仪表轮询参数，不必去现场重启设备
2. **无工具链环境**：Zephyr RTOS 部署场景（Aero-Edge）没有 Go 运行时可用来编译 Contract
3. **批量部署**：Edge-Ops 预设了云端集中式契约管理，与当前的本地编译模式不一致
4. **单源点表**：C 和 Go 各持一份标签元数据，人工同步已出现 drift 风险

## 选项

### 选项 A：本地编译 → 运行时加载（两步走）

**Phase 1 — 当前本地编译器加输出格式扩展**
保持 Go 编译器在本地运行，但输出改为"二进制契约 Blob"（一个包含所有映射表的序列化结构体），而非 C 头文件。

**Phase 2 — 云端编译器 + 节点下载**
将 Go 编译器部署为 Edge-Ops 云服务，EdgeC 节点在启动时从 Edge-Ops 拉取最新的契约 Blob。Edge-Ops 提供 REST API：`POST /api/v1/contracts/compile` 和 `GET /api/v1/contracts/{id}/blob`。

**Phase 3 — 运行时热加载**
契约 Blob 支持 `bus_orchestrator_apply_contract(blob)` 接口，在不停机的情况下替换运行中的协议映射和 Pipeline 拓扑。

### 选项 B：直接上全量运行时编译

EdgeC C 引擎内嵌 YAML 解析器 + 一个轻量级编译运行时，直接在边端编译 Contract。

**优点：**
- 完全不依赖外部编译器
- 真正的一步到位

**缺点：**
- 需要在 C 引擎中内嵌完整的 YAML 解析和代码生成能力，这是约 2-3 万行 C 代码的工作量
- YAML 库在 MCU（Cortex-M4）上内存消耗过大
- 契约编译有复杂的错误报告需求，C 层表达能力不足
- 已经有一个功能完整的 Go 编译器，不应该废弃

### 选项 C：保持现状（本地编译时生成 C 头文件）

**优点：**
- 零改动
- 编译时优化机会最多（C 编译器可以死代码消除）

**缺点：**
- 无法满足热加载需求
- Zephyr 场景无法运行 Go 编译器
- 与 Edge-Ops "云端推送契约"的愿景矛盾
- 点表双源问题永久无法解决

## 决策

选择 **选项 A：本地编译 → 运行时加载（三步演进）**。

## 设计决策（Grilling 2026-07-06）

| # | 问题 | 决策 |
|---|------|------|
| G1 | 序列化格式 | protobuf（schema 演化能力强，云边一致，MCU 可预解码到静态 struct） |
| G2 | 运行时契约承载结构 | 新建 `contract_instance_t`，与 `contract_registry_t` 分离，支持新旧版本共存 |
| G3 | 热加载切换策略 | 全量 Pipeline 原子切换（双缓冲 + RCU 指针交换），拒绝逐 Stage 替换 |
| G4 | 离线缓存策略 | 本地缓存 + 版本比对，启动时加载本地、异步拉取云端、云端更新时原子切换 |
| G5 | 安全校验 | 签名链：Edge-Ops 私钥签名 + 节点公钥验证，复用 License 引擎基础设施 |

**理由：**

1. **渐进风险低**：Phase 1 只需要改动 Go 编译器输出端和 C 加载端，不破坏现有流程
2. **向后兼容**：在 Phase 1 和 Phase 2 之间，现有编译流程可以保持工作
3. **MCU 可行**：最终结果是在节点上加载序列化二进制，内存开销可控
4. **投资保护**：Go 编译器的所有业务逻辑（协议映射、Pipeline 拓扑生成）不需要重写

## 实施计划

### Phase 1：二进制契约 Blob（Sprint 4）

1. 定义 `contract_blob_t` 的序列化格式（protobuf 或自定义紧凑结构）
2. 修改 Go 编译器输出端：`--output-format=blob` 生成 `.edgec` 文件
3. C 端实现 `contract_blob_load(path)` → 填充 `contract_registry_t`
4. 现有的 `.h` 文件生成保留（用于编译时优化路径）

### Phase 2：云端编译器（Sprint 5-6）

1. Edge-Ops 部署编译器服务
2. EdgeC 节点增加 `--contract-url` 启动参数
3. Contract Registry 增加 "fallback to local blob" 逻辑（离线时使用本地缓存）

### Phase 3：运行时热加载（Sprint 7）

1. `bus_orchestrator_apply_contract(blob)` 实现原子切换
2. 双缓冲 Pipeline 拓扑（RCU 风格指针切换）
3. WASM App 在契约切换时的生命周期管理（存活/重建）

## 影响

- **正面**：解除"换业务需重启"的架构约束，EdgeC 成为真正可软件重定义的设备
- **正面**：Zephyr 部署场景可以预装 Blob 在文件系统中，启动时加载
- **正面**：点表单源问题自然解决（编译器是唯一数据源）
- **负面**：Phase 1 增加二进制 Blob 格式维护成本
- **负面**：Phase 3 的热加载接口需要严格的测试（状态迁移的一致性问题）
- **风险**：Phase 2 引入对 Edge-Ops 服务的网络依赖——离线场景需要本地 Blob 缓存

## 参考文献

- `CONSUMER_DRIVEN_REFACTOR_PLAN.md` — 接口解耦是热加载的前提条件
- `EDGE_OPS_PRD.md` — `cloud_contract_compiler` 模块定义
- `contract_registry.h` — 当前契约注册中心接口
