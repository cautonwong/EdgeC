# DCU-D0-07 把 698 目录的 --check 变成 CI 门禁

## 做了什么

`scripts/extract_dcu_698_catalog.py --check` 之前只能手动跑，没有门禁、没有正反夹具。本次补上三件事：生成器支持 `--root`，新增门禁脚本，门禁进自检与 CI。

## 生成器

`scripts/extract_dcu_698_catalog.py` 增加 `--root <dir>`，默认仍是脚本所在的仓库根。解析源与两份产物的路径由 `catalog_paths(root)` 统一给出，不再写死。

这样门禁能对临时目录里的真实文档副本运行。生成器自带的断言（26 个类、至少 400 行 OI）在副本上照常生效。

## 门禁

`ci/scripts/check_dcu_698_catalog.py`：

- 接受 `--root`，调用生成器的 `--check`，把 stdout/stderr 原样带出。
- 通过时打印 `catalog matches docs/698_parsed.md`，失败时提示重新生成并列出 stale 文件。
- 生成器始终从本仓库定位（`ci/scripts/` 的上级），`--root` 只决定比较哪份文档。临时目录因此只需包含 `docs/`。
- 比对逻辑只有一处实现，就在生成器里；门禁只做包装与输出整理。

## 自检注册

`ci/scripts/run_guard_selftest.py`：

- `TEST_MATRIX` 增加 `check_dcu_698_catalog.py`，正例是仓库根。
- 新增 `[CATALOG_GATE]` 段：把真实的 `docs/698_parsed.md` 与 `docs/design/DCU_698_OI_CATALOG.{md,json}` 复制到 `tempfile.TemporaryDirectory()`，先确认未改动的副本通过，再把目录头一行改成 `Classes: 99.`，确认门禁拒收；临时目录随 `with` 块删除，不写仓库。
- 挑这个改法的原因：它是字节级比对，任何一行变化都应失败；同时不依赖极小夹具，避免绕开生成器的 26 类与 400 行断言。若日后目录头格式变化，`corrupted == text` 会让夹具直接报错，不会静默失效。
- `LIVE` 列表加入该门禁，随其他守卫在活树上跑。

## CI

`.github/workflows/ci.yml` 的 `Run Architecture & Anti-Entropy Gates (x86_64 only)` 步骤在 `run_guard_selftest.py` 之后加一行：

```
python3 ci/scripts/check_dcu_698_catalog.py --root .
```

`ci/scripts/run_arch_gates.sh` 同步加同一行，`check_ci_gate_parity.py` 从 10 条门变为 11 条，本地与 CI 口径一致。

## 验证

| 命令 | 结果 |
| --- | --- |
| `run_guard_selftest.py --root .` | 全部通过；目录门禁三条断言均绿（矩阵正例、副本通过、副本拒收） |
| `check_ci_gate_parity.py --root .` | 通过，11 条门禁 |
| 故意改坏仓库目录 | 门禁退出 1；恢复后退出 0 |
| 临时副本负例 | 未改动副本通过、改坏后拒收 |

完整输出见 `tests.log`。

## 改动文件

- `scripts/extract_dcu_698_catalog.py`（`--root`）
- `ci/scripts/check_dcu_698_catalog.py`（新增）
- `ci/scripts/run_guard_selftest.py`（矩阵条目、动态夹具、LIVE 列表）
- `.github/workflows/ci.yml`、`ci/scripts/run_arch_gates.sh`（CI 步骤与本地镜像）
- `docs/evidence/DCU-D0-07/`

未改产品代码，未改 `docs/plans/`。
