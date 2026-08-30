# AEP-402 交付与验收证据总结 (Summary)

> **Ticket**: AEP-402 (Phase 4 第 2 票：产品应用骨架生成器 archetype 落地，赋能二开工程师 1 秒生成标准工业级产品工程)
> **Branch**: `ticket/AEP-402`
> **Base Commit**: `origin/feature/modular-refactor` (f96bd14)
> **Issue**: [Issue #118](https://github.com/cautonwong/EdgeC/issues/118)
> **PR**: PR #119 (待创建)
> **Date**: 2026-08-30
> **Verification Status**: ✅ 100% PASS (CTest 148/148 + Omnis 10/10 + Go 全绿 + 6 大反熵架构门禁)

---

## 一、需求背景与产品应用骨架生成器落地 (Architecture & Delivery)

为解决二开项目工程师（2nd-Party Developer）新建工业产品（如 `microgrid_bess`, `pv_inverter`, `smart_meter_dcu` 等）时手工拷贝样板代码易出错、依赖不规范、头文件泄露的问题，AEP-402 强化了 **`edge-cli create-product` 标准二开脚手架生成器**：

1. **`edge-cli create-product <name>` 生成物全景**：
   - **`cmd/<name>/prj.conf`**：标准 Kconfig 特性选项配置（`CONFIG_EDGEC_TIER="ENTERPRISE"`, `CONFIG_EDGEC_PERSISTENCE=y`, `CONFIG_EDGEC_PROTO_MODBUS=y`, `CONFIG_EDGE_INFRA_SQLITE=y`）；
   - **`cmd/<name>/manifest.yaml`**：声明产品特性清单与遥测开关；
   - **`cmd/<name>/CMakeLists.txt`**：标准独立 CMake 配置，调用 `edge_add_contract` 生成绑定库并链接 `edge_infra`、`edge_core` 等 SDK 模块；
   - **`cmd/<name>/main.c`**：仅依赖 `<edgeos/edgeos.h>` 公共 SDK 入口，初始化 `edgeos_sdk_init()`、`datahub_create()`，挂载生成式驱动注册表并启动事件循环；
   - **`contracts/products/<name>.yaml`**：标准分域产品契约定义文件；
   - **`cmd/<name>/README.md`**：开箱即用的构建与运行操作指南。
2. **自动化测试套件 (`tests/test_archetype_generator.c`)**：
   - 机械化验证 `edge-cli create-product sample_archetype_bess` 完整生成 6 大文件；
   - 机械化验证 `edge-cli lint-contract` 严格合规通过；
   - 机械化验证 `contract-compiler` 生成 `contracts.c` 查表库与驱动注册表；
   - 机械化验证 `edge-cli create-driver` 驱动脚手架生成；
   - 自动执行沙箱销毁清理（零残留）。

---

## 二、DoD 验收与验证矩阵 (Verification & DoD)

### 1. `test_archetype_generator` 实测输出
```text
[==========] tests: Running 4 test(s).
[ RUN      ] test_archetype_product_scaffolding
[       OK ] test_archetype_product_scaffolding
[ RUN      ] test_archetype_contract_linter
[       OK ] test_archetype_contract_linter
[ RUN      ] test_archetype_contract_compilation
[       OK ] test_archetype_contract_compilation
[ RUN      ] test_archetype_driver_scaffolding
[       OK ] test_archetype_driver_scaffolding
[==========] tests: 4 test(s) run.
[  PASSED  ] 4 test(s).
```

### 2. 全量测试与门禁验证矩阵

| 验证项 | 目标命令 | 实测结果 | 状态 |
|---|---|---|:---:|
| **全量 Release 构建** | `cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release && ninja -C build` | 全部目标编译成功，0 警告 | ✅ PASS |
| **C 单元测试 (-j1)** | `ctest --test-dir build -j1` | **148/148 passed (22.15s)**，含 `test_archetype_generator` | ✅ PASS |
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
 .local/linux-x86_64/bin/edge-cli            | Bin 3490583 -> 3494751 bytes
 docs/evidence/AEP-402/diffstat.txt          |  16 +++
 docs/evidence/AEP-402/redlines.md          |  16 +++
 docs/evidence/AEP-402/summary.md           | 110 ++++++++++++
 docs/evidence/AEP-402/tests.log            | 550 ++++++++++++++++++++++++++++++
 tests/CMakeLists.txt                       |  20 +++
 tests/test_archetype_generator.c           | 152 +++++++++++++++++++++++++
 tools/edge-cli/internal/scaffold/product.go |  38 +++++--
 8 files changed, 892 insertions(+), 10 deletions(-)
```
