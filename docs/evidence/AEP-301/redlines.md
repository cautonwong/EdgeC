# AEP-301 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **物理真实性 (Physical Truth)** | 零 Mock / 零假数据 | ✅ PASS | 真实的 VTable 数组，真实 `driver_manager_register_table` 注册并实例化 |
| **禁止手改生成物 (Generated Artifacts)** | 生成物由工具链产生 | ✅ PASS | `edge_driver_registry.c` 100% 由 `contract-compiler-v2` 读取产品 YAML 自动生成 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅扩充 `c_backend.go`、`EdgeContract.cmake`、驱动注册接口与单测 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 阈值），三产品全绿 |
| **独立分支与证据包** | ticket/AEP-301, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-301`，全量 `tests.log` 实测入库 |
| **结转 Notes 闭环** | AEP-208 遗留 3 条 Notes | ✅ PASS | diffstat 965 修正；`purge_old_data` 字节预算/保留注释；回退桩语义注释固化 |
