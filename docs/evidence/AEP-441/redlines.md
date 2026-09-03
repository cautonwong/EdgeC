# AEP-441 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **三相不平衡换相决策算法** | 严格遵循 GB/T 15543 计算不平衡度并贪心求解负荷迁移组合 | ✅ PASS | 实现 `dcu_calc_three_phase_unbalance` 与 `dcu_solve_phase_switch_plan` |
| **2~31 次高精谐波分析** | 2~31 次奇偶次谐波含有率与总谐波畸变率 THD | ✅ PASS | 实现 `dcu_analyze_harmonics_thd` (基波有效值、奇次/偶次/总 THD) |
| **电压暂态事件捕获状态机** | 捕获暂降 (<0.9Un)、暂升 (>1.1Un) 与短时中断 (<0.1Un) | ✅ PASS | 实现 `dcu_detect_voltage_transient` 毫秒级状态机与极值记录 |
| **物理真实性 (Physical Truth)** | 无 Mock 占位符、真实三相物理量与谐波功率计算 | ✅ PASS | 基于真实电流电压采样、相序与台区表箱支路负荷 |
| **手术级修改 (Surgical Edit)** | 仅改动 DCU 电能质量模块与单测配置 | ✅ PASS | 新增 `cmd/dcu/dcu_power_quality.c/.h` 与针对性单测 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 4/4 产品二进制严控尺寸预算通过 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-441, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-441`，全量 `tests.log` 实测入库 |
