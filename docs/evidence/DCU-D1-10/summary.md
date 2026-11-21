# DCU-D1-10 对象表与目录一致性测试

基线 `6638ce23`（D0-09 已合入），分支 `dcu/D1-10-catalog-consistency`。

## 做了什么

新增守卫 `tests/test_dcu_698_catalog.c`，手写对象表与生成目录不一致时必红。目录侧新增一份生成夹具 `tests/fixtures/dcu_698_catalog_expected.h`，由 `scripts/extract_dcu_698_catalog.py` 生成，`--check` 一并校验，所以 D0-07 的门禁会拦住夹具过期。

## 夹具内容

- 每个 OI 一行：`{oi, class_id}`（514 行，按 OI 数值排序）。
- 每个类一行：`{class_id, max_attr, max_method}`（26 行，按 class_id 排序）。
- `max_attr` 取 7.3 类定义的属性号最大值。
- `max_method` 取 7.3 方法号与**该类的 OI 在附录 A 中定义的方法号**的最大值。这一步必须做：class 10 的 7.3 方法只到 3，而 6000 的方法 127-134 定义在附录 A.7，6002 的 127-129 同样如此。没有它，表里的 127/128 会被误判越界。

## 断言

1. 表里每个 OI 都在目录中，且 `class_id` 完全相等（D0-09 修正后不再需要例外）。
2. `attr_mask` 的每个置位号不超过该类 `max_attr`；`method_mask` 同理不超过 `max_method`。
3. 表内没有重复 OI。

对象表通过新增的两个只读访问器暴露行数与第 i 行（`dcu_698_object_count` / `dcu_698_object_at`），静态数组本身不出头文件。

## 已知不覆盖

3000/3005 的 `attr_mask` 仍用 Class 7 式属性号（2-6、8-10），而 `class_id` 已是 24。范围检查不会报错：Class 24 的 `max_attr` 是 14，这些号都在范围内。这个线型剖面问题本单不处理，也不改 mask，单独跟踪。

## 验证

| 项 | 结果 |
| --- | --- |
| 构建 | 退出 0，0 错误，0 告警 |
| 新测试（绿） | 3/3，退出 0 |
| 故意把 F200 改成 `IC_ACQUISITION` | 测试红，退出 8，失败在“OI 在目录中且 class 匹配” |
| 恢复 `IC_PERIPHERAL` | 3/3，退出 0 |
| `ctest -R 'dcu\|dlt698'` | 29/29（原 28 + 新测试） |
| `check_dcu_698_catalog.py --root .` | 通过；`--check` 现在也校验夹具 |
| `run_guard_selftest.py --root .` | 通过；临时副本连同夹具一起复制，未改动副本被接受、改坏后拒收 |
| `./test.sh --omnis dcu` | 2 任务 6 用例全合格 |
| `git diff --check` | 干净 |

## 改动文件

- `tests/fixtures/dcu_698_catalog_expected.h`（新增，生成）
- `tests/test_dcu_698_catalog.c`（新增）
- `scripts/extract_dcu_698_catalog.py`（附录 A 方法解析、夹具生成与校验）
- `cmd/dcu/dcu_698_object_model.{h,c}`（两个只读访问器）
- `tests/CMakeLists.txt`（注册测试与夹具包含路径）
- `ci/scripts/run_guard_selftest.py`（临时目录复制夹具）
- `docs/evidence/DCU-D1-10/`

未改对象表内容、OI、mask、北向适配与驱动。

## 没做的

- 没在 GitHub Actions 上重跑；以上为本机同命令结果。
- 3000/3005 的 Class 7/24 属性号剖面问题未处理（见上）。
