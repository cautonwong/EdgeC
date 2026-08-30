# AEP-303 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **Clean Architecture 二开脚手架** | 零手工 extern 样板代码生成 | ✅ PASS | `create-product` 生成的 `main.c` 100% 接入 `edge_driver_registry_init(drv_mgr)` |
| **驱动接口规范** | 符合 edge_driver_t 规约 | ✅ PASS | `create-driver` 生成标准 `ops_t`（init/open/close/read/write/ioctl）与 binding YAML |
| **契约静态审计 (Linter)** | 校验 Tag 冲突与数据类型 | ✅ PASS | `lint-contract` 能够准确识别非法类型与重复 TagID 并输出友好诊断 |
| **物理真实性 (Physical Truth)** | 零 Mock / 零假数据 | ✅ PASS | 生成真实可编译的工程与驱动模板，零占位假数据 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅新增 `tools/edge-cli` 工具链、单测脚本与 CTest 注册 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-303, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-303`，全量 `tests.log` 实测入库 |
