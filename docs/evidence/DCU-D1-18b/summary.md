# DCU-D1-18b 3000/3005 按 Class 24 重排属性号

基线与分支：`dcu/D1-18b-event-class24`，工作树 `/tmp/dcu-d1-18b`，父提交 `8acf47fe`（D1-18a 合并后）。

## 问题

3000/3005 的 `attr_mask` 直接沿用了 Class 7 的编号（2/3/4/5/6/8/9/10），处理器返回的也是 Class 7 的语义。主站按 7.3.24 表193读 3000 时，A4 拿到记录条数（应为最大记录数）、A6 拿到门限（应为记录表 1），A2/A3/A10 拿到错误结构或无标量表示。

## 处理结果（两个 OI 相同）

| 属性 | 标准含义（7.3.24） | 处理 | 线上表现 |
| ---: | --- | --- | --- |
| 1 | 逻辑名 | 关闭 | DAR 5 |
| 2 | 关联对象属性表（array OAD） | 关闭 | DAR 5；数组编码未落地（D3） |
| 3 | 当前记录数（四表 structure） | 关闭 | DAR 5；只有一张记录表（D3） |
| 4 | 最大记录数（long-unsigned） | 暴露 | long-unsigned = `MAX_EVENT_RECORDS` |
| 5 | 配置参数（structure） | 关闭 | DAR 5。3000 需要电压上限/下限与电流下限三路门限，只有单门限；3005 是 `{电流下限 dl(-4), 延时 unsigned}`，结构通道未落地（D3） |
| 6-9 | 事件记录表 1-4（array） | 关闭 | DAR 5（D3） |
| 10 | 当前值记录表（array {次数, 累计秒}） | 关闭 | DAR 5；累计秒未存储（D3） |
| 11 | 上报标识（enum 0-3） | 暴露读写 | enum 0x16，写拒绝 0-3 之外的值 |
| 12 | 有效标识（bool） | 暴露读写 | boolean |
| 13 | 失压统计（structure） | 关闭 | DAR 5；次数/累计秒未存储（D3） |
| 14 | 时间状态记录表（array {发生, 结束}） | 关闭 | DAR 5（D3） |

方法 1（复位）保留：真实清空记录表、记录数与最近发生/结束时间，不触碰关联对象属性表（表194 只要求清记录）。

`attr_mask` 由 `{2,3,4,5,6,8,9,10}` 改为 `{4,11,12}`；`method_mask` 仍为 `{1}`。

## 北向拒绝语义

`dcu_698_value_for_oad()` 与 `dcu_north_accept_axdr()` 统一按对象表分三层：

- OI 不在对象表 → `EDGE_SLAVE_SRC_NOT_FOUND`（DAR 6）；
- 类定义了属性号、但该实例 `attr_mask` 未声明 → `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）；
- 类里没有这个属性号 → `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4）。

类定义上限来自 `dcu_698_class_defines_attr()`（`dcu_698_object_model.c`），与 `tests/fixtures/dcu_698_catalog_expected.h` 的类表由 `tests/test_dcu_698_catalog.c` 逐类逐属性号对拍。

`k_scalars[]` 删除 3000/3005 的 5、6、8、9 旧行，新增 A11（`VK_ENUM`）与 A12（`VK_BOOL`），不可达旧行不能再次被读。

## 改动文件

- `cmd/dcu/dcu_698_object_model.c/.h`：3000/3005 行、`_oi_event_read`/`_oi_event_write`、`dcu_698_class_defines_attr`
- `cmd/dcu/dcu_698_var.c`：A11/A12 标量行、删除旧行、属性拒绝分级
- `cmd/dcu/dcu_698_axdr.c`：写入路径同样分级、enum 编码
- `cmd/dcu/dcu_router.h`：`dcu_event_obj_t` 字段注释同步到 Class 24 编号
- `tests/test_dcu_698_object_model.c`：A4/A11/A12 读、写、关闭位、方法 1 复位
- `tests/test_dcu_698_value_table.c`：A4/A11/A12 类型标签、关闭位 DAR 5、类外属性 DAR 4
- `tests/test_dcu_698_axdr.c`：线上 A11/A12 值、A6 的 DAR 5、写入路径 DAR 4；ACL 增加 3000 条目
- `tests/test_dcu_698_catalog.c`：类属性上限与目录一致
- `tests/test_dcu_advanced_modules.c`：事件记录断言改为直接读 `evt_3000.records[0]`
- `docs/design/DCU_698_OBJECT_MODEL.md`：Class 24 已落地/关闭清单与 DAR 语义

## 门禁

- 干净构建 + `check_build_warnings.py`：0 warning
- `ctest -j`：335/335 通过
- ASAN：8/8 相关目标通过
- `run_guard_selftest.py`：全部通过
- 验收 `test.sh --omnis dcu`：dcu-acceptance 3/3、dcu-northbound 9/9
- `git diff --check`：干净
