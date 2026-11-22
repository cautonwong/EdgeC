# DCU-D3-05a Class 24 分项事件对象（3000/3005）

基线与分支：`dcu/D3-05a-event-records`，工作树 `/tmp/dcu-d3-05a`，父提交 `4e6f6507`。
原始提交 `26ce419e`；本文件按 follow-up 提交更新。

## 目的

Class 24 之前只广告 A4/A11/A12。本单按标准 7.3.24 表 193/194 用 OI 专属 A-XDR 编码器暴露
A2/A3/A4/A11/A12，只广告有真实存储的位；没有存储的位明确回 DAR，不用 0 或当前值冒充。

## 位 → 来源

| 位 | 广告 | 来源 | 编码 |
| --- | --- | --- | --- |
| A2 关联对象属性表 | 是 | `evt->snapshot_oads[0..snapshot_count)` | `array`，每项 OAD（0x51 + 4B）；无关联项时 `array 0` |
| A3 当前记录数 | 是 | 表 1 = `evt->record_count`；表 2–4 未实例化 | `structure{4 × long-unsigned}`；表 2–4 为 0（表示没有记录表） |
| A4 最大记录数 | 是（保持） | `MAX_EVENT_RECORDS` | `long-unsigned` |
| A11 上报标识 | 是（保持） | `evt->report_flag` | `enum` |
| A12 有效标识 | 是（保持） | `evt->is_enabled` | `bool` |

## 未暴露的位

| 位 | 原因 |
| --- | --- |
| A1 逻辑名 | 没有实例名存储 |
| A5 配置参数 | 表A.4 按 OI 定义不同结构：3000 为 `{电压触发上限 long-unsigned, 电压恢复下限 long-unsigned, 电流触发下限 double-long, 判定延时时间 unsigned}`，3005 为 `{电流触发下限 double-long, 判定延时时间 unsigned}`；`dcu_event_obj_t` 只有单个 `threshold`/`delay_s`，无法诚实编码 |
| A6 事件记录表 1 | 记录时标是单调纳秒，编成日历时间会伪造墙钟；见下 |
| A7/A8/A9 事件记录表 2–4 | `dcu_event_obj_t` 只有一张记录表，表 2–4 未实例化 |
| A10 当前值记录表 | 需要每张表的发生次数与累计时间计数器，结构体没有存储 |
| A13 | 表 193 没有该属性（12 后直接是 14）；类最大属性表按连续编号无法表达该空位，按既有规则回 DAR 5 |
| A14 时间状态记录表 | 同 A6：`last_start_time`/`last_end_time` 也是单调纳秒 |
| 方法 2 执行 | 没有实例化语义 |
| 方法 4/5 增删关联对象属性 | 线上非 8000 ACTION 路由属于 D1-20a，当前挂起；现在广告会回 DAR 6。存储 `snapshot_oads` 已存在 |

A6/A14 stay masked because event record times are monotonic `uv_hrtime` ns
（`cmd/dcu/dcu_node.c:419`、`cmd/dcu/dcu_event_monitor.c:203-227`）；a civil `date_time_s` would be
fabricated. Needs event records to store wall-clock ns (separate task) before exposing.
A6 的编码分支已删除：`snapshot_count > 0` 的旧拒绝路径也随之消失。

ACL 仍列出 A8/A9/A10；本单曾追加 A14。这些位被对象表屏蔽，线上统一回 DAR 5。

## 测试

`tests/test_dcu_698_axdr.c` 的 `test_event_class24_complex_attrs` 逐字节断言 A2（空/非空）、A3；
A5 在 3000 与 3005 上都回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`，A6 与 A14 同理（线上 DAR 5）。
SET 路径对 A6 同样断言类不匹配。
既有测试同步：对象表掩码为 `{2,3,4,11,12}`，标量入口把 A5/A6/A14 归入屏蔽位，advanced_modules
注释改为 A6–A9 均未暴露。

## 门禁

见 `tests.log`。`diffstat.txt` 记录原始提交 `26ce419e` 的代码差异。
