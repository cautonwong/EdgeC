# ARCH-DEEP-02 交付与验收证据总结 (Summary - 南向驱动自动轮询调度与遥测编排器加深)

> **Ticket**: ARCH-DEEP-02 (Issue #125: 南向驱动自动轮询调度与遥测编排器加深)
> **Branch**: `ticket/ARCH-DEEP-02`
> **Base Commit**: `origin/feature/modular-refactor` (53d8204)
> **PR**: PR #153 (待创建)
> **Date**: 2026-09-02
> **Verification Status**: ✅ 100% PASS (CTest 154/154 + Omnis 10/10 + Go 全绿 + 6 大反熵架构门禁)

---

## 一、需求背景与架构加深交付 (Architecture & Delivery)

在传统驱动设计中，`driver_manager.c` 仅作为浅层驱动模板数组容器（~95 行），导致上层应用（如 DCU、IEMS、SmartBox 等）必须在各业务 `main.c` 中重复手写 150+ 行脆弱的定时轮询、Buffer 申请释放、请求编码、物理 IO 发送、响应解码及入库 DataHub 逻辑，不仅代码严重冗余，而且容易因异常路径导致 `net_buf` 泄漏并难以对齐工业级 MTT 挂钟确定性调度。

本工单完成 **ARCH-DEEP-02：南向驱动自动轮询调度与遥测编排器加深 (`edge_polling_orchestrator`)**：
1. **统一自动轮询编排引擎**：
   - 实现了声明式多通道轮询注册：`driver_manager_add_poll_target(mgr, device_id, protocol, io_handle, tag_ids, tag_count, policy)`；
   - 实现了自闭环请求-响应-入库生命周期：自动从 `io_pool` 申请 Buffer、调用 `encode_request`、下发 `write_async`、在接收端解码并直接通过 `datahub_write_shadow` 零拷贝写入 DataHub；
   - 实现了工业级 MTT 挂钟时间对齐算法：利用 `clock_gettime(CLOCK_REALTIME)` 计算下一周期整点边界，自动补偿网络与 IO 延迟抖动；
   - 实现了通道级超时看门狗与统计指标监控：`driver_manager_get_target_stats(mgr, device_id, stats)`，支持连续失败计数与故障隔离；
   - 实现了手动即时触发与启停控制：`driver_manager_trigger_poll_now`、`driver_manager_set_target_active`、`driver_manager_stop_auto_polling`。
2. **SDK 与公共头文件导出**：
   - 在 `include/edgeos/driver.h` 中导出 `edgeos_poll_policy_t`、`edgeos_orchestrator_cfg_t`、`edgeos_poll_stats_t`，供二开项目零样板代码使用。
3. **自动化测试套件**：
   - 编写 `tests/test_driver_orchestrator.c`，覆盖初始化注册、手动触发与入库、挂钟定时与超时看门狗、通道暂停恢复与资源销毁（3/3 测试通过）；
   - CTest 用例数提升至 **154 项 (100% PASS)**！

---

## 二、DoD 验收与验证矩阵 (Verification & DoD)

### 1. `test_driver_orchestrator` 实测输出
```text
[==========] tests: Running 3 test(s).
[ RUN      ] test_orchestrator_init_and_registration
[2026-09-02 19:22:45][INFO][IO] Registered Protocol Template: MockSensor
[       OK ] test_orchestrator_init_and_registration
[ RUN      ] test_orchestrator_manual_trigger_and_feed
[2026-09-02 19:22:45][INFO][BUS] Sealed memfd Ring created (FD: 3, Size: 8 KB)
 [Metrics] Registered: Tag=12001, Name=datahub.ring.watermark
 [Metrics] Registered: Tag=12002, Name=datahub.ring.drop_count
 [Metrics] Registered: Tag=12003, Name=datahub.ring.read_latency
 [Metrics] Registered: Tag=12004, Name=datahub.shadow.write_count
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d4fbc0: Created (Refs: 1)
[2026-09-02 19:22:45][INFO][IO] Registered Protocol Template: MockSensor
[2026-09-02 19:22:45][INFO][IO] [Orchestrator] Southbound Auto-Polling Orchestrator Started
[2026-09-02 19:22:45][INFO][IO] [Orchestrator] Added poll target Dev=101 Proto=MockSensor Interval=1000ms
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d4fbc0: Ref (1 -> 2)
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d4fbc0: Ref (2 -> 3)
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d4fbc0: Unref (3 -> 2)
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d4fbc0: Unref (2 -> 1)
[2026-09-02 19:22:45][INFO][IO] [Orchestrator] Southbound Auto-Polling Orchestrator Stopped
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d4fbc0: Unref (1 -> 0)
[       OK ] test_orchestrator_manual_trigger_and_feed
[ RUN      ] test_orchestrator_periodic_timer_and_timeout
[2026-09-02 19:22:45][INFO][BUS] Sealed memfd Ring created (FD: 13, Size: 8 KB)
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d502d0: Created (Refs: 1)
[2026-09-02 19:22:45][INFO][IO] Registered Protocol Template: MockSensor
[2026-09-02 19:22:45][INFO][IO] [Orchestrator] Southbound Auto-Polling Orchestrator Started
[2026-09-02 19:22:45][INFO][IO] [Orchestrator] Added poll target Dev=202 Proto=MockSensor Interval=20ms
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d502d0: Ref (1 -> 2)
[2026-09-02 19:22:45][WARN][IO] [Orchestrator] Target 202 (MockSensor) polling response timeout (fails: 1)
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d502d0: Ref (2 -> 3)
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d502d0: Unref (3 -> 2)
[2026-09-02 19:22:45][WARN][IO] [Orchestrator] Target 202 (MockSensor) polling response timeout (fails: 2)
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d502d0: Ref (2 -> 3)
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d502d0: Unref (3 -> 2)
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d502d0: Unref (2 -> 1)
[2026-09-02 19:22:45][INFO][IO] [Orchestrator] Southbound Auto-Polling Orchestrator Stopped
[2026-09-02 19:22:45][INFO][CORE] NetBufPool 0x55b4a5d502d0: Unref (1 -> 0)
[       OK ] test_orchestrator_periodic_timer_and_timeout
[==========] tests: 3 test(s) run.
[  PASSED  ] 3 test(s).
```

### 2. 全量测试与门禁验证矩阵

| 验证项 | 目标命令 | 实测结果 | 状态 |
|---|---|---|:---:|
| **全量 Release 构建** | `cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release && ninja -C build` | 全部目标编译成功，0 警告 | ✅ PASS |
| **C 单元测试 (-j1)** | `ctest --test-dir build -j1` | **154/154 passed (103.09s)**，含 `test_driver_orchestrator` | ✅ PASS |
| **Matrix 全链路物理仿真** | `./test.sh --omnis` | **10/10 物理场景全通过 (51.87s)** | ✅ PASS |
| **Go 全量单元测试** | `cd go && go test -v ./...` & `tools/edge-cli` | 全绿 | ✅ PASS |
| **架构门禁 1: 旧目录清零** | `./scripts/check_no_old_src.sh` | 0 legacy src 引用 | ✅ PASS |
| **架构门禁 2: 符号基线** | `./scripts/check_symbols_baseline.sh` | 7/7 核心符号验证通过 | ✅ PASS |
| **架构门禁 3: 严格符号** | `STRICT_CORE_GATE=1 ./scripts/check_core_symbols.sh` | `libedge_core.a` 0 禁止第三方符号 | ✅ PASS |
| **架构门禁 4: 核心只读** | `STRICT_CORE_GATE=1 ./scripts/check_core_readonly.sh origin/feature/modular-refactor` | 0 违规核心修改 | ✅ PASS |
| **架构门禁 5: 严格尺寸** | `STRICT_SIZE_GATE=1 ./scripts/check_binary_sizes.sh` | 4/4 产物严控预算达标 | ✅ PASS |
| **架构门禁 6: 生成物可信链** | `STRICT_DRIFT_GATE=1 ./scripts/check_generated_drift.sh` | 编译器一致(delta:0B)，全生成物零漂移 | ✅ PASS |

---

## 三、真实 Diffstat 汇总

```text
 docs/evidence/ARCH-DEEP-02/diffstat.txt      |  12 +++
 docs/evidence/ARCH-DEEP-02/redlines.md      |  16 +++
 docs/evidence/ARCH-DEEP-02/summary.md       | 115 ++++++++++++++++++++
 docs/evidence/ARCH-DEEP-02/tests.log        | 590 ++++++++++++++++++++++++++++++
 include/edgeos/driver.h                  |   5 +-
 modules/edge_io/include/driver_manager.h |  85 ++++++
 modules/edge_io/src/driver_manager.c     | 437 ++++++++++++++++++++++++++++++-
 tests/CMakeLists.txt                     |  18 ++
 tests/test_driver_orchestrator.c         | 313 ++++++++++++++++++++++++++++++++++++++++++++++
 9 files changed, 1591 insertions(+), 2 deletions(-)
```
