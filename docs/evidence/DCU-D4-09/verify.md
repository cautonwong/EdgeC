# DCU D4-09 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D4-09-group`，merge `814f1de9`（tip `50ab80ca`），基线 `cd4adcd0`。

## 结论

VERIFIED。Class 23 总加组（2301..2308）以空掩码对象行 + 只读 ACL 落地：A1..A16 只读（ACL 属性 schema 上限 16，A17/A18 记 HOLD）、方法 1..5。GET 已广告类定义属性 resolver DAR 5；A17/A18 resolver DAR 5、主站 DAR 3；A19 resolver DAR 4、主站 DAR 3；未知 OI 2309 resolver DAR 6、主站 DAR 3。ACTION 方法 1..5 DAR 5 且绝不执行；SET DAR 3。不返回总加值/滑差/轮次。

## 红先绿后（独立复跑）

- 红：测试 commit `d50e3395` 在旧码上 5/45 失败（`120 != 128`、Class 23 行 nil、axdr `-20 != -21`、`107 != 115`、DAR `3 != 5`）。
- 绿：tip `50ab80ca` 45/45；变异守卫 `DCU-D4-09-group-attr16-advertised` 独立复跑 1/1 sensitive。

## 验收

3 连绿，2/2 任务，3+35 passed：4.05/27.75、3.61/22.90、3.60/22.65s。

## 计数

对象 120→128；ACL 107→115（104 手写 + 11 参数宏）；对象列表 5551 B / 115 项，帧后 5573 B < 10240 且 < 从站池 usable 10432 B；115 < 224。

## 对抗评审

NO FINDINGS。评审在既有二进制上复核：表190 A1..A18/方法 1..5、目录 2301..2308、对象/ACL 行、ACL 槽位 16/8 上限、对象列表 gdb 实测 5551 B/115 项、DAR 矩阵（A1..A18 resolver 5、A19/A20 4、未知 OI 6；方法 1..5 DAR 5、6 起 DAR 4）、生产池可用 10432 B 与帧后 5573 B。

## 合并树门禁

- 合并树 build 0 第一方 warning、守卫自检全绿、全量 ctest 367/367、变异守卫 `DCU-D4-09-group-attr16-advertised` 1/1 sensitive，原文 `verifier/merged-*.log`。
- ASAN（Debug+ASAN，`detect_leaks=1`）：`test_dcu_698_object_model`、`test_dcu_698_appconn`、`test_dcu_698_axdr`、`test_driver_dlt698` 全绿、无 sanitizer 报告；`run_fanalyzer.py` 3 known / 0 new。原文 `verifier/asan-*.log`、`verifier/fanalyzer.log`。

## HOLD

- A17/A18 因 ACL 属性 schema 上限 16 未广告（主站 DAR 3）。
- 无总加组配置/总加值/滑差/功控来源。
