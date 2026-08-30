# AEP-352 交付与验收证据总结 (Summary)

> **Ticket**: AEP-352 (Phase 3.5 第 2 票：TagID 兼容分域与别名映射，落地 legacy_aliases.yaml 与分域规则)
> **Branch**: `ticket/AEP-352`
> **Base Commit**: `origin/feature/modular-refactor` (5c2ed30)
> **Issue**: [Issue #107](https://github.com/cautonwong/EdgeC/issues/107)
> **PR**: PR #108 (待创建)
> **Date**: 2026-08-30
> **Verification Status**: ✅ 100% PASS (CTest 143/143 + Omnis 10/10 + Go 全绿 + 6 大反熵架构门禁 + test_tag_domains_and_aliases 3/3 全绿)

---

## 一、需求背景与分域架构交付 (Architecture & Delivery)

为支持 EdgeOS 多产品线扩展、防止 TagID 命名空间碰撞并实现对历史存量系统的零破坏平滑演进，AEP-352 落地了 **TagID 分域规则体系与别名映射机制**：

1. **清晰的 16-bit 领域命名空间划分**：
   - **`0x000000 ~ 0x00FFFF`（0 ~ 65535）**：`core_standard` 基础度量域（三相电压/电流/功率/标准 SOC/SOH）；
   - **`0x010000 ~ 0x01FFFF`（65536 ~ 131071）**：`dcu_distribution` 配电台区智能子域（变压器负载率、16 分支线损、停电 3011、16-DI/8-DO）；
   - **`0x020000 ~ 0x02FFFF`（131072 ~ 196607）**：`iems_microgrid` 工商业储能与微电网子域（PCS 四模式、BMS 单体电芯电压/温度极值、防逆流）；
   - **`0x030000 ~ 0x03FFFF`（196608 ~ 262143）**：`smart_box_panel` 智能配电箱子域（32 路微断、剩余漏电流、端子温度）；
2. **零破坏别名映射表 (`contracts/schema/legacy_aliases.yaml`)**：
   - 建立历史存量 Tag 标识符（如 `TAG_VOLTAGE_A`, `DCU_FEEDER_LOSS`, `BESS_MAX_CELL_V`）到标准 TagID 与规范名称的映射表；
3. **领域契约 Schema (`contracts/schema/domains/*.yaml`)**：
   - 分别落盘 `dcu_distribution.yaml`、`iems_microgrid.yaml`、`smart_box_panel.yaml`，并通过 `edge-cli lint-contract` 严格静态校验；
4. **自动化单元测试套件 (`tests/test_tag_domains_and_aliases.c`)**：
   - 机械化验证 TagID 分域前缀识别；
   - 机械化验证历史别名查找与解析；
   - 机械化验证 DataHub 跨领域 Tag 影子总线读写与真实浮点/整数精度断言。

---

## 二、DoD 验收与验证矩阵 (Verification & DoD)

### 1. `test_tag_domains_and_aliases` CMocka 实测输出
```text
[==========] tests: Running 3 test(s).
[ RUN      ] test_tag_domain_partitioning
[       OK ] test_tag_domain_partitioning
[ RUN      ] test_legacy_alias_resolution
[       OK ] test_legacy_alias_resolution
[ RUN      ] test_datahub_partitioned_tag_telemetry
[2026-08-30 11:35:11][INFO][BUS] Sealed memfd Ring created (FD: 13, Size: 68 KB)
 [Metrics] Registered: Tag=12001, Name=datahub.ring.watermark
 [Metrics] Registered: Tag=12002, Name=datahub.ring.drop_count
 [Metrics] Registered: Tag=12003, Name=datahub.ring.read_latency
 [Metrics] Registered: Tag=12004, Name=datahub.shadow.write_count
[       OK ] test_datahub_partitioned_tag_telemetry
[==========] tests: 3 test(s) run.
[  PASSED  ] 3 test(s).
```

### 2. 全量测试与门禁验证矩阵

| 验证项 | 目标命令 | 实测结果 | 状态 |
|---|---|---|:---:|
| **全量 Release 构建** | `cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release && ninja -C build` | 全部目标编译成功，0 警告 | ✅ PASS |
| **C 单元测试 (-j1)** | `ctest --test-dir build -j1` | **143/143 passed (20.45s)**，含 `test_tag_domains_and_aliases` | ✅ PASS |
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
 contracts/schema/domains/dcu_distribution.yaml |  32 +++
 contracts/schema/domains/iems_microgrid.yaml   |  36 +++
 contracts/schema/domains/smart_box_panel.yaml  |  22 ++
 contracts/schema/legacy_aliases.yaml           |  62 +++++
 docs/evidence/AEP-352/diffstat.txt             |  17 ++
 docs/evidence/AEP-352/redlines.md             |  16 ++
 docs/evidence/AEP-352/summary.md              | 105 ++++++++
 docs/evidence/AEP-352/tests.log               | 525 +++++++++++++++++++++++++++++++
 tests/CMakeLists.txt                          |  20 ++
 tests/test_tag_domains_and_aliases.c          | 205 ++++++++++++
 10 files changed, 1040 insertions(+)
```
