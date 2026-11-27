# DCU-P3-9-RUNTIME-METHODS：运行时类方法按 7.3 真实集合判定

分支 `dcu/P3-9-runtime-methods`，工作树 `/tmp/edgec-land/p3-9-runtime-methods`，
基线 `40d8dca5`（= `feat/dcu`；施工期间 `feat/dcu` 未前进，无 rebase）。**未合并、未 push。**

## 问题与根因

`cmd/dcu/dcu_698_object_model.c` 的 `k_class_max_method` 只存各类方法号上限，
`_class_defines_method` 对 `method_id <= max_method` 一律视为“类定义”。7.3 类表有
真实空号：Class 7/24 无方法 3、Class 9 无方法 6（`docs/698_parsed.md:4021-4025`、
`:4170-4177`、`:5438-5442`；P3-8 的 `CLASS_METHODS` 已按集合校验）。后果：
Class 24 实例 3000/3005 的方法 3 在 `dcu_698_check_call` 回 DAR 5，而不是 DAR 4；
Class 9 方法 6 靠一次性特判；Class 7 无实例，行为无法观测。

## 改动

- `cmd/dcu/dcu_698_object_model.c`
  - `k_class_max_method` 增加 `absent_methods` 缺号位图：Class 7/24 标 bit 3，
    Class 9 标 bit 6，其余类为 0。
  - 新增 `dcu_698_class_defines_method(class_id, method_id)`：方法号非 0、不超上限
    且不在缺号位图才为 true；与既有 `dcu_698_class_defines_attr` 对称，让无实例的
    Class 7 也可观测。
  - `_class_defines_method` 删除 Class 9/方法 6 的一次性特判，末尾改调新谓词；
    OI 专属分支（127+、F001 9/10、8000 127–131 等）顺序不变。
- `cmd/dcu/dcu_698_object_model.h`：声明新谓词。
- `tests/test_dcu_698_object_model.c`：新增两个测试（见下）。
- `ci/mutation_guards.tsv`：`DCU-P3-9-class-method-holes`。
- `docs/design/DCU_698_OBJECT_MODEL.md` §8.2/§8.4、`docs/design/DCU_698_DEVIATIONS.md`
  新增 Class 7/9/24 方法集合与 DAR 口径。

ACL（`cmd/dcu/dcu_698_appconn.c`）零 diff：广告本就只列真实方法（5000–5007 为
`{3,4,5,7,8}`，3000/3005 无方法），缺号方法从不进入 Class 20 列表。

## Spec 结论（真实集合）

| Class | 7.3 方法集合 | 缺号 |
| --- | --- | --- |
| 7 事件对象 | 1/2/4/5 | 3 |
| 9 冻结数据 | 1/2/3/4/5/7/8 | 6 |
| 24 分项事件 | 1/2/4/5 | 3 |

任务原文对“Class 7（3000/3005?）”不确定：目录与 7.3 表明 3000/3005 是 Class 24，
Class 7 在本产品无对象实例；因此 Class 7 只经类谓词观测，Class 24 另经
`dcu_698_call` 观测。

## 红（commit `42b4107f`，只含测试）

1. 行为断言（旧码可编译）：`CMOCKA_TEST_FILTER=test_class24_missing_method_is_undefined`
   退 1，原始断言 `-21 != -23`（`tests/test_dcu_698_object_model.c:257`）：
   3000/3005 方法 3 旧码回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5），期望
   `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4）。
2. 类谓词测试加入后旧码构建退 1：`implicit declaration of function
   'dcu_698_class_defines_method'`（新接口尚不存在）。

完整原始输出见 `red.log`。

## 绿（commit `88005fcf`）

- 新测试：`test_class24_missing_method_is_undefined`、`test_class_method_sets_exclude_standard_holes`
  各 1/1 通过；`test_dcu_698_object_model` 全量 26/26 通过；`test_dcu_698_catalog` 5/5。
- 定向：`ctest --test-dir build -R 'dcu|dlt698'` → **45/45 通过**，0 失败
  （Total Test time 108.73 sec），`tests.log`。
- 既有 DAR/ACL/目录测试保持绿：5004 方法 6 仍 DAR 4，5004 方法 3 仍 DAR 7（空参数），
  3000 方法 2 仍 DAR 5，Class 20 对象列表不变。

## 守卫

`ci/mutation_guards.tsv` 新条目 `DCU-P3-9-class-method-holes`：把谓词的
“上限且非缺号”替换回“只看上限”。`run_mutation_guards.py --build build --only
DCU-P3-9-class-method-holes`：mutated build rc=0 / test rc=8 红，还原后 rc=0 绿，
`1/1 guards sensitive`（`guard.log`）。

## 验收

命令（串行锁）：`flock /tmp/edgec-land/acceptance.lock env TESTPLATFORM=... PYTHON=...
EDGEC_BUILD_DIR=$PWD/build ./acceptance/run.sh dcu`。退出 0，`全部合格:2 个验收任务`
（dcu_acceptance、dcu_northbound；`acceptance-1.log`）。

## 文档守卫

`python3 ci/scripts/check_doc_references.py --root .` 退 0；`check_guard_path_independence.py
--root .` 退 0。

## 未决 / 说明

- 端到端 ACTION 对 3000/3005 方法 3 仍是 ACL 先拒（DAR 3）：ACL 不广告该方法，本单
  有意不改广告；DAR 4 在 resolver（`dcu_698_check_call`）层可见。
- Class 7 仍无对象实例（DEVIATIONS 既有行不变），本次仅保证类谓词不再把方法 3
  判为定义。
- Class 9 方法 6 的对外行为与修复前一致（原特判本就回 DAR 4），本单把它并入统一
  位图，消除一次性特判。
- 未改 `scripts/extract_dcu_698_catalog.py`、`cmd/dcu/dcu_698_var.c`、
  `cmd/dcu/dcu_698_router.h`、`cmd/dcu/dcu_698_appconn.c`、`.audit/`、`docs/plans/`。
