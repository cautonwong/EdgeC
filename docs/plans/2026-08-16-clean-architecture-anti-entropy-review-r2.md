# Codex 二审：反熵主计划与 ADR-0008（Second Review / Independent Verification）

> **Mr. WKT** — 二审对一审判定做独立复核，并补充了 6 组新实测证据。结论：**一审方向性意见全部成立；二审再补 4 个必须落地的机制性设计**（生成式注册表、接口先行、尺寸门禁口径、领域词汇治理）。

---

## 1. 新实测证据（二审新增）

| 项目 | 实测结果 | 对计划的影响 |
|---|---|---|
| `libedge_core.a` 当前尺寸 | **638,176 B ≈ 623 KB**（Debug 构建） | ADR-0008 的 150KB 不变量目前差 4 倍；必须定义口径（Release/stripped/MCU）并评估可行性 |
| SQLite 依赖面 | **35 个 C/H 文件**引用 `sqlite_service/sqlite_svc_t`（modules/cmd/src/tests） | Phase 2 迁移是“接口先行 + 分步迁移”，不是“搬文件” |
| 驱动注册现状 | `main.c` 手工 `extern ..._register()`：iems 4 个、dcu 2 个、evse/hems/sg 各 1 个 | “散弹式中央注册”属实，但形态是 extern 调用，不是 #ifdef；自注册价值明确 |
| `#ifdef CONFIG` 数量 | `cmd/*/main.c` 未发现成片产品分支 | 一审结论复核：**“上百个 #ifdef”依旧无证据**，应改写 |
| CI ASan/TSan | d7001ce 已新增 `x86_64_asan`、`x86_64_tsan` 矩阵 | T-107 部分闭环；但仍无 TSan 白名单/超时，且 `-DENABLE_COVERAGE=ON` 与 CMake `COVERAGE` 不匹配未改 |
| Go 版本 | `go.mod 1.24.0`，CI `go-version: 1.24` | 一审指出的版本错位已修复 ✅ |
| EdgeOSConfig / edge-cli | `cmake/EdgeOSConfig.cmake.in`、`tools/edge-cli` **均不存在** | 计划是前瞻性任务，符合预期；但 Phase 4/3 的“一键体验”尚无可复用资产 |

## 2. 对一审意见的复核结论

| 一审意见 | 二审复核 |
|---|---|
| 需要 Phase 0 基线冻结 | ✅ 成立，且更紧迫：工作区仍有未提交 DCU daemon/fusion 等新代码 |
| ADR-0008 状态应改 Proposed | ✅ 成立 |
| 与 ADR-0005/0007 的关系要写明 | ✅ 成立 |
| 依赖白名单统一 + 机械门禁 | ✅ 成立；二审补：`nm -u` + 尺寸门禁必须一起上 |
| 自注册机制要写死 | ✅ 成立；二审给出更优替代方案（见 §3） |
| DDD 任务缺失 | ✅ 成立；二审给出具体落地形态（见 §4） |
| 安全 backlog 不能消失 | ✅ 成立 |
| Out-of-tree 需 ABI/依赖/许可策略 | ✅ 成立 |
| 无证据断言（God File） | ✅ 复核确认无证据，仍建议改写 |

## 3. 二审最重要的机制建议：注册表走“生成式”，而不是链接器魔法

计划把 `EDGE_DRIVER_REGISTER` 描述成“在驱动 .c 里声明，编译期自动注册”。这可行，但在本仓库有三个现实约束：

1. 静态库链接时，未引用段会被丢弃，必须引入链接脚本/`--whole-archive`/constructor 之一；
2. 交叉编译（aarch64）与 Zephyr 的工具链行为不同，需要分别验证；
3. 注册顺序、重复注册、优先级、热插拔语义全部要重新定义。

**更优且与 CDD 天然一致的方案：由 contract-compiler-v2 生成注册表。**

```c
/* 生成物：build/products/<p>/generated/edge_driver_registry.c */
const edge_driver_reg_entry_t g_edge_driver_registry[] = {
    EDGE_DRIVER_ENTRY("ModbusRTU", modbus_rtu_driver_ops),
    EDGE_DRIVER_ENTRY("DLT698",   dlt698_driver_ops),
    /* 由 YAML 声明，编译器生成，零手写 */
};
```

