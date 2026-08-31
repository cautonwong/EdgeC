# AEP-405 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **跨版本迁移向导工具** | 支持 --dir, --dry-run, --fix | ✅ PASS | `edge-cli migrate` 完整支持扫描、诊断报告与就地安全改写 |
| **契约 YAML 自动升级** | `include:` -> `includes:` 列表化与别名归一化 | ✅ PASS | 自动化将旧单复数配置及非标单位（`Volts` -> `V`）升级为标准契约 |
| **C 源码 SDK 门面重构** | 自动改写为统一 `<edgeos/edgeos.h>` 公共头文件 | ✅ PASS | 消除旧式内部头文件直连，引导二次开发遵循 DIP 原则 |
| **自动化迁移集成测试** | 机械化验证扫描、自动修复与干净目录幂等性 | ✅ PASS | 编写 `tests/test_cli_migrate.c`（3/3 测试通过）与 Go 单元测试 |
| **物理真实性 (Physical Truth)** | 无 Mock、真实 CLI 二进制与磁盘文件重构 | ✅ PASS | 物理运行于 `.local/linux-x86_64/bin/edge-cli` 真实构建产物 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅修改 `tools/edge-cli/`, `tests/test_cli_migrate.c`, `tests/CMakeLists.txt` |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-405, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-405`，全量 `tests.log` 实测入库 |
