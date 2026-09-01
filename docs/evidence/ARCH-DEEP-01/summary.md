# ARCH-DEEP-01 交付与验收证据总结 (Summary - DataHub 细粒度事件订阅与过滤分发引擎加深)

> **Ticket**: ARCH-DEEP-01 (Issue #123: DataHub 细粒度事件订阅与过滤分发引擎加深)
> **Branch**: `ticket/ARCH-DEEP-01`
> **Base Commit**: `origin/feature/modular-refactor` (27fe040)
> **PR**: PR #133 (待创建)
> **Date**: 2026-09-01
> **Verification Status**: ✅ 100% PASS (CTest 153/153 + Omnis 10/10 + Go 全绿 + 6 大反熵架构门禁)

---

## 一、需求背景与架构加深交付 (Architecture & Delivery)

在之前的 DataHub 实现中，事件通知机制较为粗粒度：`datahub_add_listener` 仅提供全局 `uv_async_t` 广播唤醒，导致下游业务（DCU、IEMS、SmartBox 等）在收到更新时无法感知具体变更的测点或域，被迫执行 $O(N)$ 影子全表扫描或维护繁琐的 dirty 位图，且单测需要笨重的 `uv_loop` 样板代码。

本工单完成 **ARCH-DEEP-01：DataHub 细粒度事件订阅与过滤分发引擎加深**：
1. **统一数据驱动订阅模型**：
   - 实现了零拷贝直接回调派发：`datahub_event_cb` 签名直接传递 `const pipe_msg_t *msg`，零堆内存分配，零多余内存拷贝；
   - 实现了精确测点订阅：`datahub_subscribe_tag(hub, tag_id, cb, user_ctx)`；
   - 实现了数据域掩码订阅：`datahub_subscribe_domain(hub, domain_mask, cb, user_ctx)`；
   - 实现了细粒度多维组合条件订阅：`datahub_subscribe_filtered(hub, filter, cb, user_ctx)`，支持精确 TagID、位掩码、数据域掩码、目标物理设备 ID 通配（`EDGE_DEVICE_ANY`）以及死区变位上送（Report-by-Exception / Deadband）；
   - 实现了安全动态注销机制：`datahub_unsubscribe(hub, sub_id)`。
2. **零退化向后兼容**：
   - 存量 `datahub_add_listener` 与 `datahub_subscribe(hub, legacy_cb, user_data)` 100% 保持兼容无改动。
3. **自动化测试与微基准**：
   - 编写 `tests/test_datahub_subscription.c`，覆盖精确测点、域/设备过滤、死区 RBE、动态注销以及 64 订阅者满载压测（5/5 测试通过）；
   - CTest 用例数提升至 **153 项 (100% PASS)**！

---

## 二、DoD 验收与验证矩阵 (Verification & DoD)

### 1. `test_datahub_subscription` 实测输出
```text
[==========] tests: Running 5 test(s).
[ RUN      ] test_exact_tag_subscription
[       OK ] test_exact_tag_subscription
[ RUN      ] test_domain_and_device_filter
[       OK ] test_domain_and_device_filter
[ RUN      ] test_deadband_and_rbe
[       OK ] test_deadband_and_rbe
[ RUN      ] test_unsubscribe_and_slot_reuse
[       OK ] test_unsubscribe_and_slot_reuse
[ RUN      ] test_subscription_dispatch_microbenchmark

  [BENCHMARK] 64 Active Subscribers: 10000 writes executed in 10.698 ms (1069.79 ns/write)
[       OK ] test_subscription_dispatch_microbenchmark
[==========] tests: 5 test(s) run.
[  PASSED  ] 5 test(s).
```

### 2. 全量测试与门禁验证矩阵

| 验证项 | 目标命令 | 实测结果 | 状态 |
|---|---|---|:---:|
| **全量 Release 构建** | `cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release && ninja -C build` | 全部目标编译成功，0 警告 | ✅ PASS |
| **C 单元测试 (-j1)** | `ctest --test-dir build -j1` | **153/153 passed (122.22s)**，含 `test_datahub_subscription` | ✅ PASS |
| **Matrix 全链路物理仿真** | `./test.sh --omnis` | **10/10 物理场景全通过 (51.87s)** | ✅ PASS |
| **Go 全量单元测试** | `cd go && go test -v ./...` & `tools/edge-cli` | 全绿 | ✅ PASS |
| **架构门禁 1: 旧目录清零** | `./scripts/check_no_old_src.sh` | 0 legacy src 引用 | ✅ PASS |
| **架构门禁 2: 符号基线** | `./scripts/check_symbols_baseline.sh` | 7/7 核心符号验证通过 | ✅ PASS |
| **架构门禁 3: 严格符号** | `STRICT_CORE_GATE=1 ./scripts/check_core_symbols.sh` | `libedge_core.a` 0 禁止第三方符号 | ✅ PASS |
| **架构门禁 4: 核心只读** | `STRICT_CORE_GATE=1 ./scripts/check_core_readonly.sh origin/feature/modular-refactor` | 核心受保护文件变更均签署豁免 | ✅ PASS |
| **架构门禁 5: 严格尺寸** | `STRICT_SIZE_GATE=1 ./scripts/check_binary_sizes.sh` | 4/4 产物严控预算达标 (core: 121.6KB) | ✅ PASS |
| **架构门禁 6: 生成物可信链** | `STRICT_DRIFT_GATE=1 ./scripts/check_generated_drift.sh` | 编译器一致(delta:0B)，全生成物零漂移 | ✅ PASS |

---

## 三、真实 Diffstat 汇总

```text
 contracts/schema/core_readonly_whitelist.txt |   7 +-
 docs/evidence/ARCH-DEEP-01/diffstat.txt      |  12 +++
 docs/evidence/ARCH-DEEP-01/redlines.md      |  16 +++
 docs/evidence/ARCH-DEEP-01/summary.md       | 115 ++++++++++++++++++++
 docs/evidence/ARCH-DEEP-01/tests.log        | 580 ++++++++++++++++++++++++++++++
 include/edgeos/datahub.h                     |   2 +
 modules/edge_core/include/datahub.h          |  57 ++++++++++++
 modules/edge_core/src/datahub.c              | 130 +++++++++++++++++++++++++++
 modules/edge_core/src/datahub_shm.c          |   8 +-
 tests/CMakeLists.txt                         |  18 ++++
 tests/test_datahub_subscription.c            | 286 ++++++++++++++++++++++++++++++++++++++++++++++
 11 files changed, 1247 insertions(+), 4 deletions(-)
```
