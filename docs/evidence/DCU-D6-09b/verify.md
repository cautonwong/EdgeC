# DCU-D6-09b 独立验证 (`verify.md`)

规格：[spec.md](spec.md)。基线 `f443e688`，worker HEAD `a755e60d`，合并 `1ed708e1`。

## 独立复算

从 worker 的原始 `build/coverage_clean.info` 累加，不读摘要：

- `foundation/src/protocol`：1353/1722 = **78.5714%**，新增下限 78.5，满足 `实测 + 1e-9 >= 下限`。
- `foundation`（工具口径排除 protocol 子域）：223/236 = **94.4915%**，旧下限 87 上调到 94.4。
- `find build -name '*.gcno' | grep foundation` = **16**，全部在 `edge_codec.dir`；D6-09 时为 0。
- `check_coverage.py --info build/coverage_clean.info --root .` → rc=0，两个新 scope PASS，其余 scope 无下调。

## 构建改动核对

- `foundation/CMakeLists.txt` 只在 `COVERAGE OR ENABLE_COVERAGE` 下给 `edge_codec` 加 `--coverage`；非覆盖率构建的编译命令不含该标志（worker 用 `build-nocov` 复核）。
- 没有把全部 foundation 库一起插桩：那样 foundation 口径从“头文件”扩到全部 `.c`，实测 72.8% 低于既有 87 下限，只能下调，违反规格；本单选择最小目标。

## 局限

- `foundation/src/protocol|78.5` 与 `foundation|94.4` 距实测仅 0.07/0.09 个百分点，未来确定性的覆盖下降会按设计让门禁变红。
- `ALL` 与 `cmd` 保留旧下限：`cmd/edgeos-launcher/main.c` 子进程 `.gcda` 落盘不稳定，同一构建两次测量相差约 0.4 个百分点。

## 结论

**VERIFIED**：插桩、实测、下限与门禁均独立复现；无既有下限降低。
