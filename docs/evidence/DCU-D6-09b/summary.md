# DCU-D6-09b foundation 覆盖率插桩与基线重定

- 日期：2026-11-28
- 分支：`dcu/D6-09b-foundation-coverage`，基线提交 `f443e688`，工作树 `/tmp/dcu-D6-09b`
- 规格：本目录 `spec.md`（五栏）
- 改动文件：`foundation/CMakeLists.txt`（构建）、`ci/baseline/coverage_floor.txt`（下限）

## 结论

新增 `foundation/src/protocol` 下限，`foundation` 下限上调；其余既有下限不变，
无任何下限被下调。

| 目录 | 旧下限 | 实测 line coverage | 新下限 | 判定 |
| --- | --- | --- | --- | --- |
| `foundation/src/protocol` | 无 | 1353/1722 = 78.5714%（工具报 78.6%） | 78.5 | 新增，`check_coverage.py` 通过 |
| `foundation`（不含 protocol） | 87.0 | 223/236 = 94.4915%（工具报 94.5%） | 94.4 | 上调，通过 |
| `ALL` | 74.5 | 30973/38665 = 80.1060%（工具报 80.1%） | 74.5（保留） | 见下“为何不调 ALL” |
| `cmd` | 76.5 | 78.2%~80.8% 波动 | 76.5（保留） | 见下波动 |

`adapters` / `modules/*` / `cmd/dcu` 无变化，实测均高于既有下限。

## 构建改动（`foundation/CMakeLists.txt`）

`-DCOVERAGE=ON` 只经 `edge_common_interface` 下发 `--coverage`；`edge_codec` 等
foundation 静态库不链接该接口，故不插桩。改动是在 `edge_codec` 目标上直接加
同一编译 flag，只在覆盖率构建生效：

```cmake
# edge_codec holds foundation/src/protocol. It does not link edge_common_interface,
# so the coverage flags set there do not reach it; instrument it directly instead.
if(COVERAGE OR ENABLE_COVERAGE)
    target_compile_options(edge_codec PRIVATE --coverage)
endif()
```

链接侧无需改：测试可执行文件已通过 `edge_common_interface` 链接 `--coverage`
（含 libgcov），`edge_codec` 目标编译插桩后 `.gcda` 正常落盘。

验证（`coverage.log`）：

- 覆盖率构建：`dlt698_codec.c` 编译命令含 `--coverage`，`edge_math` 的
  `control_algo.c` 不含；`find build -name '*.gcno' | grep foundation` = 16，
  全部在 `foundation/CMakeFiles/edge_codec.dir/`；`foundation/src/protocol/`
  的 15 个 `.c` 均产出 `.gcno`，`coverage_clean.info` 有 12 个 protocol `SF:` 条目。
- 非覆盖率构建：`find build-nocov -name '*.gcno'` = 0，`edge_codec` 编译命令
  不含 `--coverage`。非覆盖率构建的 flags/产物/依赖不变。

## 作用域收窄（相对“插桩全部 foundation 静态库”）

先按目录级 `add_compile_options(--coverage)` 插桩全部 foundation 库实测：
`foundation` 口径从“头文件”扩到全部 `.c`，实测 72.8%（或拆出 protocol 后
`foundation` 66.4%），低于既有 87.0，必然要求下调，违反规格“任何既有下限不得
降低”。故改为只插 `edge_codec`——即承载 `foundation/src/protocol` 的那个静态库，
这也是满足规格可观测输出（protocol 有 `.gcno/.gcda`、有 `SF:` 条目）的最小改动。
`edge_math` / `edge_foundation` / `edge_netbuf` / `edge_log` / `edge_crypto`
保持原有不插桩状态，不扩大口径。

## 测量路径（与 D6-09 / CI 覆盖率步骤同路径）

