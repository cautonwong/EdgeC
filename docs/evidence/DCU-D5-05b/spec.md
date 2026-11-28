# DCU-D5-05b 五栏规格：Class 14/15/16 统计/累加平均/极值软件引擎与 698 接线

## 1. 业务规则
1. 新建 `cmd/dcu/dcu_698_stat.h` 与 `cmd/dcu/dcu_698_stat.c`，实现纯软件统计领域引擎（覆盖 Class 14 `0x2100..0x2104` 区间统计、Class 15 `0x2110..0x2114` 累加平均、Class 16 `0x2120..0x2124` 极值统计）：
   - 每个统计 OI（共 15 个）支持最多 `DCU_STAT_MAX_ENTRIES = 8` 个配置条目，记录源 OAD（`uint32_t src_oad`）、周期（`uint32_t period_s`）与运行期统计状态（`count`、`sum`、`avg`、`min`、`max`、`min_ts_s`、`max_ts_s`、`last_ts_s`）。
   - 提供样本注入接口 `dcu_698_stat_ingest(uint32_t src_oad, double value, uint64_t ts_s)`：更新所有匹配 `src_oad` 的统计条目（首个样本初始化 `min`/`max`，后续增量更新 `count`/`sum`/`avg`/`min`/`max` 及极值时标）。
   - 提供对象模型读写与方法处理器：
     - `dcu_698_stat_read`：编码 Attribute 2（A-XDR `array` of `structure`）；未配置条目时返回空数组（`01 00`），已配置时按标准编码当前统计值（数值按 `double-long` × 100 或 `double-long-unsigned`、`date_time_s` 7 字节）。
     - `dcu_698_stat_execute`：支持 Method 1（复位统计计数/极值，保留配置）、Method 3（添加/配置统计 OAD 与周期）、Method 4（删除指定 OAD 配置）。参数形状非法返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`（DAR 7），不存在返回 `EDGE_SLAVE_SRC_NOT_FOUND`（DAR 6）。
2. 在 `cmd/dcu/dcu_698_object_model.c` 中为 `0x2100..0x2124` 挂接 `.read = dcu_698_stat_read`、`.method = dcu_698_stat_execute`，并打开 `attr_mask`（A2）与 `method_mask`（M1/M3/M4）。
3. 注意检查 `tests/test_dcu_698_axdr.c` / `tests/test_dcu_698_catalog.c` / `tests/test_dcu_698_p4_matrix.c` 是否对 `2100..2124` 断言空掩码或 DAR 5，必要时同步更新为新契约。

## 2. 不变量
1. **零动态内存**：严禁使用 `malloc`/`calloc`/`realloc`/`free`（通过 `check_no_dynamic_memory.py`），全部使用固定静态存储。
2. **不伪造数值**：未注入任何样本的条目，`count == 0` 且不返回虚构统计值；未配置条目的 OI 返回空数组 `01 00`。
3. **跨窗不变量**：复位（M1）清空样本计数与累计值但保留配置；删除（M4）仅移除指定 OAD。

## 3. 可观测输出
- `dcu_698_stat_ingest` 注入多组样本后，`dcu_north_resolve_axdr` 读 `0x21000200` / `0x21100200` / `0x21200200` 返回 `0` 且 A-XDR 载荷包含精确的 `count/sum/avg/min/max`。
- Method 1 复位后 `count == 0`；Method 3 添加与 Method 4 删除后数组长度变化；错误参数返回 DAR 7。

## 4. 验收判据
1. `tests/test_dcu_698_stat.c` 覆盖区间统计、累加平均、极值与时标、M1/M3/M4 方法及负向边界，并接入 `tests/CMakeLists.txt`。
2. Clean Release build 0 warning，`ctest -R 'dcu|dlt698'` 全绿，`run_guard_selftest.py`（含 `check_no_dynamic_memory.py`）全绿。
3. ASAN 下 `test_dcu_698_stat` 0 错误。

## 5. 非目标
- 不在本单修改 `cmd/dcu/dcu_poller.c` 自动调用 `dcu_698_stat_ingest`（留待后续集成接线单）。
- 不修改 `modules/edge_core` 或受保护的 `foundation/include/` 头文件。
