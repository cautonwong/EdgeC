# DCU D5-04 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D5-04-class12-partial`，合并 tip `7c9accd0`（feat/dcu merge `efd887a0`），基线 `feat/dcu 197d98b9`。

## 结论

VERIFIED。Class 12 脉冲计量的 2401 以单例空掩码对象行登记，ACL 只放属性 1..15 只读与方法 1..4。16/17 按类定义表157 视为已定义但 ACL 不广告，等 PDF 所有者裁决；18/19 超出 ACL 属性 schema。无脉冲 producer，GET 按类定义回 DAR 5，不伪造脉冲量。

## 红先绿后（独立复跑）

- 测试 commit `6ae69e78`：`ctest -R 'dcu|dlt698'` 5/45 失败（test_dcu_698_object_model、test_dcu_698_axdr、test_dcu_698_scheme_mgmt、test_driver_dlt698、test_dcu_698_appconn），原文 `verifier/red-ctest.log`。
- 修复 tip `7c9accd0`：同一命令 45/45 绿，76.99s，原文 `verifier/green-ctest.log`。

## 验收（COVERAGE=ON）

3 连绿，2/2 任务，35 passed，23.19/22.28/17.60s。原文 `verifier/accept-{1,2,3}.log`。

## 合并树门禁

- 守卫自检全绿；全量 ctest 367/367，150.97s；warning 0。原文 `verifier/merged-ctest.log`、`warnings.log`。
- ASAN（Debug+ASAN，`detect_leaks=1`）目标集：test_dcu_698_object_model、test_dcu_698_appconn、test_dcu_698_axdr、test_driver_dlt698、test_dcu_698_scheme_mgmt 全绿，无 sanitizer 报告。原文 `verifier/asan-*.log`。
- `run_fanalyzer.py`：3 known / 0 new，原文 `verifier/fanalyzer.log`。

## 语义与计数

- 对象表 106 行，ACL 92 项（81 手写 + 11 参数宏），对象列表 3336 B，会话 ACL 1472 B，均在 D1-21 的 224/10240/4096 内。
- 2401 GET：A1..A19 按类定义回 DAR 5（ACL 只广告 A1..A15，主站对 A16..A19 先 DAR 3）；A20 回 DAR 4；未知 OI 回 DAR 6；SET DAR 3；方法 1..4 DAR 5、方法 5 DAR 4。
- 16/17 归 HOLD，18/19 超 schema；2402-2408 未注册，脉冲 producer 缺口见计划 §12.5。
