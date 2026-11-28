# DCU-D6-09 独立验证 (`verify.md`)

规格：[spec.md](spec.md)。基线 `bc777aaa`，worker 提交 `cfcfdd6d`，合并 `acc14dd8`。

## 独立推导

从 worker 生成的原始 lcov 文件 `build/coverage_clean.info` 逐条累加 `cmd/dcu/` 的 `LF`/`LH`，不读 worker 的摘要：

- `LF` = 7039，`LH` = 6112，rate = 6112/7039 = **86.8305%**。
- 下限取工具报出的 86.8，满足 `86.8305 + 1e-9 >= 86.8`；向上取整到 86.9 会高于实测并让门禁变红，因此 86.8 是唯一正确写法。
- 旧下限文件中没有 `cmd/dcu` 条目（隐式 0），本次是新增；其余条目未改（`git diff` 只加 7 行注释与 1 行条目）。
- `cmd` 拆分后为 78.2%，仍高于原 76.5%，没有降低任何既有下限。

## Baseline / Treatment

| 项 | 命令 | 结果 |
| --- | --- | --- |
| baseline（旧下限） | `check_coverage.py --info build/coverage_clean.info --root /tmp/dcu-D6-09 --baseline <HEAD~>` | rc=0 |
| treatment（新下限） | `check_coverage.py --info build/coverage_clean.info --root /tmp/dcu-D6-09` | rc=0，`cmd/dcu 86.8/86.8 PASS` |

## foundation/src/protocol 的缺口（独立复核）

- `find build -name '*.gcno' | grep foundation` = **0**。
- `coverage_clean.info` 中 `SF:.../foundation/src/protocol/` 条目 = **0**。
- `foundation/CMakeLists.txt` 没有链接 `edge_common_interface`，`--coverage` 只加在该接口目标上。

结论：现有覆盖率入口无法测量该目录；写任何下限都会让 `check_coverage.py` 因 0 行 scope 报 MISSING 而失败。

## 结论

- `cmd/dcu` 下限提升：**VERIFIED**（独立复算、baseline/treatment 对照、旧下限未降）。
- `foundation/src/protocol` 下限：**NOT COVERED**，D6-09 按部分交付验收；后续 `DCU-D6-09b` 需先给 foundation 静态库加插桩并重定基线，不在本单元假装完成。
