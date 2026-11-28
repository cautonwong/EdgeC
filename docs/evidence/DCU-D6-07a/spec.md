# DCU-D6-07a 五栏规格：4096 表规模基准测试与性能门禁对齐

## 1. 业务规则
1. 将 `tests/test_dcu_scale_benchmark.c` 从 `4 buses x 500 meters = 2000 meters` 扩展到产品目标规模 `4 buses x 1024 meters = 4096 meters`（与 `cmd/dcu/dcu_inventory.h` 中 `MAX_METERS = 4096` 一致），每表 3 个 OAD（共 12288 OAD 调度规模）。
2. 将第 97 行陈旧注释 `0x20020200` 处的 `// A/B/C 相电压` 修正为符合 698 标准的注释或替换为真实三相电压/A 相电流（`0x20000200, 0x20010200, 0x20040200`），避免把 `0x20020200`（电压相角）误标为相电压。
3. 保持 `EDGE_PERF_GATE=1` 性能门禁契约：
   - 默认 `ctest`（`EDGE_PERF_GATE != 1`）或插桩构建（`EDGE_SANITIZER_BUILD`）只打印 Avg / P99 / Max step latency 与 submits 数量，不因宿主调度噪声失败。
   - `EDGE_PERF_GATE=1` 时强制校验 `p99_ms < p99_limit_ms`（默认 `5.0 ms`，可由 `EDGE_PERF_P99_MS` 覆盖）；注：`EDGE_PERF_AVG_US=20` 为 `test_dds_shm_zerocopy_perf` 专用，不适用于 4096 表轮询步进基准。

## 2. 不变量
1. `inv.count == 4096`（`MAX_METERS`）且 `dcu_inventory_add` 全部成功。
2. `dcu_poller_step` 在 4096 表规模下仍保持每步每通道至多提交 1 项，不阻塞事件循环。
3. 内存无泄漏（`dcu_poller_destroy` 与 `dcu_inventory_destroy`（如适用）完整清理，ASAN 下通过）。

## 3. 可观测输出
- 标准输出打印 `[BENCHMARK] 4 Buses x 1024 Meters (4096 Meters, 12288 OADs):`，包含 `Avg Step Latency`、`P99 Step Latency`、`Max Step Latency`、`Submits issued`。
- 默认运行与 `EDGE_PERF_GATE=1 EDGE_PERF_P99_MS=20 EDGE_PERF_AVG_US=20` 均返回 0。

## 4. 验收判据
1. `ctest --output-on-failure -R dcu_scale_benchmark` 通过。
2. `EDGE_PERF_GATE=1 EDGE_PERF_P99_MS=20 EDGE_PERF_AVG_US=20 ctest --output-on-failure -R dcu_scale_benchmark` 通过。
3. ASAN 构建下 `test_dcu_scale_benchmark` 无内存泄漏或越界报告。

## 5. 非目标
- 不修改 `cmd/dcu/` 生产代码或 `ci/` 脚本。
- 不在本单实现 24 小时 soak 脚本。
