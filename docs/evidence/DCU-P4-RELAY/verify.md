# DCU P4 继电器映射验证记录（dcu-verifier，2026-11-26）

分支 `dcu/P4-relay-map`，merge `5bcfdeda`（tip `4125fdd2`，证据提交 `79486194`），基线 `efd887a0`。

## 结论

VERIFIED。OI 8000 方法 129/130/131 的继电器 OAD 经产品点位表接口解析（`dcu_relay_map_t`，由 `dcu_node` 聚合根拥有，`dcu_node_set_relay` 注入）。空表仍回 DAR 2；命中后真实 DO 执行未接线，仍回 DAR 5，不返回成功。表随节点存储释放时解绑，避免悬挂单例。

## 红先绿后（独立复跑）

- 红：`red-build.log`，测试提交引入 `dcu_relay_map.h` 前编译失败。
- 绿：`green-build.log` 构建通过；`green-ctest.log` `ctest -R 'dcu|dlt698'` 45/45，105.90s。
- 行为敏感度由两条变异钉住（`mutation-a.log`、`mutation-b.log`）：命中改回 DAR 2 时 2 例红；空表改 DAR 5 时 5 例红（含 129/130/131 既有 DAR 2 用例）。

## 验收（分支 `build-cov`，Release，未开 COVERAGE 旗标）

3 连绿，2/2 任务，35 passed，18.23/22.74/23.09s。原文 `verifier/accept-{1,2,3}.log`。

## 合并树门禁（`5bcfdeda`）

- 守卫自检全绿，原文 `verifier/merged-guards.log`。
- 全量 ctest 367/367，161.69s；构建 warning 0。原文 `verifier/merged-ctest.log`、`verifier/warnings.log`。
- ASAN（Debug+ASAN，`detect_leaks=1`）：`test_dcu_698_axdr` 47/47、`test_dcu_node` 5/5，无 sanitizer 报告。原文 `verifier/asan-*.log`。
- `run_fanalyzer.py`：3 known / 0 new，原文 `verifier/fanalyzer.log`。

## 语义

- 映射接口是消费者定义的端口（控制类只查表，不认识点位表存储）；未命中与未接 DO 是两种不同 DAR，主站可区分“没配点”与“配了点但产品还不执行”。
- 点位表数值仍需产品给出（§12.5）；真实 DO 执行另立任务。
