# DCU-D0-12 验收任务集合门禁

基线：分支 `dcu/D0-12-acceptance-gate`，工作树 `/tmp/dcu-d1-11`，`1ca7130a` 起步，rebase 到最新 main `e0fa0cd9`（D1-11/D1-18e 已合入）。diff 范围只含 `acceptance/run.sh`、检查脚本、守卫夹具与本目录；不触碰 verifier 的 `docs/evidence/DCU-D6-08/` 与 `docs/plans/`。

## 假绿路径

平台 `gate.py` 只统计有 `summary.json` 的任务。D1-18e 的任务文件加载失败，`run_task.py` 退出 4，没有 summary，于是 `./test.sh --omnis dcu` 打印 `全部合格:1 个验收任务` 并返回 0 —— 一个从未加载的任务从门禁里消失了。

## 修复

`acceptance/run.sh` 在跑每个任务时把 `产品<TAB>任务<TAB>退出码` 写进 `reports/acceptance/tasks.status`。平台的 gate 跑完后，再调用仓库内的 `ci/scripts/check_acceptance_tasks.py`：

- 从 `acceptance/<product>/tasks/*.yaml` 枚举本次声明的任务集合（按 `--products` 限定本轮产品，避免把没跑的产品算进来）；
- 状态文件里缺该任务，或退出码非 0，就逐条列出并以非 0 退出；
- 平台 gate 已经非 0 时保留它的退出码，否则用任务集合检查的退出码。

平台与产品都不用改；检查逻辑留在仓库内。

## 守卫

`run_guard_selftest.py` 注册了两组正反夹具：

- `tests/guards/fixtures/acceptance_tasks/good`：2 个任务、两行退出码 0 → 通过；
- `bad_missing`：只记录 1 个任务 → 失败并点名 `dcu/dcu_northbound: did not run`；
- `bad_nonzero`：北向退出码 4 → 失败并点名 `dcu/dcu_northbound: exit 4`。

## 负例 A/B（wrapper 级）

临时放入 `acceptance/dcu/tasks/zz_broken.yaml`（YAML 语法错误），运行 `./test.sh --omnis dcu`：

- 平台仍然打印 `全部合格:2 个验收任务`（这正是假绿）；
- 新检查打印 `dcu/zz_broken: exit 4`，wrapper 退出 1；
- 临时文件随后删除。

## 正例

正常一轮：`dcu-acceptance` 与 `dcu-northbound` 退出码均 0，检查打印 `2/2 expected tasks exited 0`，wrapper 退出 0。

## 门禁

- 干净构建 + `check_build_warnings.py`：0 warning
- `ctest -j`：338/338 通过
- ASAN：3/3 相关目标通过
- `run_guard_selftest.py`：全部通过（含两组新夹具）
- 验收正例两次：两个任务均退出 0，`ACCEPTANCE_TASKS` 2/2；北向 17 条用例（D1-18e 合入后）
- `git diff --check`：干净
