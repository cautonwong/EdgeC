# AEP-353 交付与验收证据总结 (Summary)

> **Ticket**: AEP-353 (Phase 3.5 第 3 票：ACL 防腐层模块落地，在 modules/edge_infra/ 实现规约寄存器到领域实体的双向映射)
> **Branch**: `ticket/AEP-353`
> **Base Commit**: `origin/feature/modular-refactor` (85e1805)
> **Issue**: [Issue #109](https://github.com/cautonwong/EdgeC/issues/109)
> **PR**: PR #110 (待创建)
> **Date**: 2026-08-30
> **Verification Status**: ✅ 100% PASS (CTest 144/144 + Omnis 10/10 + Go 全绿 + 6 大反熵架构门禁 + test_acl_engine 4/4 全绿)

---

## 一、需求背景与 ACL 防腐架构交付 (Architecture & Delivery)

为彻底解决底层工业异构通信规约（Modbus 寄存器、DLT645 数据项、IEC104 信息体、CAN 信号）对上层领域业务模型（DCU 台区、IEMS 储能微网、SmartBox 配电箱）的侵入与腐化，AEP-353 在 `modules/edge_infra/` 中落地了 **Anti-Corruption Layer (ACL) 防腐层双向转换引擎**：

1. **统一规约到实体映射结构 (`edge_acl_entry_t`)**：
   - 关联业务 `tag_id`、所属 `domain_name` 与领域实体属性 `entity_prop`；
   - 关联南向 `proto`（Modbus/DLT645/IEC104/CAN）、原始寄存器/点位地址 `raw_address`；
   - 携带物理量换算系数：`scaler`（缩放因子）、`offset`（偏移量）与目标数据类型 `data_type`；
2. **高性能线程安全 ACL 引擎 (`edge_acl_engine_t`)**：
   - `edge_acl_init` / `edge_acl_destroy`：支持动态自动扩容点位表与 `pthread_mutex_t` 线程安全访问；
   - `edge_acl_register` / `edge_acl_register_table`：批量注入点位映射规则；
   - `edge_acl_lookup_by_tag` / `edge_acl_lookup_by_address`：支持按领域 TagID 或按通信规约点位高效索引；
3. **Inbound / Outbound 双向转换闭环**：
   - `edge_acl_translate_inbound`：南向采集原始寄存器数值 $\rightarrow$ 经过 $(raw \times scaler) + offset$ 转换并自动构造 64B `PipeMsg` 遥测消息；
   - `edge_acl_translate_outbound`：上层领域下发控制物理值 $\rightarrow$ 经过 $(phys - offset) / scaler$ 逆向还原为南向寄存器下发原始物理字；
4. **自动化单元测试套件 (`tests/test_acl_engine.c`)**：
   - 机械化验证 ACL 动态扩容与多协议注册；
   - 机械化验证 Modbus、DLT645、DCU 线损、IEMS PCS 功率、SmartBox 漏电流 Inbound 转换；
   - 机械化验证 IEMS 功率设定与 DCU DO 跳闸指令 Outbound 逆向转换；
   - 机械化验证领域层 0 规约寄存器魔数隔离性。

---

## 二、DoD 验收与验证矩阵 (Verification & DoD)

### 1. `test_acl_engine` CMocka 实测输出
```text
[==========] tests: Running 4 test(s).
[ RUN      ] test_acl_init_and_registration
[       OK ] test_acl_init_and_registration
[ RUN      ] test_acl_inbound_translation
[       OK ] test_acl_inbound_translation
[ RUN      ] test_acl_outbound_translation
[       OK ] test_acl_outbound_translation
[ RUN      ] test_acl_domain_entity_isolation
[       OK ] test_acl_domain_entity_isolation
[==========] tests: 4 test(s) run.
[  PASSED  ] 4 test(s).
```

### 2. 全量测试与门禁验证矩阵

| 验证项 | 目标命令 | 实测结果 | 状态 |
|---|---|---|:---:|
| **全量 Release 构建** | `cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release && ninja -C build` | 全部目标编译成功，0 警告 | ✅ PASS |
| **C 单元测试 (-j1)** | `ctest --test-dir build -j1` | **144/144 passed (20.55s)**，含 `test_acl_engine` | ✅ PASS |
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
 docs/evidence/AEP-353/diffstat.txt   |  17 ++
 docs/evidence/AEP-353/redlines.md   |  16 ++
 docs/evidence/AEP-353/summary.md    | 105 ++++++++
 docs/evidence/AEP-353/tests.log     | 530 +++++++++++++++++++++++++++++++
 modules/edge_infra/CMakeLists.txt   |   5 +-
 modules/edge_infra/include/edge_acl.h |  75 +++++
 modules/edge_infra/src/edge_acl.c   | 136 ++++++++
 tests/CMakeLists.txt                |  20 ++
 tests/test_acl_engine.c             | 205 ++++++++++++
 9 files changed, 1108 insertions(+), 1 deletion(-)
```
