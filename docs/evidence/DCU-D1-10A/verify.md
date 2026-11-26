# DCU D1-10A 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D1-10a-oi-methods`，merge `632783b7`（tip `414298de`），基线 `34aae98e`。

## 结论

VERIFIED。目录夹具从“每类一个 max_method”扩展为同时记录 per-OI 附录 A 方法集合与 7.3 类表自身上限 `max_class_method`；新增 `test_methods_stay_within_the_oi_definition` 拒收“方法号属于同类别的另一个 OI”的广告（例：7000 广告 130，130 属于 7010）。既有按类 max 检查保留，产品代码零改动。变异守卫 `DCU-D1-10A-wrong-oi-method` 登记并自测红/绿。

## 红先绿后（独立复跑）

- 测试 commit `8d16ef95` 单独构建红（夹具表在下一 commit 才生成）；修复/夹具 commit `c9b06aad` 后 `test_dcu_698_catalog` 5/5。
- 变异：把 130 广告到 7000 时，旧按类检查仍过（Class 11 上限 131），新用例在 `tests/test_dcu_698_catalog.c` 红；恢复后绿。独立复跑 `run_mutation_guards.py --only DCU-D1-10A-wrong-oi-method`：red rc=8 → green rc=0，1/1 sensitive。

## 合并树门禁

- `check_dcu_698_catalog.py` 对 `docs/698_parsed.md` 一致；`check_doc_references.py` 0。
- 合并树 build 0 第一方 warning、守卫自检全绿、全量 ctest 367/367、变异守卫在合并树再跑 1/1 sensitive，原文 `verifier/merged-*.log`。
- 无产品代码改动，未跑 ASAN/fanalyzer/验收。

## 残留

- 7.3 类表内部跳号（Class 7/24 方法 3、Class 9 方法 6）仍按 max 放行，既有行为未变；生成器行窗口 6298..9450 未动，均留在 §12.5。
