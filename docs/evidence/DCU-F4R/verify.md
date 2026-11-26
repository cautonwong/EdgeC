# DCU F4R 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/F4-scheme-identity`，合并 tip `60a13c85`（feat/dcu merge `168ab797`），基线 `feat/dcu 0066b073`。

## 结论

VERIFIED。方案内容代际 `content_gen` 纳入执行身份。同一 `scheme_id` 下 CSD 内容变化时递增代际，只重开引用该方案的 ACTIVE 任务；旧 served 位图不会按新 csd_list 的下标解释。内容相同不重开。

## 红先绿后（独立复跑）

- 测试 commit `5195d2c8`：`test_dcu_poller` 35 例中 1 红（`test_scheme_content_change_reopens_referencing_execution`），34 绿，原文 `verifier/red.log`。
- 修复 tip `60a13c85`：同一命令 35/35 绿，`ctest -R 'dcu|dlt698'` 45/45。原文 `verifier/green.log`、`verifier/broad.log`。

## 验收（COVERAGE=ON）

3 连绿，2/2 任务，35 passed，22.85/22.80/22.79s。原文 `verifier/accept-{1,2,3}.log`。

## 合并树门禁

合并后守卫、全量 ctest、ASAN、fanalyzer、warning 在最终 `feat/dcu` 树上统一跑，结果见交接与计划 §12.3。本单改动 `dcu_poller.c/.h` 与测试，需特别关注 `dcu_scheme_t` 布局变化后的全量构建。

## 已知限制

- HOLD：`dcu_poller_get_scheme` 仍返回可变指针。生产 store 的 Set_CSD 只改未被活动任务引用的方案，单线程事件循环内校验与修改之间无并发窗口；彻底收口建议改 const 或让 store 走 `upsert_scheme`，另立任务。
- `dcu_poller_upsert_scheme` 有 `csd_count` 越界检查，未加 `ms_count` 检查；当前 store 路径不填 ms 字段，属潜伏边界，记 HOLD。
- 未改 `dcu_scheme_store.c` 与 `dcu_router.h`。
