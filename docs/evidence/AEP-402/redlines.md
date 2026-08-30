# AEP-402 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **产品应用骨架生成** | `edge-cli create-product` 1 秒生成标准二开工程 | ✅ PASS | 自动化生成 `prj.conf`, `manifest.yaml`, `CMakeLists.txt`, `<edgeos/edgeos.h>` 的 `main.c`, `contracts/products/*.yaml` |
| **二开体验闭环验证** | 脚手架生成物能够直接通过 Contract Compiler 编译并被 Linter 校验 | ✅ PASS | 编写 `tests/test_archetype_generator.c` 验证完整生命周期（4/4 测试通过） |
| **物理真实性 (Physical Truth)** | 无 Mock、真实 DataHub / 驱动挂载闭环 | ✅ PASS | 物理运行于 `datahub_create`、`edge_driver_registry_init` 与事件循环 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅增强 `internal/scaffold/product.go`、重新编译 CLI 二进制与新增 C 单测 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-402, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-402`，全量 `tests.log` 实测入库 |
