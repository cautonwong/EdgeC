# AEP-403 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **二开自定义驱动范例** | `examples/custom_driver/` 独立可编译与契约映射 | ✅ PASS | 包含驱动源码、头文件、Modbus 绑定 YAML、CMake 构建脚本与开发指南 |
| **二开自定义算法范例** | `examples/custom_algo/` 独立可编译与 DataHub 读写 | ✅ PASS | 包含 EMA 滤波算法源码、头文件、CMake 构建脚本与开发指南 |
| **二开范例集成测试** | 机械化验证驱动生命周期与算法滤波闭环 | ✅ PASS | 编写 `tests/test_examples_integration.c` 验证完整管线（3/3 测试通过） |
| **物理真实性 (Physical Truth)** | 无 Mock、真实 DataHub / 驱动挂载闭环 | ✅ PASS | 物理运行于 `datahub_init`、`custom_inverter_simulate_poll`、`custom_soc_filter_process_datahub` |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅新增 `examples/`、`tests/test_examples_integration.c` 并修正 SDK driver 头文件引用 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-403, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-403`，全量 `tests.log` 实测入库 |
