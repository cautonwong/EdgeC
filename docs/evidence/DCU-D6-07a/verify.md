# DCU-D6-07a 独立验证报告

## 1. 独立预期推导
- **规格与契约依据**：`cmd/dcu/dcu_inventory.h` 定义 `MAX_METERS = 4096`；`.github/workflows/ci.yml:119` 使用 `EDGE_PERF_GATE=1 EDGE_PERF_P99_MS=20 EDGE_PERF_AVG_US=20`。
- **独立期望值**：
  - `4 buses x 1024 meters` 应注册 `4096` 台电表（`inv.count == MAX_METERS`），3 个 OAD（`0x20000200, 0x20010200, 0x20040200`）共 `12288` 个 OAD；400 步 × 4 总线 × 1024 表 × 3 OAD = `4915200` 次提交（每步每总线至多 1 项在途，`g_pend_n <= 4`）。
  - `EDGE_PERF_P99_MS=20` 门禁下 P99 step latency 应远低于 `20.0 ms`（`EDGE_PERF_AVG_US=20` 属 `test_dds_shm_zerocopy_perf` 专用，不在本用例强制）。

## 2. 合并树实测结果（`feat/dcu`）
- **默认运行**：`ctest --test-dir build --output-on-failure -R dcu_scale_benchmark` — **1/1 passed**。
- **性能门禁运行**：`EDGE_PERF_GATE=1 EDGE_PERF_P99_MS=20 EDGE_PERF_AVG_US=20 ctest --test-dir build --output-on-failure -R dcu_scale_benchmark` — **1/1 passed**。
- **全量回归**：`ctest --test-dir build --output-on-failure` — **382/382 passed**。
- **结论**：`VERIFIED`。
