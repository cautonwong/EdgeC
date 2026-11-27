# DCU-P3-9-RUNTIME-METHODS spec

## 事实依据（先于实现）

- `docs/698_parsed.md`：7.3.7 表142（`4003-4025`）方法为 1/2/4/5；7.3.9 表148（`4159-4177`）
  方法为 1/2/3/4/5/7/8；7.3.24 表193（`5416-5442`）方法为 1/2/4/5。
- `scripts/extract_dcu_698_catalog.py` 的 `CLASS_METHODS`（P3-8 已按 7.3 集合校验）：
  `7:(1,2,4,5)`、`9:(1,2,3,4,5,7,8)`、`24:(1,2,4,5)`；其余类为 1..max 连续。
- 运行时：`cmd/dcu/dcu_698_object_model.c` 的 `k_class_max_method` 只存上限，
  `_class_defines_method` 按 max 放行，Class 7/24 方法 3、Class 9 方法 6 被当作“类定义”。
- ACL：`cmd/dcu/dcu_698_appconn.c` 的 `k_objects`/`k_methods_*` 按 OI 显式广告；
  5000–5007 广告 `{3,4,5,7,8}`，3000/3005 无方法广告，Class 7 无对象。
- 目录事实：`tests/fixtures/dcu_698_catalog_expected.h` 把 OI 3000–3005 记为 class 24；
  Class 7 无产品对象行（`docs/design/DCU_698_DEVIATIONS.md` Class 7 行）。

### 任务歧义裁决

任务原文对“Class 7（3000/3005?）”不确定。以目录与 7.3 为准：3000/3005 是
**Class 24**；Class 7 在本产品无对象实例。因此 Class 7 的“方法 3 不存在”在运行时
只能经类定义谓词观测；Class 24 另经 `dcu_698_call` 观测。

## 五栏

| 编号 | 业务规则 | 不变量 | 可观测输出 | 验收判据 | 非目标 |
| --- | --- | --- | --- | --- | --- |
| S1 | Class 7 的 7.3 方法集合是 `{1,2,4,5}`；方法 3 不存在 | `dcu_698_class_defines_method(7, m)`：m=1/2/4/5 为 true；m=0、3、>=6 为 false | 公开谓词返回值（无 Class 7 对象行，`dcu_698_call` 仍回 DAR 6，不变） | 测试 `test_class_method_sets_exclude_standard_holes` | 不新增 Class 7 对象行；不改 DEVIATIONS 的 Class 7 无实例结论 |
| S2 | Class 9 的 7.3 方法集合是 `{1,2,3,4,5,7,8}`；方法 6 不存在 | `dcu_698_class_defines_method(9, 6)==false`；1–5、7、8 为 true；>=9 为 false | `dcu_698_call(5000..5007, 6)` 回 `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4），与修复前一致；合法方法 3/4/5/7/8 的处理器行为不变 | 新测试 + 既有 `test_mask_rejects_missing_attr_and_method` | 不改冻结方法 3/4/5/7/8 的执行语义 |
| S3 | Class 24 的 7.3 方法集合是 `{1,2,4,5}`；方法 3 不存在 | `dcu_698_class_defines_method(24, 3)==false`；1/2/4/5 为 true；>=6 为 false。3000/3005 的 `dcu_698_check_call`：方法 3 → UNDEFINED；方法 2/4/5 → CLASS_MISMATCH；方法 1 → EDU 处理器 | `dcu_698_call(MK_OMD(3000/3005, 3))` 由 CLASS_MISMATCH（DAR 5）变为 `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4）；方法 2/4/5 仍 DAR 5；方法 1 仍执行 | 测试 `test_class24_missing_method_is_undefined` + 既有 `test_dar_classification_by_class_definition` | 不改 3000/3005 的属性、方法 1 行为、事件存储 |
| S4 | 其余接口类的方法集合仍是 1..max 连续 | 1–6、8、10–23、25、26 的 `dcu_698_class_defines_method` 与修复前逐值一致 | 谓词返回值；既有 DAR 测试不变 | 新测试覆盖代表类；定向 ctest 全绿 | 不重排/不合并 `k_class_max_method` 的既有上限 |
| S5 | ACL 广告按 OI 显式表，不按类 max 推导；缺号方法不广告 | Class 20 对象列表不变：5000–5007 方法 `{3,4,5,7,8}`；3000/3005 无方法；Class 7 无对象 | 主站 ACTION 未广告方法仍先被 ACL 拒（DAR 3）；已广告合法方法可达 resolver | `dcu_698_appconn.c` 零 diff；既有 `test_dcu_698_appconn` 全绿 | 不改 `dcu_698_appconn.c`、不动 ACL 属性/方法表 |
| S6 | DAR 分层不变：类/方法未定义 → DAR 4；类定义但无来源/未掩码 → DAR 5；未知 OI → DAR 6；未广告 → DAR 3 | `EDGE_SLAVE_SRC_UNDEFINED`/`CLASS_MISMATCH`/`NOT_FOUND`/`DENIED` 的产生条件与 `docs/design/DCU_698_OBJECT_MODEL.md` §5 一致 | ACTION 应答 DAR 值 | S3 测试 + 既有 DAR 测试 | 不改驱动 DAR 映射 |
| S7 | OI 专属方法（127+、F001 的 9/10、6000 127–134、8000 127–131 等）不受类集合收紧影响 | `_class_defines_method` 的 OI 专属分支先于类表判定 | 既有 ACTION 测试的返回码不变 | 既有 `test_dar_classification_by_class_definition`、`test_dcu_698_catalog` | 不改 `k_oi_max_method` 与 OI 特判 |

## 测试到 spec 的映射

| 测试 | spec |
| --- | --- |
| `test_class_method_sets_exclude_standard_holes`（新增） | S1、S2、S3、S4 |
| `test_class24_missing_method_is_undefined`（新增） | S3、S6 |
| `test_mask_rejects_missing_attr_and_method`（既有，保持绿） | S2、S6 |
| `test_dar_classification_by_class_definition`（既有，保持绿） | S3、S6、S7 |
| `test_dcu_698_catalog`（既有，保持绿） | S4、S7 |
| `test_dcu_698_appconn`（既有，保持绿） | S5 |

## 验收判据（整体）

1. 红：新增行为断言在修复前失败（Class 24 方法 3 当前回 DAR 5）。
2. 绿：`ctest --test-dir build -R 'dcu|dlt698'` 全绿，记录通过数。
3. 守卫：`ci/mutation_guards.tsv` 新条目在“恢复 max 放行”变异下新测试红，还原后绿。
4. 验收：`acceptance/run.sh dcu` 退出 0，全部合格 2 个验收任务。
5. 文档：`check_doc_references.py --root .` 绿。

## 全局非目标

- 不改 `scripts/extract_dcu_698_catalog.py`（P3-8 已完成校验）。
- 不改 `cmd/dcu/dcu_698_var.c`、`cmd/dcu/dcu_698_router.h`、`cmd/dcu/dcu_698_appconn.c`。
- 不改事件/冻结合法方法的执行语义、属性掩码、ACL 广告内容。
- 不新增对象行、不新增 Class 7 实例、不改 `.audit/` 与 `docs/plans/`。
