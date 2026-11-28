# DCU-D5-04b 五栏规格：Class 12 脉冲计量 2402..2408 诚实只读骨架与 ACL

## 1. 业务规则
1. 在 `cmd/dcu/dcu_698_object_model.c` 中补齐 Class 12（`IC_PULSE = 12`）其余 7 个标准脉冲计量对象：`0x2402`..`0x2408`（与 `0x2401` 相同：`scope = DCU_OBJ_SINGLETON`，`attr_mask = 0`，`method_mask = 0`，不注册任何读/写/方法处理器）。
2. 在 `cmd/dcu/dcu_698_appconn.c` 中将 `0x2402`..`0x2408` 以只读方式加入 `k_objects[]`（复用 `k_attrs_2401` 15 项只读属性 `{1..15}` 与 `k_methods_pulse` 4 项只读方法 `{1..4}`）。属性 16/17 因标准表 157/158 冲突继续 HOLD，18/19 超 ACL 16 属性 schema 上限继续不广告。
3. 由于产品当前无脉冲输入与脉冲电能示值来源，所有已广告属性 GET 必须诚实返回 DAR 5（`EDGE_SLAVE_SRC_CLASS_MISMATCH`），所有已广告方法 ACTION 必须诚实返回 DAR 5，所有 SET 请求必须被 ACL 拒绝并返回 DAR 3（`EDGE_SLAVE_SRC_DENIED`）。

## 2. 不变量
1. `dcu_698_object_count()` 从 `136` 增至 `143`（`<= 224`）；`dcu_698_appconn` ACL 对象数从 `131` 增至 `138`（`<= EDGE_SLAVE_ACL_MAX_OBJECTS = 224`）。
2. `4400` 对象列表 A-XDR 编码字节数保持在 `EDGE_SLAVE_AXDR_MAX`（`10240`）以内，会话序列化字节数保持在 `DRIVER_PRIV_SIZE`（`4096`）以内。
3. 严禁为 `2402..2408` 伪造脉冲计数值、虚拟电能量或假方法执行成功。
4. `dcu_698_value_for_oad` 标量路径对 `2402..2408` 与 `2401` 保持一致（`TYPE_MISMATCH` / `CLASS_MISMATCH` / `UNDEFINED`）。

## 3. 可观测输出
- 对 `0x2402..0x2408` 属性 `1..15`：
  - `dcu_north_resolve_axdr` 返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）。
  - `dcu_north_accept_axdr` 返回 `EDGE_SLAVE_SRC_DENIED`（DAR 3）。
- 对 `0x2402..0x2408` 类定义但未广告属性 `16..19`：
  - `dcu_698_read` / `dcu_north_resolve_axdr`（无 ACL 时）返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）；经 Class 20 ACL 门禁先返回 `EDGE_SLAVE_SRC_DENIED`（DAR 3）。
- 对 `0x2402..0x2408` 类外属性（如 `20`）：
  - `dcu_north_resolve_axdr` 返回 `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4）；经 ACL 先返回 DAR 3。
- 对 `0x2402..0x2408` 方法 `1..4`：
  - `dcu_698_call` / `dcu_north_accept_action` 返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）。
- 对 `0x2402..0x2408` 类外方法（如 `5`）：
  - `dcu_698_call` / `dcu_north_accept_action` 返回 `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4）。

## 4. 验收判据
1. `tests/test_dcu_698_catalog.c` 更新对象数（`143`）与 ACL 对象数（`138`）并全绿。
2. `tests/test_dcu_698_object_model.c` 与 `tests/test_dcu_698_appconn.c` 新增覆盖 `0x2402..0x2408` 的正反断言（DAR 5/4/3、ACL 1..15 只读、方法 1..4 只读）并全绿。
3. 在 `ci/mutation_guards.tsv` 登记一条针对 `0x2402..0x2408` 骨架/ACL 的变异守卫（删去 `0x2402` 对象或 ACL 行后目标测试必须变红，恢复后变绿）。
4. Clean build 0 warning，`ctest -R 'dcu|dlt698'` 全绿，`run_guard_selftest.py` 与 `run_mutation_guards.py` 全绿。

## 5. 非目标
- 不实现脉冲输入驱动、脉冲电能累加、需量或事件产生器。
- 不修改 `modules/edge_core` 或受保护的 `foundation/include/` 头文件。
- 不裁决表 157/158 属性 16/17 冲突（继续 HOLD）。
