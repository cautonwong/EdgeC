# DCU-D6-09 覆盖率下限只升不降

- 日期：2026-11-28
- 分支：`dcu/D6-09-coverage-floor`，基线提交 `bc777aaa`，工作树 `/tmp/dcu-D6-09`
- 规格：本目录 `spec.md`（五栏）
- 改动文件：`ci/baseline/coverage_floor.txt`（仅此一个产品/配置文件）

## 结论

| 目录 | 旧下限 | 实测 line coverage | 新下限 | 判定 |
| --- | --- | --- | --- | --- |
| `cmd/dcu` | 无（0） | 6112/7039 = 86.8305%（工具报 86.8%） | 86.8 | 上调，`check_coverage.py` 通过 |
| `foundation/src/protocol` | 无（0） | 0 有效行：现有入口不插桩 | 不设 | 测量路径缺插桩，见下 |

新增 `cmd/dcu` 范围后，`cmd` 的余量范围变为 78.2%（原 76.5%），仍高于原下限，
未调低任何已有下限。其余范围下限不变。

### `foundation/src/protocol` 为何不设下限

现有覆盖率入口（`-DCOVERAGE=ON` → `--coverage` 经
`edge_common_interface` 下发）只覆盖链接了该接口目标的目标。`edge_codec`
等 foundation 静态库**没有**链接它（`foundation/CMakeLists.txt:106-108`），
因此 `foundation/src/protocol/*.c` 不产生 `.gcno`/`.gcda`：

- `find build -name '*.gcno' | grep -E 'foundation|edge_codec'` → 0
- `lcov` 采集结果中 `foundation/src` 的 `SF:` 条目 → 0

`check_coverage.py` 对 0 行的范围判 `MISSING`（退出 1，`check_coverage.py:83-86`），
因此写入 `foundation/src/protocol|<任意值>` 会让门禁变红，违反规格不变量
“`check_coverage.py` 用新下限通过”。要测该目录必须先改构建（给 foundation
静态库接插桩），那会改变 ALL/foundation 的测量口径，属于本单元非目标；
此处保留为 finding，不在本单元扩面。

## 测量路径（与 CI 覆盖率步骤一致）

`./test.sh --coverage` 只产出 gcovr HTML，不产出 `check_coverage.py` 所需的
lcov `.info`；故按 CI 的同一路径测量（`.github/workflows/ci.yml` 的
`Generate Code Coverage` 步骤）：

```bash
cd /tmp/dcu-D6-09 && mkdir build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DCOVERAGE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build . -j"$(nproc)"
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/tmp/dcu-D6-09/.local/linux-x86_64/lib" \
  ctest --output-on-failure -E '^app_platform_e2e$'
lcov --capture --directory . --output-file coverage.info --ignore-errors gcov,source
lcov --remove coverage.info '/usr/*' '*/tests/*' '*/third-party/*' \
  --output-file coverage_clean.info --ignore-errors gcov,source
python3 ci/scripts/check_coverage.py --info build/coverage_clean.info --root /tmp/dcu-D6-09
```

- 构建：1313 个目标，退出 0（唯一 warning 来自 third-party open62541）。
- 单测：`100% tests passed, 0 tests failed out of 380`，472.88 s，退出 0。
- 未改任何测试、未改 `check_coverage.py`、未新增业务代码或断言。

## 数值口径

- 新下限取工具报告的一位小数值（86.8%），`check_coverage.py` 以
  `rate + 1e-9 >= floor` 判定，6112/7039 = 86.8305% ≥ 86.8，通过。
- 若取下一位（86.9）将高于实测 86.8305%，门禁变红，与规格不变量冲突；
  故记录精确分数并取 86.8，未为了“更高”而制造失败。

## 门禁

| 门禁 | 命令 | 结果 |
| --- | --- | --- |
| 覆盖率（旧下限基线） | `check_coverage.py --baseline <HEAD 版本>` | 退出 0，全部 PASS |
| 覆盖率（新下限） | `python3 ci/scripts/check_coverage.py --info build/coverage_clean.info --root /tmp/dcu-D6-09` | 退出 0，`cmd/dcu 86.8% / 86.8% PASS` |
| 全量单测 | `ctest --output-on-failure -E '^app_platform_e2e$'` | 380/380，退出 0 |
| `git diff --check` | `git diff --check` | 退出 0 |

`run_guard_selftest.py` 不受本次改动影响：其覆盖率自测用独立的
`tests/guards/fixtures/coverage_floor.txt`（`ci/scripts/run_guard_selftest.py:1706`），
不读 `ci/baseline/coverage_floor.txt`，故未跑全量守卫。

## 工件

- `coverage.log`：构建/ctest/lcov/新旧下限复跑门禁的原始输出（含两个目录的
  lcov 行与精确分数）。
- `floor.diff`：`ci/baseline/coverage_floor.txt` 的 diff。
- `diffstat.txt`：改动统计与工作树状态。

## 与规格五栏的对应

| 规格要点 | 证据 |
| --- | --- |
| 业务规则 1/2：既有入口测量、只升不降 | 本文件“测量路径”与结论表 |
| 业务规则 3：低于现下限则保留并记录 | 无已有下限被下调；`cmd` 余量 78.2% ≥ 76.5% |
| 不变量 1：新下限通过门禁 | `coverage.log` 末尾退出 0 |
| 不变量 2：新下限 ≥ 旧下限 | 唯一新增范围；无下调 |
| 不变量 3：不改测试制造覆盖 | 未改测试；ctest 380/380 |
| 可观测输出：百分比、diff、退出码 | 结论表、`floor.diff`、`coverage.log` |
| 非目标：不改判定规则/其他目录 | 只改 `coverage_floor.txt` 增加注释块与 `cmd/dcu` |
