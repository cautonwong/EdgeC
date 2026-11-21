# DCU-D0-02 证据包：把演示行为移出生产路径

- 分支：`dcu/D0-02-remove-demo`
- 起点：`2822bdda`（main）
- 提交：见 `git log --oneline -1`（本文档随提交一起入库）

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `cmd/dcu/dcu_heartbeat.c` | 生产模式（默认）只刷盘 + 心跳日志；冻结管线、每 200 表随机跳闸、遥控测试、3 周期自退全部移到 `DCU_DEMO_MODE=1` 下。新增 `dcu_heartbeat_tick()` / `dcu_heartbeat_control_tick()` 作为定时器与测试共用的执行入口。 |
| `cmd/dcu/dcu_heartbeat.h` | 记录 `demo_mode`、`max_cycles` 与 4 个可观测计数（冻结/跳闸/遥控/自退）；更新契约注释。 |
| `tests/test_dcu_runtime_health.c` | 新增单元测试，覆盖默认生产模式与演示模式两条路径。 |
| `tests/CMakeLists.txt` | 用 `add_edge_test` 注册 `test_dcu_runtime_health`，链接 `dcu_app_lib`。 |
| `acceptance/dcu/dcu_product.py` | 台架启动环境显式设置 `DCU_DEMO_MODE=1`，并加入 env 白名单。 |

未改 `modules/`、`foundation/`、`adapters/`。

## 为什么

计划第 3.2 节 D-01 / 第 8 节 D0-02：生产路径每 5 秒触发日冻结、每 200 块表随机拉合闸、默认 3 个周期后自杀。这些是台架演示，不是产品行为。生产默认必须持续运行且不主动控制现场设备。

## 完成定义逐项

- [x] 默认生产模式只保留刷盘、心跳日志、正常采集轮询；不冻结、不随机跳闸、不自退。
      `test_production_cycle_has_no_demo_behavior`：3 个周期后 `demo_freezes==0`、`demo_relays==0`、`control_triggers==0`、`stop_requested==0`，且缓存遥测已落盘。
- [x] `DCU_DEMO_MODE=1` 恢复台架行为。
      `test_demo_mode_restores_bench_behavior`：201 块表下一周期推 201 次冻结、命中一次 OI 8000 跳闸、遥控触发一次；第 3 周期后进入自退路径。
- [x] `DCU_DEMO_MODE` 未设置或 `0` 时 `DCU_HEARTBEAT_CYCLES` 不再导致自退；0/未设置表示一直运行。
      `dcu_heartbeat_start()` 只在 `demo_mode` 为真时读取 `DCU_HEARTBEAT_CYCLES` 并设置 `max_cycles`；生产模式 `max_cycles` 恒为 0。
- [x] 只改 `cmd/dcu/`、`tests/`、`acceptance/`、`docs/evidence/DCU-D0-02/`。
- [x] 新测试覆盖三条要求，`add_edge_test` 注册并链接 `dcu_app_lib`。
- [x] 验收显式开启演示：`acceptance/dcu/dcu_product.py` 设置 `DCU_DEMO_MODE=1`。
      原因：`tests/test_dcu_acquisition.py::test_dcu_sends_trip_control_frame` 依赖 2 秒遥控定时器。这是台架显式开启，生产默认关闭；未改该用例判据。
- [x] 构建、`ctest -R dcu`、`--omnis dcu`、ASAN 均通过（见 `tests.log`、`runtime.log`）。
- [x] 缺陷对照（9.4 defect）：临时把 `demo_mode` 强制为 `true` 模拟修复前行为，`test_production_cycle_has_no_demo_behavior` 变红；还原后变绿（见 `tests.log` 的 A/B 段）。

## 可观测性说明

`service_stop` 是不可注入的真实服务对象。测试用 `stop_requested` 计数（在调用 `service_stop` 前自增）作为"是否进入自退路径"的观测点；生产模式断言该计数为 0。测试传 `svc=NULL`，因此不会真的调用 `service_stop`。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j8` | exit 0，0 warning |
| `cd build && ctest --output-on-failure -R dcu` | 19/19 通过（含 `test_dcu_runtime_health`） |
| `./test.sh --omnis dcu` | dcu-acceptance 2 passed、dcu-northbound 4 passed、退出码 0；`test_dcu_sends_trip_control_frame` 在 run-scope 内 |
| ASAN 构建 + `test_dcu_runtime_health` | 2/2 通过，无 LeakSanitizer 报告 |
| `clang-format --dry-run --Werror`（3 个改动 C 文件） | 通过 |
| `ci/scripts/run_fanalyzer.py --paths cmd/dcu,tests` | 改动文件 0 条；测试目录其余报告为既有问题，与本次无关 |

## 证据文件

- `tests.log`：构建、`ctest -R dcu`、新测试 verbose、A/B 红绿对照、ASAN、fanalyzer 摘要
- `runtime.log`：`./test.sh --omnis dcu` 完整输出
- `diffstat.txt`：`git show --stat`
