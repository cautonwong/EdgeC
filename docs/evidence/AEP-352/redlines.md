# AEP-352 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **TagID 分域与别名映射** | 0x01xxxx/0x02xxxx/0x03xxxx 分域 | ✅ PASS | `contracts/schema/legacy_aliases.yaml` 与分域 Schema 完整落地 |
| **物理真实性 (Physical Truth)** | 零 Mock / 存量向后兼容 | ✅ PASS | 存量 TagID（1001/3001/11001）100% 保持原有物理语义与向后兼容 |
| **多域 DataHub 读写联动** | 跨分域影子总线正确性 | ✅ PASS | `test_tag_domains_and_aliases.c` 验证四大分域测点真实读写与精度比对 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅新增分域 schema、`legacy_aliases.yaml`、测试与证据包 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-352, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-352`，全量 `tests.log` 实测入库 |
