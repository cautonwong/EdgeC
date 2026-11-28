# DCU-D6-09 规格：覆盖率下限只升不降

| 业务规则 | 不变量 | 可观测输出 | 验收判据 | 非目标 |
| --- | --- | --- | --- | --- |
| 1. 用仓库既有覆盖率入口（`./test.sh --coverage` 或 `ci/scripts/check_coverage.py` 认可的同一条构建/运行路径）测 `cmd/dcu` 与 `foundation/src/protocol` 的 line coverage。<br>2. 把 `ci/baseline/coverage_floor.txt` 中这两个目录的下限提高到本次实测值并向上取整到 0.1 个百分点，只升不降。<br>3. 若实测低于现下限，说明测量路径或代码有回归：保留原下限并记录原因，不调低。 | 1. `check_coverage.py` 用新下限通过。<br>2. 新下限 >= 旧下限，逐目录比较。<br>3. 不改测试用例来制造覆盖，不删除或跳过任何测试。 | 1. 覆盖率日志中两目录的 line coverage 百分比与工具输出路径。<br>2. `ci/baseline/coverage_floor.txt` 的 diff（旧值 → 新值）。<br>3. 复跑 `python3 ci/scripts/check_coverage.py` 的退出码与关键行。 | 1. 新下限等于实测值向上取整，且两目录各自独立比较。<br>2. 复跑覆盖率门禁退出 0。<br>3. 全量 `ctest` 仍通过（覆盖率构建上的测试不得因插桩改变）。<br>4. 证据写入 `docs/evidence/DCU-D6-09/`。 | 1. 不为提升覆盖率新增业务代码或断言。<br>2. 不改 `check_coverage.py` 的判定规则。<br>3. 不调整其他目录的下限，除非实测同样上升且一并记录。 |
