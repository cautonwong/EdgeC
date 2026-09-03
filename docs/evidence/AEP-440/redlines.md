# AEP-440 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **需量控制削峰算法** | 滑动窗口滚动负荷预测与储能自适应出清 | ✅ PASS | 实现 15 分钟滑窗均值与需量红线动态放电削峰 |
| **2充2放分时电价 (TOU) 排程** | 谷/平时吸收储能与午间光伏，尖/峰时放电套利 | ✅ PASS | 实现 `iems_coordinator_setup_default_tou` 与 SOC 上下限约束 |
| **<100ms 快速防逆流闭环** | 毫秒级 PID 闭环快速调节 PCS 充放电功率与光伏限额 | ✅ PASS | 实现快速 PID 调节器（抗余电倒送与光伏逆变器快速降额） |
| **物理真实性 (Physical Truth)** | 无 Mock 占位符、真实负荷与变压器容量计算 | ✅ PASS | 基于真实变压器容量、需量红线与关口电表功率闭环 |
| **手术级修改 (Surgical Edit)** | 仅改动 IEMS 协调器与单测配置 | ✅ PASS | 精准更新 `cmd/iems/iems_microgrid_coordinator.c/.h` |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 4/4 产品二进制严控尺寸预算通过 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-440, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-440`，全量 `tests.log` 实测入库 |
