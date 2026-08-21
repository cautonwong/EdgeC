# AEP-208 交付与整改闭环验收证据总结 (Summary)

> **Ticket**: AEP-208 (Phase 2 最终收口工单：WAL/压实/SnF 专项韧性回归 + Omnis 10/10 + SHM 硬断言 + AEP-207 复审意见全闭环)
> **Branch**: `ticket/AEP-208`
> **Base Commit**: `origin/feature/modular-refactor`
> **PR**: [PR #94](https://github.com/cautonwong/EdgeC/pull/94)
> **Date**: 2026-08-21
> **Verification Status**: ✅ 100% PASS (CTest 136/136 + Omnis 10/10 + Go 全绿硬断言 + 5 大反熵架构门禁 + 全禁用整包构建 100% 通过)

---

## 一、AEP-208 / AEP-207 复审整改项逐项闭环说明 (Remediation Details)

### 1. SnF 去假化与真实出队实现 (Zero-Mock Genuine SnF Dequeue)
- **物理真实实现**：重写 `modules/edge_infra/storage/sqlite_storage_driver.c` 中的 `_sqlite_driver_snf_dequeue`：
  - 通过 SQL `SELECT id, timestamp, tag_id, value_f, quality, type FROM tag_history WHERE status = 0 ORDER BY timestamp ASC LIMIT 1;` 检索最旧未发送记录；
  - 真实回填 `PipeMsg`（`ts`, `tag_id`, `quality`, `type`, `payload.val.f`）；
  - 执行 `UPDATE tag_history SET status = 1 WHERE id = ?;` 真实推进状态；空队列返回 1；
  - 彻底杜绝“无作为返回 0”的假实现。

### 2. WAL / 压实 / 分区 GC / SnF 专项测试加深与物理数值硬断言
- **真实 WAL 体积测定**：`_sqlite_driver_get_wal_size` 通过 `stat()` 物理检查真实存在的 `${db_path}-wal` 文件大小，测试中严格断言 `wal_size > 0`；
- **容量感知旧数据清理**：`_sqlite_driver_purge_old_data` 按动态行数统计与时间序真实执行物理清理并配合 `VACUUM` 压实，测试严格断言清理后行数真实下降（`count_after < count_before`）；
- **全生命周期加深断言**：在 `tests/test_infra_interfaces.c` 中实施严密的全链路状态机断言：
  - `snf_get_pending_count == 0`
  - `snf_enqueue(&m2)`
  - `snf_get_pending_count == 1`
  - `snf_dequeue(&dequeued) == 0`（断言回填的 `tag_id == 2002`、`ts == 100050`、`payload.val.f == 50.02f`）
  - `snf_get_pending_count == 0`
  - `snf_dequeue(&dequeued) == 1`（断言空队列返回 1）

### 3. AEP-207 全禁用配置整包构建与错误传播语义闭环
- **错误传播与安全回退**：在 `modules/edge_infra/src/edge_infra_base.c` 中为未开启的组件（`CONFIG_EDGE_INFRA_SQLITE=n` / `DDS=n` / `MQTT=n`）写/查询操作统一返回明确错误码（-1 / NULL），消除假成功；
- **定向单测**：在 `tests/test_infra_interfaces.c` 中增加 `test_disabled_null_stubs_error_semantics` 专门验证禁用与 NULL 句柄调用的错误码返回；
- **实测全量构建通过**：配置全部后端禁用后，不仅 `libedge_infra.a` 仅编译基础锚点（0 SQLite/DDS/MQTT 源文件），而且 **`smart_box`、`iems`、`dcu` 三大产品二进制 208/208 全量构建与链接 100% 成功通过**，彻底杜绝未定义符号错误。

### 4. SHM 硬断言与 0-Skip 落地
- 在 `go/pkg/shm/shm.go` 中实现 `CreateOrOpenShadowMatrix`；
- 在 `go/cmd/smart-box-gateway/main_test.go` 中重构 `TestUDSStateSHMReadback`，设置 capacity 为 16384（覆盖 `tags.UDS_SESSION_STATE=12031`），**0 skip，100% 物理硬断言通过**。

### 5. 尺寸门禁自校验与 CI 管道加固
- `scripts/check_binary_sizes.sh` 自校验 `CMAKE_BUILD_TYPE=Release`，非 Release 模式在严格模式下直接报错拦截，可选产物缺失输出显式 `WARN`；
- `.github/workflows/ci.yml` 启用 `shell: bash` 与 `set -euo pipefail`。

---

## 二、测试与门禁验证矩阵 (Verification Matrix)

| 验证层级 / 门禁项 | 目标命令 | 实测结果 | 状态 |
|---|---|---|:---:|
| **全量 Release 构建** | `cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release && ninja -C build` | 483/483 targets 编译通过，0 警告 | ✅ PASS |
| **全禁用整包构建** | `cmake -B /tmp/test_build_none ... -DCONF_FILE=/tmp/prj_none.conf && ninja smart_box iems dcu` | 208/208 编译通过，0 缺失符号 | ✅ PASS |
| **C 单元测试 (-j1)** | `ctest --test-dir build -j1` | 136/136 passed (40.00s) | ✅ PASS |
| **Matrix 全链路仿真** | `./test.sh --omnis` | 10/10 物理场景全通过 (53.00s) | ✅ PASS |
| **Go 全量单元测试** | `cd go && go test -v ./...` | 全绿，SHM 硬断言通过 (0 skip) | ✅ PASS |
| **架构门禁 1: 旧目录清零** | `./scripts/check_no_old_src.sh` | 0 legacy src 引用 | ✅ PASS |
| **架构门禁 2: 符号基线** | `./scripts/check_symbols_baseline.sh` | 7/7 核心符号验证通过 | ✅ PASS |
| **架构门禁 3: 严格符号** | `STRICT_CORE_GATE=1 ./scripts/check_core_symbols.sh` | libedge_core.a 0 禁止第三方符号 | ✅ PASS |
| **架构门禁 4: 核心只读** | `STRICT_CORE_GATE=1 ./scripts/check_core_readonly.sh origin/feature/modular-refactor` | 核心受保护文件 0 违规修改 | ✅ PASS |
| **架构门禁 5: 严格尺寸** | `STRICT_SIZE_GATE=1 ./scripts/check_binary_sizes.sh` | 4/4 产物严控预算达标 (core: 118.7KB) | ✅ PASS |

---

## 三、二进制尺寸实测报告 (Binary Footprint Report)

| 目标产物 | 原始体积 | Stripped 体积 | Warn 预算 | Error 阈值 | 结论 |
|---|---|---|---|---|:---:|
| `edge_core/libedge_core.a` | 121.1 KB | **118.7 KB** | 250.0 KB | 500.0 KB | ✅ PASS |
| `products/iems/bin/iems` | 4.00 MB | **3.59 MB** | 5.00 MB | 8.00 MB | ✅ PASS |
| `products/dcu/bin/dcu` | 2.91 MB | **2.59 MB** | 5.00 MB | 8.00 MB | ✅ PASS |
| `products/smart_box/bin/smart_box` | 3.19 MB | **2.87 MB** | 5.00 MB | 8.00 MB | ✅ PASS |

---

## 四、全量真实 Diffstat 汇总

```text
 .github/workflows/ci.yml                           |   2 +
 CMakeLists.txt                                     |   5 +
 cmd/dcu/main.c                                     |   1 +
 cmd/smart_box/main.c                               |   3 +-
 docs/evidence/AEP-208/diffstat.txt                 |  16 +
 docs/evidence/AEP-208/redlines.md                  |  13 +
 docs/evidence/AEP-208/summary.md                   |  93 ++++
 docs/evidence/AEP-208/tests.log                    | 511 +++++++++++++++++++++
 go/cmd/smart-box-gateway/main_test.go              |  34 +-
 go/pkg/shm/shm.go                                  |  31 ++
 modules/edge_infra/CMakeLists.txt                  |   8 +-
 modules/edge_infra/src/edge_infra_base.c           | 120 +++++
 modules/edge_infra/storage/sqlite_storage_driver.c |  75 ++-
 scripts/check_binary_sizes.sh                      |  23 +-
 tests/test_infra_interfaces.c                      |  65 ++-
 15 files changed, 964 insertions(+), 36 deletions(-)
```
