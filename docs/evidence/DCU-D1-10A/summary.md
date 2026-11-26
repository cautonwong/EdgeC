# DCU-D1-10A 698 目录按 OI 校验 A.7 方法集合

基线 `34aae98e`（`feat/dcu` tip），分支 `dcu/D1-10a-oi-methods`，工作树
`/tmp/edgec-land/d1-10a-methods`。

## 做了什么

关闭 `docs/plans/2026-11-21-dcu-698-production-plan.md` §12.5 的
“A.7 方法上限按类聚合”缺口。旧夹具把某个 OI 的附录 A 方法号并进该类的
`max_method`，范围检查对同类其它 OI 也放行；例如 Class 11 的 7000 只定义
127-129，7010 定义 127-130，7000 广告 130 时旧检查仍通过。

生成器 `scripts/extract_dcu_698_catalog.py` 现在把附录 A 的按 OI 方法集合
写进三处生成物：

- `tests/fixtures/dcu_698_catalog_expected.h`：
  - 新增 `dcu698_catalog_oi_methods[]`，一行一个 `{oi, method}`，只列附录 A
    的 OI 行方法号（31 个 OI，86 行）。
  - 类表增加 `max_class_method`：仅 7.3 类表自身的方法上限。既有
    `max_method`（7.3 与该类 OI 附录 A 方法的按类最大值）保留，既有类级检查
    不动。
- `docs/design/DCU_698_OI_CATALOG.json`：新增顶层 `oi_methods`，按 OI 记录
  A.7 方法数组；`classes` 与 `ois` 与改前一致（已用 JSON 对比确认）。
- `docs/design/DCU_698_OI_CATALOG.md`：新增 “Appendix A OI methods” 一节，
  列表与 JSON/夹具同源。

`tests/test_dcu_698_catalog.c` 新增
`test_methods_stay_within_the_oi_definition`：对象表广告的方法号若大于该类
7.3 类表自身上限，必须出现在该 OI 自己的附录 A 方法集合里；否则测试失败。
既有 `test_masks_stay_within_the_class_definition`（按类 `max_method` 的
范围检查）保留。

`ci/mutation_guards.tsv` 新增 `DCU-D1-10A-wrong-oi-method`：把方法 130
广告到 7000（7010 的方法），要求 `test_dcu_698_catalog` 变红，恢复后变绿。

## 夹具内容与断言

1. 对象表每个 OI 都在目录中，且 `class_id` 相等（既有）。
2. `attr_mask` / `method_mask` 不超过类级上限（既有）。
3. 方法号大于 7.3 类表自身上限时，必须在该 OI 的附录 A 集合内（新增）。
4. 对象表无重复 OI（既有）。

## 红先绿后

提交序列：`8d16ef95` 先落测试与变异守卫（夹具能力未生成，按设计构建红）；
`c9b06aad` 补生成器与夹具（构建绿）。

- 干净树：`test_dcu_698_catalog` 5/5，退出 0。
- 注入缺陷：在 7000 的对象行加 `.method_mask = {[BYTE(130)] = BIT(130)}`。
  130 小于 Class 11 的按类 `max_method` 131，所以既有类级测试仍通过；
  新增按 OI 测试在第 81 行失败，`4/5`，退出 1。
- 恢复对象表后重新编译：5/5，退出 0。
- 变异守卫 runner：`DCU-D1-10A-wrong-oi-method` 变异时 `test rc=8`，恢复后
  `test rc=0`，退出 0；`--selftest` 通过。

注释审查：`pstack:comment-sicko` 只读审查，5 条 KEEP、1 条 MUST KILL
（测试内联注释）已删除；见 `comments.md`。

## 验证

| 项 | 结果 |
| --- | --- |
| 构建（Release，`-DENABLE_WEB_BUILD=OFF`） | 退出 0，0 告警 |
| `test_dcu_698_catalog` | 5/5，退出 0 |
| `ctest -R 'dcu\|catalog'` | 38/38，49.98 s |
| `python3 ci/scripts/check_dcu_698_catalog.py` | 目录与生成物一致 |
| `python3 ci/scripts/check_doc_references.py --root .` | 通过 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | 全绿 |
| `python3 ci/scripts/run_mutation_guards.py --selftest` | 决策逻辑通过 |
| `python3 ci/scripts/run_mutation_guards.py --build build --only DCU-D1-10A-wrong-oi-method` | 红→绿通过 |
| `git diff --check` | 干净 |

## 发现与未覆盖

- 对象表没有本缺口暴露的真实错配：现有 31 个带附录 A 方法的 OI 中，对象表
  广告的方法号都在各自集合内；因此产品代码零改动。
- 7.3 类表内部的跳号（Class 7 与 24 的方法 3、Class 9 的方法 6）仍按既有
  `max_method` 范围放行，不在本缺口范围（缺口是附录 A 方法跨 OI 泄漏）。
- 观察：产品 `k_class_max_method` 的 Class 19 上限是 2，而 7.3.19 定义方法
  1..6；当前没有 Class 19 对象广告方法 3..6，新检查未红。该差异属产品实现
  子集，未在本单调整。
- 生成器行窗口 `6298..9450` 未改，仍按 §12.5 单独立项。

## 改动文件

- `scripts/extract_dcu_698_catalog.py`
- `tests/test_dcu_698_catalog.c`
- `tests/fixtures/dcu_698_catalog_expected.h`（生成）
- `docs/design/DCU_698_OI_CATALOG.{md,json}`（生成）
- `ci/mutation_guards.tsv`
- `docs/plans/2026-11-21-dcu-698-production-plan.md`（§12.5 状态）
- `docs/evidence/DCU-D1-10A/`

未推送、未合并、未触碰 `main`、`feat/dcu` 或其它分支。
