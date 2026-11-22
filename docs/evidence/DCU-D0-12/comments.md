# DCU-D0-12 注释审计

范围：D0-12 的 diff（`acceptance/run.sh`、`ci/scripts/check_acceptance_tasks.py`、`run_guard_selftest.py`、夹具）。

## 结果

删除 0 处。

## 保留

三处说明（`run.sh` 的状态文件注释、收尾检查注释、checker docstring）都陈述平台 gate 只统计 `summary.json` 的约束；审计对照了平台源码 `gate.py` 与 `cli.py`，确认任务文件加载失败不产生 summary，属于代码本身看不出的失败模式。

## 修正

把 “产品没起来 / product crashed at load time” 收紧为 “任务文件加载失败 / task file that fails to load”：产品没就绪仍会在 `finally` 里写 summary（reason=platform_error），真正无 summary 的是任务文件加载失败。判据不变，只改例子。
