# AEP-304 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **DX 二开契约全链路闭环** | 新增驱动仅需声明与注册 | ✅ PASS | `test_dx_scaffolding.c` 验证 2nd-party 驱动定义、注册表注入、生命周期管理与 I/O 调度 |
| **物理真实性 (Physical Truth)** | 零 Mock / 零假数据 | ✅ PASS | 真实 `NetBufPool` 物理帧编解码、真实 `DataHub` 影子总线读写与真实浮点/整数比对 |
| **内存与生命周期安全** | 零内存泄漏与野指针 | ✅ PASS | `NetBufPool` 引用计数追踪闭环（Refs 归零销毁）、`driver_instance_destroy` 与 `datahub_destroy` 优雅退出 |
| **防御性编程** | 异常与边界全覆盖 | ✅ PASS | NULL 指针防御、非法协议查询防御、损坏注册表拦截均通过测试 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅新增 `tests/test_dx_scaffolding.c`、证据包与 CTest 注册 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-304, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-304`，全量 `tests.log` 实测入库 |
