# DCU D4-06 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D4-06-file`，merge `e6bd6a94`（tip `4ab86856`），基线 `814f1de9`。

## 结论

VERIFIED。Class 18 文件传输（F000/F001/F002）以空掩码对象行 + 只读 ACL 落地：属性按表 A.11（F000 A1..A5、F001/F002 A1..A4）、方法 1..8；F001 的 9/10（读文件/软件比对）计入 resolver 定义但 ACL 方法槽 8 已满，记 HOLD。GET 已定义属性 resolver 与主站 DAR 5；未定义 resolver DAR 4、主站 DAR 3；未知 OI F003 resolver DAR 6、主站 DAR 3。ACTION 1..8 DAR 5 且绝不传输/校验/激活/回滚；F001 9/10 resolver DAR 5、主站 DAR 3；SET DAR 3。不伪造传输结果。

## 红先绿后（独立复跑）

- 红：测试 commit `df20cf2c` 在旧码上 5/45 失败（`128 != 131`、Class 18 行 nil、axdr `-20 != -21`、`115 != 118`、DAR `3 != 5`、列表 `5573 != 5828`）。
- 绿：tip `4ab86856` 45/45；变异守卫 `DCU-D4-06-file-oi-methods`（F001 9/10 例外改成 `0 && ...`）独立复跑 1/1 sensitive。

## 验收

3 连绿，2/2 任务，3+35 passed：1.94/22.79、3.52/22.55、1.97/22.73s。

## 计数

对象 128→131；ACL 115→118（107 手写 + 11 参数宏）；对象列表 5806 B / 118 项，帧后 5828 B < 10240 且 < 从站池 usable 10432 B；118 < 224。

## 对抗评审

NO FINDINGS。评审复核表175/176/177 与 A.11 映射、per-OI 例外只落在 F001 9/10（F000/F002 方法 9/10 与 F001 11 均 UNDEFINED）、ACL 行与 DAR 优先级、空掩码无伪造 GET、计数 5806/5828 B 与生产池 10432 B；既有 `tests/test_driver_dlt698.c:1305` “106 项”注释已过时（pre-existing，不影响行为）。

## 合并树门禁

- 合并树 build 0 第一方 warning、守卫自检全绿、全量 ctest 367/367、变异守卫 `DCU-D4-06-file-oi-methods` 1/1 sensitive，原文 `verifier/merged-*.log`。
- ASAN（Debug+ASAN，`detect_leaks=1`）：`test_dcu_698_object_model`、`test_dcu_698_appconn`、`test_dcu_698_axdr`、`test_driver_dlt698` 全绿、无 sanitizer 报告；`run_fanalyzer.py` 3 known / 0 new。原文 `verifier/asan-*.log`、`verifier/fanalyzer.log`。

## HOLD

- F001 方法 9/10 因 ACL 每对象方法槽 8 已满未广告（主站 DAR 3）。
- 无文件传输存储/DFU 激活/校验/回滚来源。
