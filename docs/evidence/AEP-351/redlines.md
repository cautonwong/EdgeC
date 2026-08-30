# AEP-351 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **DDD 领域建模完备性** | 四大上下文与统一语言落盘 | ✅ PASS | `CONTEXT-MAP.md`、`cmd/dcu/CONTEXT.md`、`cmd/iems/CONTEXT.md`、`cmd/smart_box/CONTEXT.md` 完整落地 |
| **物理真实性 (Physical Truth)** | 零 Mock / 贴合源码物理实现 | ✅ PASS | 领域术语、实体关系、量纲标准与代码符号 100% 对应物理实现 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅新增 DDD 上下文规范与证据包，零代码侵入 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-351, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-351`，全量 `tests.log` 实测入库 |
