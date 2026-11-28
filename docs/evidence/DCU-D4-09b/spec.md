# DCU-D4-09b 五栏规格：Class 23 总加组 (`2301..2308`) 领域引擎与总加/滑差计算

## 1. 业务规则
1. 新建 `cmd/dcu/dcu_698_group.h` 与 `cmd/dcu/dcu_698_group.c`，实现纯软件 Class 23 总加组（`0x2301..0x2308`，共 8 组）领域引擎（独立模块，暂不修改 `dcu_698_object_model.c`）：
   - 每组支持最多 `DCU_GROUP_MAX_MEMBERS = 16` 个成员配置单元 `{tsa/slot_id, add_sub_flag (0 加 / 1 减), calc_flag (0 不参与 / 1 参与)}`。
   - 提供成员管理接口：`dcu_698_group_init`、`dcu_698_group_add_member`、`dcu_698_group_delete_member`、`dcu_698_group_clear`（对应 M3/M4/M5 语义）。
   - 提供采样计算与滑差引擎：`dcu_698_group_sample(group_idx, read_fn, ctx, now_s)` 通过注入的读点回调获取各成员当前有功功率/无功功率/正向有功电能/反向有功电能，按 `add_sub_flag`（`+1` / `-1`）与 `calc_flag` 聚合计算：
     - A3：当前总加有功功率（kW × 10000，W）
     - A4：当前总加无功功率
     - A5：有功滑差平均功率（基于固定环形历史窗口 `DCU_GROUP_SLIP_WINDOW = 15` 分钟/样本）
     - A6：无功滑差平均功率
     - A7..A10：总加正向/反向有功、无功电能
   - 提供 A-XDR 编码辅助函数：`dcu_698_group_encode_attr(group_idx, attr_id, buf, cap, out_len)`，覆盖 `A2`（配置单元数组）与 `A3..A10`。

## 2. 不变量
1. **零动态内存**：全部使用固定静态数组与结构体（通过 `check_no_dynamic_memory.py`）。
2. **加减标志严格语义**：`calc_flag == 0` 的成员不计入总加；`add_sub_flag == 1` 取负号相减；无有效样本时滑差不除零、功率不伪造。
3. **边界安全**：成员数超 `16` 或 `group_idx >= 8` 返回明确错误码，不越界。

## 3. 可观测输出
- 加/减成员聚合：成员 A（`+1000 W`）+ 成员 B（`-300 W`）+ 成员 C（`calc_flag=0`, `500 W`）总加有功功率精确等于 `700 W`。
- 滑差窗口：连续注入 N 次功率样本后，滑差功率等于窗口内样本的算术平均，且环形窗口满后正确淘汰最旧样本。
- `A2` 与 `A3..A10` A-XDR 编码输出符合 DL/T 698.45 标签格式且在 `cap` 不足时返回 `-1`。

## 4. 验收判据
1. `tests/test_dcu_698_group.c` 覆盖成员增删改查、加减标志、滑差环形窗口、A2/A3..A10 A-XDR 编码及越界负路径，并接入 `tests/CMakeLists.txt`。
2. `cmd/dcu/CMakeLists.txt` 编译 `dcu_698_group.c`。
3. Clean Release build 0 warning，`ctest -R 'dcu_698_group'` 全绿，ASAN 下 0 错误，`check_no_dynamic_memory.py` 通过。

## 5. 非目标
- 不在本单修改 `cmd/dcu/dcu_698_object_model.c`（留待下一批接线）。
- 不实现 Class 13 功控/电控跳闸执行（继续等继电器 DO）。
