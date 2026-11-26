# DCU D5-03 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D5-03-class11-collection`，合并 tip `092304cd`（feat/dcu merge `0066b073`），基线 `feat/dcu f25d390f`。

## 结论

VERIFIED。7000 与 7010 以 Class 11 对象行和只读 ACL 登记。7000 只有方法 127..129，7010 有只读 A2 与方法 127..130。per-OI 方法表承载实例方法，Class 11 类上限仍为 2，其他集合 OI 不会被误广告。无文件/脚本集合存储时，7010 A2 与 7000 方法按类定义回 DAR 5，不伪造集合。

## 红先绿后（独立复跑）

- 测试 commit `8146f199`：`ctest -R 'dcu|dlt698'` 5/45 失败（test_dcu_698_object_model、test_dcu_698_axdr、test_dcu_698_scheme_mgmt、test_driver_dlt698、test_dcu_698_appconn），原文 `verifier/red-ctest.log`。
- 最终 tip `092304cd`：同一命令 45/45 绿，28.75s，原文 `verifier/green-ctest.log`。

## 验收（COVERAGE=ON）

3 连绿，2/2 任务，35 passed，22.60/22.67/22.62s。原文 `verifier/accept-{1,2,3}.log`。

## 运行时与门禁（合并树 `feat/dcu` `0066b073`）

- ASAN（Debug+ASAN，`detect_leaks=1`）目标集：test_dcu_698_object_model、test_dcu_698_appconn、test_dcu_698_axdr、test_driver_dlt698、test_dcu_698_scheme_mgmt 全绿，无 sanitizer 报告。原文 `verifier/asan-*.log`。
- `run_guard_selftest.py` 全绿（exit 0）。
- 全量 ctest 367/367，65.74s（排除 `app_platform_e2e`），原文 `verifier/merged-ctest.log`。
- `check_build_warnings.py` 0 新增，原文 `verifier/warnings.log`。
- `run_fanalyzer.py`：3 known / 0 new，原文 `verifier/fanalyzer.log`。
- `check_dcu_698_catalog.py`、`check_doc_references.py` 由 dev 跑绿；合并后守卫自检覆盖文档引用。

## 语义与计数

- 对象表 105 行，ACL 91 项（80 手写 + 11 参数宏），对象列表 3211 B，会话 ACL 1456 B，均在 D1-21 的 224/10240/4096 内。
- 7000 无属性、方法 127..129；7010 A2 只读、方法 127..130；无写权限；SET 与方法不伪装成功。
- 计划原出口“GET 7000 返回集合元素”与 A.8 冲突，按标准改为 7010 A2 DAR 5 + 7000 方法 DAR 5 骨架；集合 producer 缺口见计划 §12.5。
