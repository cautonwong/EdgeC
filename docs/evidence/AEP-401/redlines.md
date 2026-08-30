# AEP-401 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **SDK 统一入口收敛** | 建立 `include/edgeos/` 公共头文件体系 | ✅ PASS | 创建 `edgeos.h`, `version.h`, `types.h`, `datahub.h`, `driver.h`, `anti_corruption.h` |
| **二开体验端到端验证** | 仅依赖 `<edgeos/edgeos.h>` 即可编译运行 | ✅ PASS | 编写 `tests/test_sdk_public_headers.c` 模拟二开全功能调用（4/4 测试通过） |
| **物理真实性 (Physical Truth)** | 无 Mock、真实 DataHub / ACL 链路 | ✅ PASS | 物理运行于 `datahub_init`、`edge_anti_corruption` 与影子内存总线 |
| **手术级修改 (Surgical Edit)** | 不破坏现有内部头文件架构 | ✅ PASS | 100% 向后兼容现有代码，仅新增公共抽象头文件与测试 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-401, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-401`，全量 `tests.log` 实测入库 |
