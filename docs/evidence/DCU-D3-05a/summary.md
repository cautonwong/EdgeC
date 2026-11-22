# DCU-D3-05a Class 24 分项事件对象（3000/3005）

基线与分支：`dcu/D3-05a-event-records`，工作树 `/tmp/dcu-d3-05a`，父提交 `4e6f6507`。

## 目的

Class 24 之前只广告 A4/A11/A12。本单按标准 7.3.24 表 193/194 用 OI 专属 A-XDR 编码器暴露 A2/A3/A5/A6/A14，
只广告有真实存储的位；没有存储的位明确回 DAR，不用 0 或当前值冒充。

## 位 → 来源

| 位 | 广告 | 来源 | 编码 |
| --- | --- | --- | --- |
| A2 关联对象属性表 | 是 | `evt->snapshot_oads[0..snapshot_count)` | `array`，每项 OAD（0x51 + 4B）；无关联项时 `array 0` |
| A3 当前记录数 | 是 | 表 1 = `evt->record_count`；表 2–4 未实例化 | `structure{4 × long-unsigned}`；表 2–4 为 0（表示没有记录表） |
| A4 最大记录数 | 是（保持） | `MAX_EVENT_RECORDS` | `long-unsigned` |
| A5 配置参数 | 是 | `evt->threshold`、`evt->delay_s` | `structure{float32 门限, long-unsigned 延时秒}`；延时 > 65535 → DAR 7 |
| A6 事件记录表 1 | 是 | `evt->records[0..record_count)`，上限 `MAX_EVENT_RECORDS` | `array` 的 `structure{index, start, end}`；见下 |
| A11 上报标识 | 是（保持） | `evt->report_flag` | `enum` |
| A12 有效标识 | 是（保持） | `evt->is_enabled` | `bool` |
| A14 时间状态记录表 | 是 | `evt->last_start_time`、`evt->last_end_time` | `array` 1 项 `structure{date_time_s, date_time_s}`；时间为 0 → `NULL` |

A6 记录单元：标准还要求“事件上报状态”数组和 n 个关联对象 Data。`dcu_event_record_t` 只存
`index/start_time/end_time/snapshot_value`，不存这些值，因此：

- `snapshot_count == 0` 时编码 `structure{index, start, end}`（没有 Data 元素，标准有效）。
- `snapshot_count > 0` 时返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`（DAR 7），不重复当前快照值。
- “事件上报状态”数组在本单合同里不编码（无存储），记录结构计数固定为 3。

## 仍屏蔽

| 位 | 原因 |
| --- | --- |
| A1 逻辑名 | 没有实例名存储，未广告 |
| A7/A8/A9 事件记录表 2–4 | `dcu_event_obj_t` 只有一张记录表，表 2–4 未实例化 |
| A10 当前值记录表 | 需要每张表的发生次数与累计时间计数器，结构体没有存储 |
| A13 | 表 193 没有该属性（12 后直接是 14）；类最大属性表按连续编号无法表达该空位，按既有规则回 DAR 5 |
| 方法 2 执行 | 没有实例化语义 |
| 方法 4/5 增删关联对象属性 | 线上非 8000 ACTION 路由属于 D1-20a，当前挂起；现在广告会回 DAR 6。存储 `snapshot_oads` 已存在 |

ACL 仍列出 A8/A9/A10（改动前既有状态，本单只追加 ACL 项）；这些位被对象表屏蔽，线上回 DAR 5。

## 已知限制

- 记录时标来自 `pipe_msg_t.ts`（其定义注明为 `uv_hrtime` 单调纳秒）。编码按“纳秒自 Unix 纪元”换算为
  `date_time_s`，测试用真实纪元纳秒；生产若继续写单调时标，输出的日历时间不具墙钟意义，需要对时或改时标来源。
- 时标为 0 时发 `NULL`，不编 1970-01-01。

## 测试

`tests/test_dcu_698_axdr.c` 新增 `test_event_class24_complex_attrs`，逐字节断言 A2（空/非空）、A3、A5、A6（有/无记录、
快照非空拒绝）、A14（NULL/有时间），并断言屏蔽位 A7 在源层回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）。
既有测试按新广告位更新：对象表掩码、标量入口对复杂属性回 DAR 7、A8 线上 DAR 5。

## 门禁

见 `tests.log` 与 `diffstat.txt`。
