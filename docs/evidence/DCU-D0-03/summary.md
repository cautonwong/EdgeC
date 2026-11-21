# DCU-D0-03 证据包：DCU stdout 行缓冲 + 产品级守卫

- 分支：`dcu/D0-03-stdout-guard`
- 起点：`d0eb1f43`（main）
- 提交：`fix(DCU-D0-03): line-buffer DCU stdout and generalize the product guard`

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `cmd/dcu/main.c` | `setvbuf(stdout, NULL, _IONBF, 0)` → `_IOLBF`（D-06 根因修复）。 |
| `cmd/app-host/main.c`、`cmd/edge_mcp_server/main.c`、`cmd/edgeos-launcher/main.c`、`cmd/nexus_lab/main.c`、`cmd/smart_box/main.c` | 守卫扩到 `cmd/*/main.c` 后，这些产品入口也补上 `setvbuf(stdout, NULL, _IOLBF, 0)`（smart_box 另补 `<stdio.h>`）。 |
| `ci/scripts/check_iems_stdout_line_buffered.py` | 从硬编码 iEMS 改为扫描 `cmd/*/main.c`（每个必须行缓冲）与 `acceptance/*/*_product.py`（不得出现 `stdbuf`）。保留文件名，避免 `run_guard_selftest.py` / `AGENTS.md` 无谓漂移。 |
| `tests/guards/fixtures/good_iems_stdout_line_buffer/` | 新增 `cmd/dcu/main.c` 与 `acceptance/dcu/dcu_product.py`；正例现在含 iEMS 与 DCU 两个产品。 |
| `tests/guards/fixtures/bad_iems_stdout_line_buffer/` | 新增 `cmd/dcu/main.c`（`_IONBF`）与 dcu 验收壳；保留原有 iEMS 缺 setvbuf 与 stdbuf 负例。 |
| `ci/baseline/composition_root_budget.txt` | 跟随实际行数：edge_mcp_server 183→184、nexus_lab 156→157、smart_box 90→92；新增注释说明是 D0-03 行缓冲调用。 |

未改 `modules/`、`foundation/`、`adapters/`。

## 为什么

计划 3.2 节 D-06 / 第 8 节 D0-03：DCU 产品 stdout 用 `_IONBF`，违反仓库“产品 stdout 必须行缓冲”的规则；守卫只扫 iEMS，其他产品回潮无人发现。行缓冲是验收读状态行、避免用 `stdbuf` 掩盖缺陷的前提。

## 完成定义逐项

- [x] `cmd/dcu/main.c` 改 `_IOLBF`。
- [x] 守卫扫描 `cmd/*/main.c` 与 `acceptance/*/*_product.py`；缺 `_IOLBF` 或出现 `stdbuf` 都报错。
- [x] 正反夹具覆盖“至少一个产品用 `_IONBF` 会红”：bad 夹具的 `cmd/dcu/main.c` 用 `_IONBF`，守卫报红；good 夹具含 iEMS + DCU，守卫通过。
- [x] 负例自证：临时把 DCU 改回 `_IONBF`，live 守卫变红；恢复后变绿（见 `tests.log` A/B 段）。
- [x] `check_iems_stdout_line_buffered.py --root .` 通过。
- [x] `run_guard_selftest.py --root .` 通过（含正反夹具与 live 检查）。
- [x] `cmake --build build -j8` 通过（0 warning），`ctest -R dcu` 19/19 通过。
- [x] `./test.sh --omnis dcu` 通过，状态行未受缓冲改动影响。

## 说明

- 守卫扩到 `cmd/*/main.c` 后，仓库里另有 5 个产品入口原先没有 `setvbuf`。按“每个产品 main 必须行缓冲”的断言，本次一并补齐；随之 `composition_root_budget` 的行数基线上调 3 个文件（+1/+1/+2），这是该守卫的既有流程，未放宽任何断言。
- 6 个 `main.c` 的 clang-format 差异是既有的（改前/改后错误行数一致），未做整文件重排以免扩大 diff。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `python3 ci/scripts/check_iems_stdout_line_buffered.py --root .` | exit 0 |
| 同上，good / bad 夹具 | good exit 0；bad exit 1（dcu `_IONBF`、iems 缺 setvbuf、iems 壳 stdbuf 全部命中） |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0 |
| `cmake --build build -j8` | exit 0，0 warning |
| `cd build && ctest --output-on-failure -R dcu` | 19/19 通过 |
| `./test.sh --omnis dcu` | dcu-acceptance 2 passed、dcu-northbound 4 passed、退出码 0 |

## 证据文件

- `tests.log`：构建、守卫 live/夹具、A/B 红绿、guard selftest 摘要、ctest
- `runtime.log`：`./test.sh --omnis dcu` 完整输出
- `diffstat.txt`：代码提交的 `git show --stat`
