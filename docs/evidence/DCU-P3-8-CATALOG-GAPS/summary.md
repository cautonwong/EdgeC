# DCU-P3-8-CATALOG-GAPS：7.3 方法集合逐号校验与 per-OI 覆盖

分支 `dcu/P3-8-catalog-gaps`，工作树 `/tmp/edgec-land/p3-8-catalog-gaps`，
基线 `9d065b6d`（= `feat/dcu`；施工期间 `feat/dcu` 未前进）。**未合并、未 push。**

## 问题与根因

`scripts/extract_dcu_698_catalog.py` 把 7.3 每个类的方法表折叠成最大号
（`render_header` 的 `class_own_method = max(...)`，约 :415；再与附录 A 的
OI 方法最大号取 max，约 :420-421），且附录 A 的 per-OI 方法解析
（`parse_oi_methods`，约 :367）从不与 7.3 类表对照。后果：

- 原文里 Class 7 的方法 {1,2,4,5} 被删掉中间的 5 后，最大号 4 不越过旧值，
  生成器退 0 并覆盖生成物；Class 7 多插入方法 3（最大号仍 5）同样退 0。
- 附录 A 某 OI 行的 `方法 127` 改成 127 以下的未定义号（如 4307 的 126），
  生成器退 0，并把它写进 `dcu698_catalog_oi_methods[]`。

标准自身的 7.3 类表有真实空号：Class 7/24 无方法 3，Class 9 无方法 6。
所以校验不能是“1..max 连续”，必须对照记录下来的真实集合。

## 改动

- `scripts/extract_dcu_698_catalog.py`
  - 新增 `CLASS_METHODS`：26 个 7.3 类的真实方法号元组（已用独立于解析器
    的原文扫描复核 26/26）。
  - 新增 `OI_METHOD_EXCEPTIONS = {"F001": {9, 10}}`：表 A.11 给 F001 在
    127 以下定义的方法号，7.3.18 类表没有；产品侧
    `_class_defines_method` 已有同一例外。
  - 新增 `validate_class_methods` / `validate_oi_methods`：解析后、渲染前
    执行；类方法集合必须逐号相等（缺号/多号/重复号都退 1）；每个 OI 的
    127 以下方法号必须属于其类的 7.3 集合或记录例外。`--check` 与写模式
    同样先生效。
- `ci/mutation_guards.tsv`：追加守卫
  `DCU-P3-8-method-sets-max-mask`，把逐号相等判断替换回“只看最大号”的
  旧放行逻辑，运行新负例测试。
- `tests/test_dcu_698_catalog_method_sets.py`：正反夹具（缺内部号、多号、
  per-OI 越界、真实输入逐字节不变、仓库 `--check`）。

生成物（`.md`/`.json`/reconciliation/`tests/fixtures/dcu_698_catalog_expected.h`）
逐字节未变；C 头结构与 C 侧消费者未改。

## 红（commit `f953a10f`，只含测试）

命令：`python3 -I tests/test_dcu_698_catalog_method_sets.py`（旧生成器，
原始输出 `red.log`），退 1：

- `test_missing_internal_method_is_rejected`：
  `AssertionError: 0 == 0 : generator accepted a Class 7 method table without method 5`
- `test_extra_method_is_rejected`：
  `AssertionError: 0 == 0 : generator accepted an extra Class 7 method 3`
- `test_oi_method_outside_the_class_set_is_rejected`：
  `AssertionError: 0 == 0 : generator accepted OI 4307 method 126 outside class 19`

两个正例（真实输入逐字节一致、仓库 `--check`）在旧码上已通过。

## 绿（commit `89cbe9b3`）

- `python3 -I tests/test_dcu_698_catalog_method_sets.py`：5/5 通过
  （`tests.log`）。
- `python3 -I tests/test_dcu_698_catalog_source_window.py`：6/6 通过。
- `python3 -I scripts/extract_dcu_698_catalog.py --check`：退 0。
- `python3 ci/scripts/check_dcu_698_catalog.py --root .`：
  `[DCU_698_CATALOG] catalog matches docs/698_parsed.md`。
- `ctest --test-dir build -R '^(test_dcu_698_catalog|test_dcu_698_object_model)$'`：
  2/2 通过。
- 生成物 sha256 前后一致（`sha256-before-after.txt`）。

## 守卫

`python3 ci/scripts/run_mutation_guards.py --build build --only
DCU-P3-8-method-sets-max-mask`：退出 0；变异后 `test rc=1`（红），
恢复后 `test rc=0`（绿），`✅ [MUTATION_GUARDS] 1/1 guards sensitive`。
详见 `guard.log`。

## 验收

命令（串行锁）：

```
cd /tmp/edgec-land/p3-8-catalog-gaps && flock /tmp/edgec-land/acceptance.lock \
  env TESTPLATFORM=/home/edge/test-platform PYTHON=/home/edge/EdgeC/.venv/bin/python3 \
  EDGEC_BUILD_DIR="$PWD/build" ./acceptance/run.sh dcu
```

一轮通过：退出 0，`全部合格:2 个验收任务`，
`✅ [ACCEPTANCE_TASKS] 2/2 expected tasks exited 0`；
dcu-northbound 35 passed。详见 `acceptance-1.log`。

## 守卫自注册（verifier 复核补）

`tests/test_dcu_698_catalog_method_sets.py` 已登记进 `ci/scripts/run_guard_selftest.py`
（紧跟 source-window 块，total_tests/passed_tests/failures 同口径）。
`python3 ci/scripts/run_guard_selftest.py` 退出 0，全部守卫通过，
本轮 `[PASS] test_dcu_698_catalog_method_sets.py (method-set gaps rejected,
real catalog unchanged)`；详见 `run-guard-selftest.log`。

## 未决

- 本次边界只到生成器/检查器：生成头仍只携带按类最大号，C 侧
  `tests/test_dcu_698_catalog.c` 与 `cmd/dcu/dcu_698_object_model.c` 的
  max 语义未改（文件边界外）。若要在运行时拒绝 Class 7/24 方法 3、Class 9
  方法 6，需要另立单元改生成头结构与 C 消费者。