```bash
cd /tmp/dcu-D6-09b && mkdir build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DCOVERAGE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build . -j"$(nproc)"
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/tmp/dcu-D6-09b/.local/linux-x86_64/lib" \
  ctest --output-on-failure -E '^app_platform_e2e$'
lcov --capture --directory . --output-file coverage.info --ignore-errors gcov,source
lcov --remove coverage.info '/usr/*' '*/tests/*' '*/third-party/*' \
  --output-file coverage_clean.info --ignore-errors gcov,source
python3 ci/scripts/check_coverage.py --info build/coverage_clean.info --root /tmp/dcu-D6-09b
```

- 构建：1313 个目标，退出 0。
- 单测：`100% tests passed, 0 tests failed out of 380`，356.31 s，退出 0。
- 未改测试、未改 `check_coverage.py`、未新增业务代码或断言。

## 为何不调 ALL / cmd（Explain the Number）

同一口径两次采集，`foundation` 与 `foundation/src/protocol` 完全一致
（protocol 1353/1722、foundation 头文件 97/100 + edge_codec 非 protocol
126/136，两轮相同），故按实测上调/新增。

`ALL` 与 `cmd` 不稳定：`cmd/edgeos-launcher/main.c` 作为测试子进程运行时，
其 `.gcda` 是否落盘取决于子进程退出方式。落盘时 `cmd` 多出该文件 200 行
（全部 0 命中），同一口径 `cmd` 由 80.8% 变 78.2%、`ALL` 由 80.1% 变 79.7%
（`30973/38865`）。若把 `ALL`/`cmd` 下限提到单轮实测值，另一种情形会变红。
故保留 `ALL|74.5`、`cmd|76.5`，两种情形都 PASS。这符合“只升不降”（不下调），
且 `ALL`/`foundation` 的扩面已重测确认仍高于既有下限。

## 门禁

| 门禁 | 命令 | 结果 |
| --- | --- | --- |
| 覆盖率（新下限） | `check_coverage.py --info build/coverage_clean.info --root /tmp/dcu-D6-09b` | 退出 0，全部 PASS（含 `foundation/src/protocol 78.6% / 78.5%`） |
| 全量单测 | `ctest --output-on-failure -E '^app_platform_e2e$'` | 380/380，退出 0 |
| 构建告警 | `check_build_warnings.py --log build-nocov.log --root /tmp/dcu-D6-09b` | 0 new violations，退出 0（clean 非覆盖率构建） |
| `git diff --check` | `git diff --check` | 退出 0 |

`run_guard_selftest.py` 不受影响：其覆盖率自测用独立的
`tests/guards/fixtures/coverage_floor.txt`，不读 `ci/baseline/coverage_floor.txt`。

## 工件

- `coverage.log`：构建/ctest/lcov/gcno 清单/精确分数/新旧下限门禁原始输出。
- `floor.diff`：`ci/baseline/coverage_floor.txt` 的 diff。
- `diffstat.txt`：改动统计与工作树状态。
- `spec.md`：五栏规格（本单元既有）。

## 与规格五栏的对应

| 规格要点 | 证据 |
| --- | --- |
| 业务规则 1/2：foundation/src/protocol 产出 .gcno/.gcda | `coverage.log` 的 gcno 清单与 protocol SF 条目 |
| 业务规则 3：新增 protocol 下限、ALL/foundation 重测只升不降 | 结论表；`floor.diff` 无下调条目 |
| 不变量 1：非覆盖率构建不变 | 非覆盖率构建 0 个 .gcno、无 `--coverage` |
| 不变量 2：不改 `check_coverage.py` | 只改 `foundation/CMakeLists.txt` 与 baseline |
| 不变量 3：新下限不高于实测、既有下限不降 | 78.5≤78.5714、94.4≤94.4915；`floor.diff` |
| 可观测输出：gcno/SF/百分比 | `coverage.log` |
| 验收：ctest、门禁退出 0、warnings、diff --check | 门禁表 |
| 非目标：不新增测试凑覆盖 | 未改测试，ctest 380/380 |
