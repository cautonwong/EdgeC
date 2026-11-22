# DCU-D0-01 重写 README

## 做了什么

`cmd/dcu/README.md` 从 307 行改为 83 行，按当前代码重写。

旧文的问题：写的是 4 文件架构（`dcu_inventory`、`dcu_poller`、`dcu_storage`、`main.c`）与一批函数原型，实际已有 31 个 `.c` 与 30 个 `.h`；对象表、变量处理器、698 从站适配、A-XDR 端口、事件与业务引擎都没写；TDD 任务表与计划重复。

## 保留的业务背景

五位一体（计量、采集、处理、控制、通信）作为设计理由保留。三条现场约束保留，数字按代码改正：

| 约束 | 旧文说法 | 代码现状 |
| --- | --- | --- |
| SQLite 批量落盘 | 每 50 条或 10 秒 | `STORAGE_CACHE_MAX` 为 128（`dcu_storage.h:12`），定时器 10 秒（`dcu_storage.c:76`） |
| 通道熔断 | 连续 3 次超时 | 默认阈值 5（`dcu_poller.c:19`），触发在 `:205-214` |
| 掉电落盘 | 描述 | `main.c:188-190` 注册信号，`:49-60` 落盘 |
| Flash 磨损降低 95% | 有数字 | 无可核对来源，删掉数字，只留机制 |

## 结构

1. 一句话定位与当前状态。
2. 业务背景与三条工程约束。
3. 模块图：九组职责与代表文件，不逐文件列。
4. 698 覆盖现状表，附两处已知行为差异与 A-XDR 端口状态。
5. 运行与验收入口、环境变量表。
6. 权威文档链接，只链接不复述。

## 删除的内容

- 4 文件目录树。
- 重复头文件的函数原型（`production_meter_t`、`dcu_inventory_init`、`dcu_storage_*` 等）。
- 6 个 Task 的 TDD 任务表（与计划重复）。
- 类表与数据库物理模型（属 `DCU_698_OBJECT_MODEL.md` 与计划）。

## 验证

| 命令 | 结果 |
| --- | --- |
| `git diff --check` | 通过 |
| `python3 -I scripts/extract_dcu_698_catalog.py --check` | 通过 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | 全部通过 |

输出见 `tests.log`。

## 改动范围

`cmd/dcu/README.md` 与 `docs/evidence/DCU-D0-01/`。未改代码，未改 `docs/plans/`。
