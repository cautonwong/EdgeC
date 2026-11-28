# DCU-FORMAL-07 独立验证报告（CBMC 5.12 SSA 超时阻塞记录）

## 1. 实测事实
`dcu-formal-07` 在 `/tmp/dcu-FORMAL-07` 基于 `DCU-REFACTOR-01` 迭代版 `dlt698_decode_data` 实测 CBMC 5.12，在 `timeout 60` / `AUX_LIMIT=60` 预算内均卡在 `converting SSA` 阶段超时（`rc=124`）：
- 单次 1 轮常量调用（`0xFF`）：`0.74s`（`0 of 54 failed`）。
- `proof`（`FORMAL_MAX_LEN=3 --unwind 4 --unwinding-assertions`）：`61.1s` 超时（`converting SSA`，52,375 steps，1,897 VCCs）。
- `proof`（`FORMAL_MAX_LEN=5 --unwind 6`）：`82.6s` 超时（`converting SSA`，93,565 steps，3,405 VCCs）。
- `cover`（`FORMAL_MAX_LEN=5 --unwind 6`）：`65.7s` 超时（`converting SSA`，89,687 steps，2 VCCs）。
- `function`（`test_const_depth9` 19 字节常量 9 层 `ARRAY`）：`60.27s` 超时（`converting SSA`，74,244 steps，1,798 VCCs）。

## 2. 根因分析
`dlt698_decode_data` 在 `for (;;)` 与 `while (depth > 0)` 循环内通过 `dlt698_data_t *cur` 写入 `g_dlt698_item_pool[64]`（64 项约 80 B 联合体，含 `str.buf[64]` 与 `memcpy`），CBMC 5.12 的 `converting SSA` 在超过 1 次迭代后出现超线性膨胀。

## 3. 结论
- **状态**：`BLOCKED`（不放宽断言、不裁剪 `FORMAL_MAX_LEN` 伪造通过）。
- **登记**：已记入 `docs/plans/2026-11-21-dcu-698-production-plan.md` §12.5，后续可通过对 `g_dlt698_item_pool` 抽象建模或独立边界子函数拆分后再证。
