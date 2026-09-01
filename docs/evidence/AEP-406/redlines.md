# AEP-406 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **二次开发工程师手册** | 系统性指导产品创建、契约编写、驱动与算法接入 | ✅ PASS | 编制并发布 `docs/manuals/secondary_developer_guide.md` |
| **5分钟极简上手指南** | 5 分钟端到端从零创建微电网产品 | ✅ PASS | 编制并发布 `docs/manuals/quickstart_5min.md` |
| **物理一致性自动化测试** | 机械化验证手册中的所有命令、代码与 SDK 头文件 | ✅ PASS | 编写 `tests/test_developer_manuals.c`（3/3 测试通过，纳入 CTest 提升至 152 项） |
| **四平面隔离与 DIP** | 二开手册严格推行统一 SDK 公共头文件门面 | ✅ PASS | 详述 `<edgeos/edgeos.h>`、`<edgeos/driver.h>` 等标准引用准则 |
| **物理真实性 (Physical Truth)** | 无 Mock、真实脚手架命令与真实源码验证 | ✅ PASS | 测试用例实际调用 `.local/linux-x86_64/bin/edge-cli` 与 `contract-compiler` |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅新增手册、指南与测试，精准注册 `tests/CMakeLists.txt` |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-406, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-406`，全量 `tests.log` 实测入库 |
