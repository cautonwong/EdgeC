# DCU-D3-08c 验收证据：300B 空掩码事件对象

规格：`docs/evidence/DCU-D3-08c/spec.md`（五栏）。本单元按该规格实现，未扩展范围。

## 变更

- `cmd/dcu/dcu_698_object_model.c`：新增 `0x300B`（`IC_SUB_EVENT`=24，`DCU_OBJ_SHARED`，空属性/方法掩码，无实例）。
- `cmd/dcu/dcu_698_appconn.c`：新增 `k_attrs_300b`，Class 20 ACL 只读广告 A5..A9、0 方法；对象数 130 -> 131。
- `cmd/dcu/dcu_698_class_event.c`：`dcu_698_event_read`/`write`/`encode_axdr`/`execute` 先看掩码再看实例；
  定义未广告 -> DAR 5，类外 -> DAR 4，不因实例为空回 DAR 6。
- `cmd/dcu/dcu_698_object_model.c`：对象表 135 -> 136 行。
- 测试：`test_dcu_698_object_model.c` 新增 `test_class24_300b_empty_event_row`（resolver DAR 5/4）；
  `test_dcu_698_appconn.c` 新增 ACL 行断言；`test_driver_dlt698.c` 新增 `test_slave_class24_300b_dar`（GET DAR 5、SET DAR 3）；
  同步 object/ACL 计数断言（135->136、130->131、6435->6476、0x82->0x83）与既有 300B DAR 6 断言。
- `docs/design/DCU_698_OBJECT_MODEL.md`：对象/ACL/编码长度与 §8.4 300B 现状同步。
- `ci/mutation_guards.tsv`：新增 `DCU-D3-08c-300B-row`（删 300B 行 -> 测试红，恢复 -> 绿）。

## 门禁

- 干净 Ninja build（`--clean-first`）：exit 0。
- `check_build_warnings.py`：0 violation。
- 目标测试（object_model / appconn / driver / axdr / catalog / p4_matrix / scheme_mgmt）：7/7 pass。
- 全量 `ctest`：381/381 pass，411.90 s。
- `run_guard_selftest.py`：PASS。
- `run_mutation_guards.py --selftest`：PASS。
- 变更守卫 `DCU-D3-08c-300B-row`：删除 300B 行时 `test_class24_300b_empty_event_row` 变红，恢复后变绿（见 `tests.log` 末尾）。

原始命令输出见同目录 `tests.log`。