优势：
- 不需要链接脚本/constructor，任何工具链一致；
- 顺序由 YAML 优先级字段决定，确定性强；
- 新增驱动 = 写驱动 + 改 YAML，不碰核心代码（满足 OCP）；
- `edge-cli create-driver` 生成的内容与编译器规则天然一致；
- 与 T-201/T-205（契约治理、Blob）共用一条工具链。

建议：Phase 3 的 DoD 改为“**通过 YAML 声明 + 生成式注册表新增驱动，且 `nm`/运行时断言验证注册表完整**”；链接器段宏作为可选加分项，不作为主方案。

## 4. DDD 落地形态（二审具体化）

按 domain-modeling 技能实践，计划应新增以下可执行任务：

1. **建立 `CONTEXT-MAP.md`**：DCU / IEMS / SmartBox / NexusLab 四个界限上下文 + 共享内核；
2. **每上下文一份 `CONTEXT.md`**：只放统一语言词汇（Tag 语义、领域实体、命令/事件），不放实现细节；
3. **领域 Tag 命名空间 = 聚合标识**：把 T-202 的 TagID 分域与 DDD 界限上下文绑定（如 `0x01xxxx` DCU、`0x02xxxx` IEMS、`0x03xxxx` SmartBox）；
4. **ACL 层任务**：新增 `modules/edge_infra/acl/`，负责“寄存器地址 → 领域实体”映射，禁止原始地址穿透业务层；
5. **场景用例**：每个上下文至少 2 个核心场景（如 DCU“表计冻结抄读”、IEMS“孤岛保护”），用于验证边界划分正确。

## 5. 尺寸门禁口径（二审新增）

`150KB` 不能停留在口号。建议：

- 定义测量口径：`Release + -ffunction-sections -fdata-sections + --gc-sections + strip`，目标 x86_64 与 Cortex-M 各一份；
- 分两档门禁：`WARN > 120KB`、`ERROR > 150KB`（可先按当前 623KB 定基线，逐步下降）；
- CI 增加尺寸快照 artifact，防止回潮；
- 若 150KB 在保留 shmring/net_buf/arbiter 后不现实，就把不变量改为“**edge_core 零网络/零数据库/零文件系统依赖**”的定性约束 + 150KB 作为长期目标，二者都写进 ADR。

## 6. CI 二审缺口（计划应引用）

- `-DENABLE_COVERAGE=ON` → 应改 `-DCOVERAGE=ON`（覆盖门禁至今未真正生效）；
- ASan/TSan job 需要：独立超时、失败 artifact、TSan 已知不兼容用例白名单；
- CI 未跑 `check_tag_consistency.sh` / `check_repo_clean.sh` / 尺寸门禁——计划应把这些列为“持续全绿”的组成部分；
- Omnis 只在 x86_64 普通 job 跑（合理），但需要固定 seed/随机性说明，保证可复现。

## 7. 阶段依赖与工期粗估（二审补充，非承诺）

```
Phase 0 基线冻结             2-4 人天
Phase 1 双重源码树清理       3-5 人天
Phase 2 接口化+迁移+清理      10-15 人天（35 文件依赖面）
Phase 3 生成式注册+edge-cli   5-10 人天
Phase 3.5 领域建模+Tag 分域    5 人天（可与 Phase 3 并行）
Phase 4 Out-of-Tree SDK       5-10 人天（不含许可/ABI 评审）
合计约 6-8 周（2-3 人）
```

依赖关系：Phase 3 依赖 Phase 2 的接口边界；Phase 3.5 的 Tag 分域依赖编译器规则（T-201/T-202）；Phase 4 依赖 Phase 3 + ABI 冻结。

## 8. 二审结论

1. **一审方向性意见全部复核成立**，没有发现误判；
2. 新增 4 个“必须做”的机制设计：**生成式驱动注册表、接口先行的 Phase 2、尺寸门禁口径、领域词汇与 CONTEXT-MAP 治理**；
3. 修正两处过时描述：Go 版本已对齐、ASan/TSan 已入 CI（但覆盖参数仍错）；
4. 建议审批动作：**批准 ADR-0008 概念 + 批准 Phase 0/1 修订后执行**；Phase 2-4 每阶段单独验收。

---

*二审独立验证完成：无新增权限依赖、无猜测性结论；所有新断言均可复现。*
