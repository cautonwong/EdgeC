# DCU-D0-07 自审

## 逐条核对

| 结论 | 依据 | 核对结果 |
| --- | --- | --- |
| 生成器支持 `--root`，默认仓库根 | `scripts/extract_dcu_698_catalog.py` 的 `catalog_paths` 与 `main` | 一致；`--check --root .` 退出 0 |
| 门禁只有一处比对实现 | `ci/scripts/check_dcu_698_catalog.py` 只调用生成器 `--check` | 一致 |
| 门禁正例：仓库根通过 | `run_guard_selftest.py` 的 TEST_MATRIX 条目 | 一致 |
| 门禁负例：真实 docs 副本改坏一行后拒收 | `[CATALOG_GATE]` 段，`tempfile.TemporaryDirectory` | 一致；`tests.log` 有两条 PASS |
| 负例不绕开 26 类/400 行断言 | 副本是完整的 `docs/698_parsed.md`（525 KB）与两份产物 | 一致 |
| 临时目录不写仓库 | `with tempfile.TemporaryDirectory()`，退出即删 | 一致；`git status` 无残留 |
| CI 步骤存在 | `.github/workflows/ci.yml:187` | 一致 |
| 本地镜像与 CI 一致 | `ci/scripts/run_arch_gates.sh` 同一行；parity 11 条 | 一致 |
| 改坏仓库目录会变红 | `tests.log` 的 `broken_exit=1`，恢复后 `restored_exit=0` | 一致 |

## 设计取舍

- 没有把负例做成 `tests/guards/fixtures/` 下的静态目录。静态副本要把 525 KB 的解析件与 193 KB 的产物再存一份，改了原文就两边漂移，反而制造新的不一致。动态副本每次从仓库取真实文件。
- 没有在门禁里重新实现解析与比对。生成器是唯一实现，门禁只负责调用与输出。
- 矩阵条目 `bad_target` 置空，负例放在独立的 `[CATALOG_GATE]` 段。原因是现有矩阵只接受静态路径，硬塞动态逻辑会让矩阵循环变复杂；文件里已有覆盖率门禁的先例。

## 复核

- `git status` 只有预期的 4 个修改与 2 个新增。
- 三条验证命令全部通过。
- 没有改产品代码，没有改 `docs/plans/`。
