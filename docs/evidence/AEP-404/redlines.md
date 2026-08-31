# AEP-404 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **编译器 DX 诊断与纠错** | 智能拼写纠错 (Levenshtein) 与分域提示 | ✅ PASS | 支持 `'flot' -> 'float'`、`'Volts' -> 'V'` 提示与 Power/Storage 分域建议 |
| **契约加载器精准覆盖** | 支持按 Name 覆盖或按 ID 补充属性 | ✅ PASS | 修复 `loader.go` 误合并不同 Name 同 ID 导致的漏报 bug |
| **自动化诊断测试** | 机械化验证编译器异常报错与建议 | ✅ PASS | 编写 `tests/test_compiler_dx_diagnostics.c`（4/4 测试通过）与 `validator_test.go` |
| **物理真实性 (Physical Truth)** | 无 Mock、真实编译器二进制与契约解析 | ✅ PASS | 物理运行于 `.local/linux-x86_64/bin/contract-compiler` 真实编译流水线 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅修改 `validator.go`, `loader.go`, `tests/test_compiler_dx_diagnostics.c` |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 源码重构编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-404, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-404`，全量 `tests.log` 实测入库 |
