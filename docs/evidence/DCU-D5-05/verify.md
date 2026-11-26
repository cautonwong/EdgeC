# DCU D5-05 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D5-05-class14-16-stats`，合并 tip `a61e7e61`，基线 `feat/dcu 383c91cc`。

## 结论

VERIFIED。Class 14/15/16 的 15 个 OI 以空属性掩码对象行和只读 ACL 登记；无统计生产者时 GET 按类定义回 DAR 5，不伪造值。原计划“GET 2100/2110/2120 返回值”的出口改为 DAR 5 诚实骨架，生产者缺口记入计划 §12.5。

## 红先绿后（独立复跑）

- 测试 commit `395dc6b5`：`ctest -R 'dcu|dlt698'` 5/45 失败（test_dcu_698_object_model、test_dcu_698_axdr、test_dcu_698_scheme_mgmt、test_driver_dlt698、test_dcu_698_appconn），原文 `verifier/red-ctest.log`。
- 修复 tip `a61e7e61`：同一命令 45/45 绿，48.57s，原文 `verifier/green-ctest.log`。

## 验收（COVERAGE=ON）

3 连绿，2/2 任务，35 passed，27.15/22.79/22.53s。原文 `verifier/accept-{1,2,3}.log`。

## 运行时与门禁（合并树 `feat/dcu` `a61e7e61`）

- ASAN（Debug+ASAN，`detect_leaks=1`）目标集：test_dcu_698_object_model、test_dcu_698_appconn、test_dcu_698_axdr、test_driver_dlt698、test_dcu_698_scheme_mgmt 全绿，无 sanitizer 报告。原文 `verifier/asan-*.log`。
- `run_guard_selftest.py` 全绿（exit 0）。
- 全量 ctest 367/367，102.88s（排除 `app_platform_e2e`），原文 `verifier/full-ctest.log`。
- `check_build_warnings.py`（coverage clean build log）0 新增，原文 `verifier/warnings.log`。
- `run_fanalyzer.py`：3 known / 0 new。
- `check_dcu_698_catalog.py`、`check_doc_references.py` 由 dev 跑绿；合并后守卫自检覆盖文档引用。

## 语义与计数

- 对象表 101 行，ACL 87 项（76 手写 + 11 参数宏），对象列表 3011 B，会话 ACL 1392 B，均在 D1-21 的 224/10240/4096 内。
- Class 16 `max_method` 从 2 改 4，与目录夹具和方法 1..4 对齐。
- 无统计生产者；对象 mask 为空，GET 回 DAR 5，方法与 SET 不伪装成功。生产者缺口见计划 §12.5。
