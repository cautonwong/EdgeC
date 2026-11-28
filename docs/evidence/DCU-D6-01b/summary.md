# DCU-D6-01b 实现摘要：实例属性上限与变量类 DAR 分类

| 项 | 值 |
| --- | --- |
| 分支 | `dcu/D6-01b-instance-attr-max` |
| 基线 | `bc777aaa`（feat/dcu） |
| 代码提交 | `55af50a4` |
| 规格 | `docs/evidence/DCU-D6-01b/spec.md`（未改动） |
| 证据 | `docs/evidence/DCU-D6-01b/tests.log` |

## 改动

1. `cmd/dcu/dcu_698_object_model.c`：`k_oi_max_attr` 增加 `{0x4000, 4}` 与 `{OI_POWER_SAVE, 5}`。
   表 A.5 为 4000 定义 A2..A4、为 8001 定义 A2..A5；7.3 类表 Class 8 只到 A2。
   不改 `k_class_max_attr`，不改任何对象 `attr_mask`。
2. `cmd/dcu/dcu_698_var.c`：
   - `_var_encode_axdr` 的兜底分支（A1、A6+、无 attr4 的 A4）改为
     `dcu_698_oi_defines_attr(oi, desc->class_id, attr_id) ? DAR5 : DAR4`，与
     `_unadvertised_attr_rc` 同一分类；未知 OI 仍在函数入口回 `EDGE_SLAVE_SRC_NOT_FOUND`。
   - `dcu_698_value_for_oad` 变量类分支：A2..A5 保持标量通道 DAR 7；其余走
     `_unadvertised_attr_rc`，对象表缺失时仍回 `EDGE_SLAVE_SRC_NOT_FOUND`。
3. 测试：`test_discovery_oi_attributes_beyond_class_table` 增加两个 OI 上限；
   新增 `test_variable_attr_dar_classification`（A1/A6/未知 OI）；
   新增 `test_driver_get_unadvertised_and_undefined_dar`（驱动 GET）；
   `test_wrong_slot_and_attr` 的 0x2000 A1/A4 期望从 DAR 6 更正为 DAR 5/4。
4. `ci/mutation_guards.tsv`：新增 `DCU-D6-01b-instance-attr-max`。

## 验收判据对照

| 判据 | 结果 |
| --- | --- |
| `oi_defines_attr(0x4000,8,3)` / `(…,4)` true，`(…,5)` false | 通过 |
| `oi_defines_attr(0x8001,8,3..5)` true，`(…,6)` false | 通过 |
| `value_for_oad` 0x00100100 → CLASS_MISMATCH，0x00100600 → UNDEFINED，未知 OI → NOT_FOUND | 通过 |
| 驱动 GET 0010 A6 → DAR 4 | 通过（`dcu_north_resolve_axdr`） |
| 驱动 GET 4000 A3 → DAR 5 | 见下方歧义；对象模型/标量通道通过 |

## 规格歧义（需 verifier 知悉）

规格可观测输出 4 称 `dcu_north_resolve_axdr` GET `4000 A3` 回 DAR 5。实测该路径当前返回成功：

- `dcu_north_resolve_axdr` 对参数 OI 先分派到 `dcu_698_param_encode_axdr`（`dcu_698_axdr.c`），
  而 `g_params` 已实现 4000 A2/A3/A4，且 ACL `k_attrs_4000` 把 A2/A3/A4 都广告为读写。
- 因此 GET 4000 A3 会回真实校时模式值，不是 DAR。`tests/test_dcu_698_axdr.c`
  既有用例也写读回 4000 A3/A4。

要让 axdr 通道 4000 A3 回 DAR 5，只能让参数读取忽略对象 `attr_mask`（删掉现有可用特性），
与规格非目标 1「不实现 4000 A3/A4 的值」和既有写读回测试冲突。故本单元不破坏该特性；
`k_oi_max_attr` 与 `value_for_oad` 的分类修复在对象模型/标量通道可观测：
`dcu_north_resolve_scalar(4000 A3)` 回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5），
已在 `test_driver_get_unadvertised_and_undefined_dar` 断言。是否另立规格更正由评审决定。

## 门禁

- 干净 Release 构建（CI 等价参数）+ `check_build_warnings.py --root .`：0 新告警。
- 定向测试：`test_dcu_698_object_model` 27/27、`test_dcu_698_var` 10/10、
  `test_dcu_698_axdr` 61/61、`test_dcu_698_param` 通过；Debug 与 Release 均通过。
- 全量 `ctest`（Debug）：381/381 通过，360.41 s。
- `run_guard_selftest.py`：全部 PASS。
- 变异守卫 `DCU-D6-01b-instance-attr-max`：改 `{0x4000,4}`→`{0x4000,2}` 时定向测试红（rc=8），
  恢复后绿（rc=0）。
