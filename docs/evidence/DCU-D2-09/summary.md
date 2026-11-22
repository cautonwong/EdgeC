# DCU-D2-09 DL/T 698 搜表 6002 / 6003

分支 `dcu/D2-09-search-meter`，工作树 `/tmp/dcu-d2-09`，基线 `a3f8d40f`。

## 标准形状

来源 `docs/698_parsed.md`：7.3.11 集合接口类在 4297 行，表 A.7 在 8120 行。

| 对象 | 标准定义 | A-XDR / 线格式 |
| --- | --- | --- |
| 6002 搜表，IC 11（集合接口类） | 表 A.7 @8219 | 属性 2 = `array 一个搜表结果`；5 = 跨台区结果数组；6 = 所有搜表结果记录数 `long-unsigned`；7 = 跨台区记录数 `long-unsigned`；8 = 配置结构；9 = 每天周期搜表参数数组；10 = `enum{空闲(0), 搜表中(1)}` |
| 6002 方法 127 | 表 A.7 @8248 | 参数 `long-unsigned`，单位分钟；0 = 不限时间直至搜表结束 |
| 6002 方法 128 | 表 A.7 @8251 | 参数 NULL，清空搜表结果 |
| 6002 方法 129 | 表 A.7 @8253 | 参数 NULL，清空跨台区搜表结果 |
| 6003 一个搜表结果，IC 8 | 表 A.7 @8255 | `structure{通信地址 TSA, 所属采集器地址 TSA, 规约类型 enum, 相位 enum, 信号品质 unsigned, 搜到的时间 date_time_s, 搜到的附加信息 array}` |

7.3.11 的通用集合接口类同时定义 A3 当前元素个数（`long-unsigned`）。表 A.7 为 6002 另列 A6 作为“所有搜表结果记录数”。工单文字写“A3 = 记录数”，标准把 6002 的记录数写成 A6；本实现两者都暴露，读同一个 RAM 计数，不制造两个会漂移的值。

## 实现

| 位 | 行为 | 依据 |
| --- | --- | --- |
| A2 | `array`，元素个数等于结果记录数。产品没有下行搜表驱动，结果集恒为空数组（`array(0)`） | 表 A.7 @8219 |
| A3 / A6 | `long-unsigned` 记录数；无结果时为 0 | 7.3.11 / 表 A.7 @8221 |
| A10 | `enum`：空闲 0 / 搜表中 1 | 表 A.7 @8244 |
| 127 | 置搜表中，记下时长与单调起始时刻；已在搜表中回 DAR 3，不静默重开 | 表 A.7 @8248 |
| 128 | 清空 RAM 计数；结果集本来为空，操作幂等 | 表 A.7 @8251 |
| 129 | 产品没有跨台区结果集，清空无对象可清，回成功；不等同于清 A2 | 表 A.7 @8253 |
| 5 / 7 / 8 / 9 | 未暴露。产品没有跨台区搜表与周期搜表能力，类/OI 定义据此回 DAR 5 | 表 A.7 @8220-8243 |

搜表完成走现有 `dcu_node_pump()` 定时器路径：`dcu_node_pump()` 每轮调用 `dcu_discovery_tick()`，时长到期后状态从搜表中回到空闲。没有第二个定时器，也没有旁路状态机。时长为 0 表示不限时；由于没有扫描引擎能宣告结束，状态会一直停在搜表中，直到进程重启。

## 6003 目录决策

6003 是 `一个搜表结果` 结构（IC 8），由 6002 A2 的元素类型定义，不是可独立寻址的对象。

- 生成目录 `DCU_698_OI_CATALOG.{md,json}` 保留 6003 行，class 改为 8，与表 A.7 一致。
- 对象表删除 6003 行；Class 20 ACL 删除 6003 行。主站 GET 6003 回 DAR 6（对象不存在），不再看到一个永远失败的对象。
- 6002 A2 的结果元素仍按 6003 结构编码；产品没有扫描驱动时数组为空。

## 无假值

- 没有搜到任何表时：A2 为空数组，A3/A6 为 0，不返回占位行。
- 产品没有扫描驱动，也没有搜表结果存储表；状态与计数只在 RAM，进程重启即复位。这一点同时写在 `dcu_discovery.h`、`dcu_node.c` 和 `docs/design/DCU_698_OAD_STATUS.md` 4.9。
- 若计数非 0 而代码没有真实的 6003 记录可编码，A2 回 DAR 2，不编造通信地址、信号品质等硬件字段。当前没有任何路径会把计数推到非 0。
- 重复启动 127 回 DAR 3（`EDGE_SLAVE_SRC_DENIED`），文档与测试都固定这一选择。

## 测试

- `tests/test_dcu_698_object_model.c`
  - `test_discovery_start_completes_on_duration_tick`：未到期仍在搜表，到期回空闲。
  - `test_discovery_duplicate_start_is_denied`：重复启动 DAR 3，时长与起始时刻不变。
  - `test_discovery_duration_zero_has_no_timeout`：0 不限时。
  - `test_discovery_clear_and_restart_with_no_results`：清空后可重启，结果计数为 0。
  - `test_discovery_exposes_results_count_and_status`：A3/A6/A10 的标量类型与读数。
  - `test_discovery_oi_attributes_beyond_class_table`：A5..A10 由表 A.7 定义，A11 未定义。
- `tests/test_dcu_698_axdr.c`
  - `test_discovery_standard_attribute_shapes`：A2 空数组、A3/A6 为 0、A10 空闲；启动后 A10 搜表中；到期回空闲后 A2/A3 仍为空/0；A5/A8 回 DAR 5，A11 回 DAR 4。
  - `test_discovery_action_start_duplicate_and_clear`：127 参数类型、重复启动 DAR 3、128 参数 NULL 与错误类型。
- `tests/test_dcu_698_catalog.c`：对象 mask 不得超过 7.3 类表或附录 A 的 OI 属性上限。
- `tests/test_dcu_advanced_modules.c`：路由层启动、tick 完成、A3/A6 为 0、6003 不再是对象。
- 验收 `acceptance/dcu/tests/test_dcu_northbound.py::test_master_starts_search_and_reads_standard_result_shapes`，用例 `D_00_00_02_000020`：通过从站启动搜表，读 A2 空数组、A6 为 0、A10 为搜表中。

## 验证

- 干净 Release 构建 0 warning，`check_build_warnings.py` 通过。
- 全量 `ctest --output-on-failure`：341/341（串行；并行时 `test_arbiter` 和 `app_platform_e2e` 会因本机其它工作树负载超时，单跑均通过）。
- ASAN/UBSAN 相关测试见 `tests.log`。
- `run_guard_selftest.py`、`./test.sh --omnis dcu`、各项守卫的结果见 `tests.log`、`runtime.log`。

## 标准歧义

1. 工单写 “A3 = 记录数”，表 A.7 写 A6 = 所有搜表结果记录数；7.3.11 的 A3 是通用“当前元素个数”。实现同时提供 A3 与 A6，两者读同一个计数，避免按工单文字改标准。
2. 6002 A2 与 A5 都按“结果数组”定义，A5 是跨台区结果；产品没有跨台区能力，A5 不暴露，方法 129 只做空操作。
3. 标准没有“搜表完成”状态，只有空闲/搜表中；时长为 0 时没有扫描引擎可以宣告结束，实现保持搜表中。
