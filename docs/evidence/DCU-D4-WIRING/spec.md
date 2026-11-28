# DCU-D4-WIRING 五栏规格：Class 23 总加组与 Class 19 `4300` 接入对象模型与北向通道

## 1. 业务规则
1. 将 Batch 4 已实现的 `dcu_698_group.{h,c}`（`0x2301..0x2308`）与 `dcu_698_devmgr.{h,c}`（`0x4300`）接入 `cmd/dcu/dcu_698_object_model.c`、`cmd/dcu/dcu_698_axdr.c` 与 `cmd/dcu/dcu_north_adapter.c`：
   - `0x2301..0x2308`：打开 `.read` / `.method` 与 `attr_mask`（`A2..A10`）、`method_mask`（`M3/M4/M5`），支持通过 `dcu_north_resolve_axdr` 读取 `A2..A10`（未配置时返回空数组/0 值不伪造）、通过 `dcu_north_accept_action` 执行 `M3`（加成员）/ `M4`（删成员）/ `M5`（清空）。
   - `0x4300`：打开 `.read` / `.method` 与 `attr_mask`（`A2..A5`）、`method_mask`（`M1/M3/M4/M5/M6`），支持通过 `dcu_north_resolve_axdr` 读取 `A2..A5`，`M3/M5/M6` 走软件初始化端口，`M1/M4` 仍诚实回 DAR 5（`EDGE_SLAVE_SRC_CLASS_MISMATCH`）。
2. 在 `cmd/dcu/dcu_698_axdr.c` 收敛 ACTION OI 路由判断辅助函数，保证 `cmd/dcu/dcu_north_adapter.c` 行数严格 `<= 205`（满足 `check_composition_root_budget.py`）。
3. 同步更新 `tests/test_dcu_698_object_model.c`、`tests/test_dcu_698_axdr.c`、`tests/test_dcu_698_p4_matrix.c` 中曾断言 `2301..2308` / `4300` 为空掩码 DAR 5 的用例，并补充端到端 `dcu_north_resolve_axdr` / `dcu_north_accept_action` 断言。

## 2. 不变量
1. `cmd/dcu/dcu_north_adapter.c <= 205` 行（组合根预算不变红）。
2. 零动态内存（`check_no_dynamic_memory.py` 通过）。
3. `4300` M1/M4 无硬件复位端口时绝不返回 0 假装重启或恢复出厂。

## 3. 可观测输出
- `dcu_north_accept_action` 对 `0x2301` M3 添加成员后，`dcu_north_resolve_axdr` 读 `0x23010200` 返回包含该成员的 A-XDR 数组；M5 清空后恢复为空数组。
- `dcu_north_resolve_axdr` 读 `0x43000200..0x43000500` 返回 `0` 及合法 A-XDR 数据；`dcu_north_accept_action` 对 `0x4300` M1/M4 返回 DAR 5。

## 4. 验收判据
1. Clean Release build 0 warning，`ctest -R 'dcu|dlt698'` 全绿。
2. `run_guard_selftest.py`（含 `check_composition_root_budget.py`、`check_no_dynamic_memory.py`）全绿。
3. ASAN 下 `test_dcu_698_axdr`、`test_dcu_698_object_model`、`test_dcu_698_p4_matrix` 0 错误。

## 5. 非目标
- 不在本单修改 `dcu_698_stat.*` 或 `dcu_poller.c`。
- 不修改 `modules/edge_core` 或受保护的 `foundation/include/` 头文件。
