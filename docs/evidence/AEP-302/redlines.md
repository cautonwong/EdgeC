# AEP-302 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **消灭手工 extern** | 各产品 main.c 零手工 extern 驱动注册 | ✅ PASS | `iems`, `dcu`, `evse`, `hems`, `sg` 全部移除手工 extern，接入 `edge_driver_registry_init` |
| **物理真实性 (Physical Truth)** | 零 Mock / 零假数据 | ✅ PASS | 真实的 VTable 数组，真实 `driver_manager_register_table` 注册与动态实例化 |
| **禁止手改生成物 (Generated Artifacts)** | 生成物由工具链产生 | ✅ PASS | `edge_driver_registry.h` 与 `edge_driver_registry.c` 100% 由 `contract-compiler-v2` 自动生成 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅迁移产品 main.c 注册入口、解耦头文件、加固错误处理与参数化单测 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **独立分支与证据包** | ticket/AEP-302, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-302`，全量 `tests.log` 实测入库 |
| **结转 Notes 闭环** | AEP-301 遗留 5 条 Notes | ✅ PASS | ① diffstat=904 固化；② `edge_driver_registry.h` 独立头分层；③ `register_table` 错误日志与严格失败返回；④ 编译器重建可复现；⑤ DCU/SmartBox 参数化注册表单测全覆盖 |
