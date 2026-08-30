# AEP-354 交付与验收证据总结 (Summary)

> **Ticket**: AEP-354 (Phase 3.5 第 4 票：核心业务场景用例固化，为 DCU 冻结抄读/停电上报与 IEMS 防逆流/峰谷套利固化 DDD 场景测试)
> **Branch**: `ticket/AEP-354`
> **Base Commit**: `origin/feature/modular-refactor` (7740a01)
> **Issue**: [Issue #112](https://github.com/cautonwong/EdgeC/issues/112)
> **PR**: PR #113 (待创建)
> **Date**: 2026-08-30
> **Verification Status**: ✅ 100% PASS (CTest 145/145 + Omnis 10/10 + Go 全绿 + 6 大反熵架构门禁 + test_domain_scenarios 4/4 全绿)

---

## 一、需求背景与 DDD 业务场景固化交付 (Architecture & Delivery)

为确保 EdgeOS 在经历模块化重构与防腐层解耦后，上层业务领域层（DCU 配电台区与 IEMS 储能微网）的核心业务场景具备 100% 确定性的端到端自动化测试支撑，AEP-354 交付了 **[tests/test_domain_scenarios.c](tests/test_domain_scenarios.c)** 业务场景测试套件：

1. **DCU 场景 1: 台区拓扑与 16 分支动态线损核算闭环 (`test_dcu_lineloss_topology_scenario`)**：
   - 模拟主变压器供入总功率 $P_{in} = 120.0\text{kW}$，16 路分支供出功率之和 $P_{out} = 115.8\text{kW}$；
   - 领域算法动态核算：$Loss = \frac{120.0 - 115.8}{120.0} \times 100\% = 3.5\%$；
   - 经由 ACL 防腐层转换并写入 DataHub 影子总线（Tag `65538` `DCU_LINELOSS_PCT`），验证影子读写与健康阈值判定。
2. **DCU 场景 2: 停电主动侦测与 0x3011 SOE 快照闭环 (`test_dcu_outage_soe_3011_scenario`)**：
   - 模拟电网三相失压（电压骤降至 0V，超级电容接管）；
   - 停电侦测引擎毫秒级捕获掉电瞬态，生成 DLT698 `0x3011` 事件快照与微秒时间戳；
   - 写入 DataHub 影子总线（Tag `65539` `DCU_OUTAGE_STATUS`），断言事件触发与状态码一致性。
3. **IEMS 场景 1: 关口防逆流（Zero Export）毫秒级功率下调闭环 (`test_iems_anti_reverse_flow_scenario`)**：
   - 模拟 PCC 关口电表检测到倒送电功率 $P_{export} = -40.0\text{kW}$（突破死区 $-5.0\text{kW}$）；
   - 能量管理聚合根 `ESSContainer` 计算下调目标功率：$P_{target} = 80.0 + (-40.0) = 40.0\text{kW}$；
   - 逆向 ACL 转换为 Modbus 寄存器原始字，更新 DataHub Tag `131078`（防逆流动作激活）与 Tag `131074`（新功率设定值 40.0 kW），断言控制收敛。
4. **IEMS 场景 2: 动态分时电价（TOU）峰谷套利与 BMS SOC 安全包络保护 (`test_iems_tou_arbitrage_and_soc_envelope_scenario`)**：
   - 模拟电价时钟切换至峰期（1.35 元/kWh），BMS 检测 SOC=45.0%（在 15%~95% 安全区间）；
   - 决策触发最大放电功率（150.0 kW）；
   - 模拟放电后电池 SOC 触及低电量保护下限（SOC $\le 15.0\%$），安全包络线强制触发停机归零闭锁；
   - 断言 DataHub 状态与安全保护闭环。

---

## 二、DoD 验收与验证矩阵 (Verification & DoD)

### 1. `test_domain_scenarios` CMocka 实测输出
```text
[==========] tests: Running 4 test(s).
[ RUN      ] test_dcu_lineloss_topology_scenario
[2026-08-30 15:57:36][INFO][BUS] Sealed memfd Ring created (FD: 13, Size: 68 KB)
[       OK ] test_dcu_lineloss_topology_scenario
[ RUN      ] test_dcu_outage_soe_3011_scenario
[2026-08-30 15:57:36][INFO][BUS] Sealed memfd Ring created (FD: 13, Size: 68 KB)
[       OK ] test_dcu_outage_soe_3011_scenario
[ RUN      ] test_iems_anti_reverse_flow_scenario
[2026-08-30 15:57:36][INFO][BUS] Sealed memfd Ring created (FD: 13, Size: 68 KB)
[       OK ] test_iems_anti_reverse_flow_scenario
[ RUN      ] test_iems_tou_arbitrage_and_soc_envelope_scenario
[2026-08-30 15:57:36][INFO][BUS] Sealed memfd Ring created (FD: 13, Size: 68 KB)
[       OK ] test_iems_tou_arbitrage_and_soc_envelope_scenario
[==========] tests: 4 test(s) run.
[  PASSED  ] 4 test(s).
```

### 2. 全量测试与门禁验证矩阵

| 验证项 | 目标命令 | 实测结果 | 状态 |
|---|---|---|:---:|
| **全量 Release 构建** | `cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release && ninja -C build` | 全部目标编译成功，0 警告 | ✅ PASS |
| **C 单元测试 (-j1)** | `ctest --test-dir build -j1` | **145/145 passed (21.32s)**，含 `test_domain_scenarios` | ✅ PASS |
| **Matrix 全链路物理仿真** | `./test.sh --omnis` | **10/10 物理场景全通过 (51.88s)** | ✅ PASS |
| **Go 全量单元测试** | `cd go && go test -v ./...` | 全绿 | ✅ PASS |
| **架构门禁 1: 旧目录清零** | `./scripts/check_no_old_src.sh` | 0 legacy src 引用 | ✅ PASS |
| **架构门禁 2: 符号基线** | `./scripts/check_symbols_baseline.sh` | 7/7 核心符号验证通过 | ✅ PASS |
| **架构门禁 3: 严格符号** | `STRICT_CORE_GATE=1 ./scripts/check_core_symbols.sh` | `libedge_core.a` 0 禁止第三方符号 | ✅ PASS |
| **架构门禁 4: 核心只读** | `STRICT_CORE_GATE=1 ./scripts/check_core_readonly.sh origin/feature/modular-refactor` | 核心受保护文件 0 违规修改 | ✅ PASS |
| **架构门禁 5: 严格尺寸** | `STRICT_SIZE_GATE=1 ./scripts/check_binary_sizes.sh` | 4/4 产物严控预算达标 (core: 118.7KB) | ✅ PASS |
| **架构门禁 6: 生成物可信链** | `STRICT_DRIFT_GATE=1 ./scripts/check_generated_drift.sh` | 编译器一致(delta:0B)，全生成物零漂移 | ✅ PASS |

---

## 三、真实 Diffstat 汇总

```text
 docs/evidence/AEP-354/diffstat.txt   |  16 +++
 docs/evidence/AEP-354/redlines.md   |  16 +++
 docs/evidence/AEP-354/summary.md    | 115 ++++++++++++
 docs/evidence/AEP-354/tests.log     | 550 ++++++++++++++++++++++++++++++
 tests/CMakeLists.txt                |  20 ++
 tests/test_domain_scenarios.c       | 235 ++++++++++++
 6 files changed, 952 insertions(+)
```
