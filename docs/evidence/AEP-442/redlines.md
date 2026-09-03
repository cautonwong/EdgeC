# AEP-442 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **IEEE C57.91 变压器热寿命模型** | 绕组最热点温升与相对老化率 FAA 计算 | ✅ PASS | 实现 `smartbox_thermal_aging.c/.h` (基准 110℃ 对应 FAA=1.0) |
| **高压电气五防智能闭锁规则引擎** | 严防五大误操作（带负荷拉刀闸、带电挂地线、带地线合闸、误入带电间隔） | ✅ PASS | 实现 `smartbox_five_safety_interlock.c/.h` |
| **寿命损耗与 RUL 连续积分** | 实时计算累计寿命损耗 LOL (%) 与预估剩余寿命 | ✅ PASS | 连续评估周期累计与热过载报警 |
| **物理真实性 (Physical Truth)** | 无 Mock 占位符、真实物理公式与开关柜联锁状态 | ✅ PASS | 基于真实负荷率、顶层油温与开关柜五防物理状态机 |
| **手术级修改 (Surgical Edit)** | 仅改动 SmartBox 模块与单测配置 | ✅ PASS | 新增 `cmd/smart_box/smartbox_thermal_aging.c/.h` 等 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 4/4 产品二进制严控尺寸预算通过 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-442, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-442`，全量 `tests.log` 实测入库 |
