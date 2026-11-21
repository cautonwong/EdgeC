# DCU-D0-09 注释审查

工具：`pstack:no-comments`（comment-sicko 子代理，opus），范围 `fcb10511` 触及的四个代码文件。

## 接受的处理

删除 55 行注释，分布在：

- `cmd/dcu/dcu_router.h`：29 行。包括已被实现推翻的 `dcu_tag_map_fn` 文档（写的 `MeterIdx * 10 + Offset`，实际 `device_id * 32 + slot`）、与 `DCU_SHADOW_CAPACITY` 推导和 `dcu_node.c` 检查重复的容量同步说明、分组横幅、结构体字段的逐行释义。
- `cmd/dcu/dcu_698_object_model.c`：17 行。包括与 `dcu_inventory.h` 重复的线程所有权说明与 `revision++` 说明、三处重复的 `-4` 理由、127/128 兼容映射注记、`_oi_8000_execute` 的聚合根说明、空的 `IC_RECORD` 分支 TODO。
- `tests/test_dcu_698_object_model.c`：9 行。测试意图与期望码的旁注，期望码在头文件已写明。

审查报告给出的保留理由都核对过：协议条款类注释（IC 映射、OAD/OI 编号、事件属性号、`1=跳闸 0=合闸允许`）、公开 API 契约、生成器的规则来源与 OCR 碎行过滤依据，全部保留在文件里。被删的“容量必须同步”说法确实已被 `DCU_SHADOW_CAPACITY` 与 `dcu_node.c:97-104` 取代；被删的 `revision++` 说明在 `dcu_inventory.h:46-55` 有同一份。

## 未接受的删除

恢复 1 行：`dcu_698_snapshot` 里的 `/* class_id 是标准接口类（3xxx 为 24）；快照按处理器族分派。 */`。

理由：这一行是 D0-09 派工明确要求的说明（“with a one-line comment why class_id is not the discriminator”），不是对代码的复述；删掉后 `obj->read == _oi_event_read` 这个类型标签式判断就没有任何依据可查。

## MUST KILL / HOLD（记录，不在本单处理）

- 快照用 `obj->read == _oi_event_read` 当类型标签，且 `IC_RECORD` 分支为空。审查建议加显式的对象类别字段或谓词。本单按派工选择处理器族判断；改成字段是结构性改动，另立。
- `_oi_6000_execute` 的 127/128 是生产代码里的测试兼容分支（`[Legacy/Testing mapping]`），应移到夹具或删除。属既有代码，不在本单。
- 3000/3005 的 `attr_mask` 用 Class 7 的属性号（2-6、8-10），而表 A.4 与 7.3.24 把这些 OI 定为 Class 24，属性号不同。这是真实冲突，但本单冻结 mask，未改；`dcu_router.h` 的结构体字段注释与 `dcu_obj_snapshot` 文档因此保持原样（HOLD），待一致性测试或线上一致性核对时统一。

## 审查后的验证

- 干净构建：0 编译错误、0 告警；`check_build_warnings.py` 通过。
- `ctest -R 'dcu|dlt698'`：28/28，退出 0（`test_dcu_advanced_modules` 覆盖 3000/3005 快照）。
- `check_dcu_698_catalog.py --root .`、`run_guard_selftest.py`、`./test.sh --omnis dcu`、`git diff --check` 均通过。
