# DCU D4-07 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D4-07-esam`，merge `cd4adcd0`（tip `b0cd3b74`），基线 `88e1e550`。

## 结论

VERIFIED。Class 21 只注册目录事实 F100（F101 是 Class 8 安全模式参数，本单按目录纠正任务单口径）：F100 空掩码对象行 + 只读 ACL（A1..A15、方法 1..8；9..12 因每对象方法槽 8 记 HOLD）。产品无 ESAM 硬件/会话来源，软件 ESAM 模拟只保留 `dcu_security_*` C API：698 侧删除原方法 1 的复位处理器（`dcu_698_class_security.c`），ACTION 不再改动会话、密钥或认证态，方法 1..12 走 resolver 回 DAR 5，主站未广告的 9..12 先 DAR 3；GET A1..A15 回 DAR 5，不返回序列号/版本/MAC/密钥；SET 回 DAR 3。

## DAR 两层口径（评审 F1 的裁决）

对抗评审指出主站对 F100 A16 / F101 A2 看到 DAR 3，而 resolver 层是 4/6，同一改动里两套测试看似矛盾。裁决：这不是缺陷。既有产品口径是**主站请求先过 ACL**（未广告属性/方法一律 DAR 3），resolver 层的 5/4/6 只在通过 ACL 后可见；D1-02 已记录“ACL 3 优先”。已在 DEVIATIONS Class 21 行写清两层，并要求后续类族沿用同一写法。

## 红先绿后（独立复跑）

- 红：测试 commit `5dcbe1a3` 在旧码上 6/45 失败（advanced `0 != -21`、掩码不等、axdr `-20 != -21`、计数 `106 != 107`、DAR `3 != 5`）；原文 `verifier/red-ctest.log`。
- 绿：tip `b0cd3b74` 45/45；原文 `verifier/green-ctest.log`。
- 变异守卫 `DCU-D4-07-esam-attr15-advertised`（ACL 属性 15→14）独立复跑 1/1 sensitive。

## 验收

3 连绿，2/2 任务，3+35 passed：1.99/22.60、1.90/22.87、1.99/22.67s。原文 `verifier/accept-*.log`。

## 合并树门禁（`cd4adcd0`）

- 合并树 build 0 第一方 warning、守卫自检全绿、全量 ctest 367/367、变异守卫 `DCU-D4-07-esam-attr15-advertised` 1/1 sensitive，原文 `verifier/merged-*.log`。
- ASAN（Debug+ASAN，`detect_leaks=1`）：`test_driver_dlt698`、`test_dcu_advanced_modules`、`test_dcu_698_object_model`、`test_dcu_698_appconn`、`test_dcu_698_axdr` 全绿、无 sanitizer 报告；`run_fanalyzer.py` 3 known / 0 new。原文 `verifier/asan-*.log`、`verifier/fanalyzer.log`。

## 计数

对象 120 行（F100 原已存在，本单改掩码）；ACL 106→107（96 手写 + 11 参数宏）；对象列表 4455 B / 107 项，帧后 4477 B < 从站池 usable 10432 B。

## HOLD

- 方法 9..12 因 ACL 每对象方法槽 8 未广告（主站 DAR 3）。
- 无 ESAM 硬件/会话端口；有来源后补 A2/A3 值与处理器。
