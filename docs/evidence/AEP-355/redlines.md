# AEP-355 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **全局契约防腐查重** | 跨产品 Tag ID 与分域合规校验 | ✅ PASS | 编写 `tools/check_contract_collisions.py` 与 `tests/test_tag_collision_checker.c`，扫描 17 份契约 120 个 Tag，0 冲突 |
| **物理真实性 (Physical Truth)** | 消除全域历史冲突点位 | ✅ PASS | 清理 `smartbox.yaml` 与 `industrial_foc`/`automotive_uds`/`smartbox_radar` 历史 ID 碰撞（12051..12061），重命名 `SMARTBOX_BATTERY_SOC`（12013） |
| **并发与内存安全** | 线程安全与零内存泄漏 | ✅ PASS | C 单元测试无内存泄漏，Python 工具安全静态 AST/YAML 解析 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅新增 `check_contract_collisions.py`、`test_tag_collision_checker.c`、修复冲突 schema 与归档证据包 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-355, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-355`，全量 `tests.log` 实测入库 |
