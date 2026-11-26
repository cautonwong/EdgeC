# DCU D5-06 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D5-06-class17-display`，合并 tip `af263dc7`（feat/dcu merge `f25d390f`），基线 `feat/dcu c2eaa5ea`。

## 结论

VERIFIED。F300/F301 以空属性掩码单例对象行和只读 ACL 登记。无显示配置来源时 GET 按类定义回 DAR 5，不伪造轮显表或显示参数。计划原“GET F300 空轮显表”的出口改为 DAR 5 诚实骨架，显示配置 producer 缺口记入计划 §12.5。

## 红先绿后（独立复跑）

- 测试 commit `49dc087a`：`ctest -R 'dcu|dlt698'` 5/45 失败（test_dcu_698_object_model、test_dcu_698_axdr、test_dcu_698_scheme_mgmt、test_driver_dlt698、test_dcu_698_appconn），原文 `verifier/red-ctest.log`。
- 修复 tip `af263dc7`：同一命令 45/45 绿，59.96s，原文 `verifier/green-ctest.log`。

## 验收（COVERAGE=ON）

3 连绿，2/2 任务，35 passed，22.76/22.48/22.69s。原文 `verifier/accept-{1,2,3}.log`。

## 运行时与门禁（合并树 `feat/dcu` `f25d390f`）

- ASAN（Debug+ASAN，`detect_leaks=1`）目标集：test_dcu_698_object_model、test_dcu_698_appconn、test_dcu_698_axdr、test_driver_dlt698、test_dcu_698_scheme_mgmt 全绿，无 sanitizer 报告。原文 `verifier/asan-*.log`。
- `run_guard_selftest.py` 全绿（exit 0）。
- 全量 ctest 367/367，79.78s（排除 `app_platform_e2e`），原文 `verifier/merged-ctest.log`。
- `check_build_warnings.py` 0 新增，原文 `verifier/warnings.log`。
- `run_fanalyzer.py`：3 known / 0 new，原文 `verifier/fanalyzer.log`。
- `check_dcu_698_catalog.py`、`check_doc_references.py` 由 dev 跑绿；合并后守卫自检覆盖文档引用。

## 语义与计数

- 对象表 103 行，ACL 89 项（78 手写 + 11 参数宏），对象列表 3141 B，会话 ACL 1424 B，均在 D1-21 的 224/10240/4096 内。
- ACL 属性 2..4 只读、方法 1..6；A1 不广告；SET 回 DAR 3，方法与未广告属性不伪装成功。
- 无显示配置 producer；对象 mask 为空，GET 回 DAR 5。producer 缺口见计划 §12.5。
