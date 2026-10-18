# ADR 0010: 废除全局契约编译器与数字 TagID，推行产品聚合根自治

## 状态
已通过 (Accepted)

## 上下文
在早期版本（ADR-0002 / AEP-301）中，系统设计了中心化的全局契约体系：在 `contracts/` 下维护 YAML，并通过 Go 编写的 `contract-compiler` 为所有产品编译生成全局 `contracts.h`、`contracts.c` 和连续数字 `TagID`。

然而随着系统推进至微内核与 DDD 架构治理：
1. **违背消费者定接口与有界上下文**：不同产品（DCU、IEMS、SmartBox）的量纲、寻址、业务规则完全不同，却被强行绑在同一个全局 YAML 空间中；
2. **严重的双重运行时翻译开销**：电力集中器（DCU）原生基于 DL/T 698 OAD（如 `0x20000200`），南向驱动被强行翻译成全局 `TagID 1001`，DCU 业务层又不得不写大量 `if-else` 将 `1001` 翻译回 `0x20000200`；
3. **编译构建重型化**：C 基础工程编译被强行挂载了 Go 语言工具链依赖。

代码事实审计表明：
- `modules/` 目录下没有任何核心基础模块 `#include "contracts.h"`；
- DataHub 只认标量数字与应用注入的回调函数 `map_fn`；
- 南向协议（`edge_protocol`）零依赖；南向驱动（`edge_driver`）纯依赖注入；
- 外部时序库与 Web 前端依赖的是北向传输协议（MQTT/JSON 字段名），而非 C 语言头文件内部的数字 ID。

## 决策
1. **废除中心化契约体系**：逐步停用 `contracts/` 目录与 `contract-compiler`，删除全局 `contracts.h`。
2. **推行产品聚合根自治**：
   - 每个产品在 `cmd/<product>/` 维护专属聚合根与组合根；
   - 内部寻址与测点模型由产品领域天然属性自决（如 DCU 原生使用 698 OAD 与 645 DI，IEMS 使用工业寄存器表）；
   - 驱动点表由产品防腐层（ACL）提供依赖注入。
3. **DataHub 角色归位**：
   - DataHub 降级为各产品的 Outbound 通道（提供 SHM Ring、SeqLock 高频缓存与 DDS 跨节点发布），不再充当全局测点字典。
4. **单点概念验证（PoC）**：
   - 率先在 `cmd/dcu` 落地验证，断开 `edge_add_contract` 依赖，重构寻址映射并验证全部测试绿灯。

## 详细架构设计
详细技术解剖、代码审计证据链与演进方案见：
[docs/design/no-tagid-no-contract-architecture.md](../design/no-tagid-no-contract-architecture.md)

## 后果
- **正面影响**：
  - 各产品彻底自治，编译完全解耦，构建速度大幅提升；
  - 彻底消除 OAD 到 TagID 的冗余哈希查表与二次翻译，执行效率大幅提升；
  - 架构清晰度与内聚度显著提高，完全符合 DDD 与依赖倒置原则。
- **负面影响/风险**：
  - 各产品需要自行维护本地的驱动适配点表（通过极简静态结构体，代码量极小）。
