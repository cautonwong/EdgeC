# DCU-D3-04a Class 24 事件墙钟时标

基线 `2f502b50`（D2-03 已合并；分支从 `b9c0d7c7` rebase 到此），分支
`dcu/D3-04a-event-wallclock`，工作树 `/tmp/dcu-d3-04a`。

## 目的

Class 24 事件对象（OI 3000 失压 / OI 3005 过流）的事件记录原来只存 `uv_hrtime()`
单调纳秒，A6/A14 无法据此编出真实的 `date_time_s`，D3-05a 因此屏蔽了这两个属性。
本单在生成记录时取墙钟，和单调值并列保存，A6/A14 只从墙钟编码，然后重新暴露。

## 评审裁决（本轮落地）

1. **设备本地民用时间，取代 UTC。** 表49 的 `date_time_s` 不带时区；中国主站必须看到
   设备本地民用时间。`dcu_698_put_date_time_s` 与 `dcu_698_date_time_s_to_epoch`
   改为经 `localtime_r` / `mktime` 转换，进程 `TZ` 即设备配置的时区。两者仍是冻结记录、
   Class 24 A6/A14 以及后续 `date_time_s` 使用者的唯一实现。**运维要求：部署设备必须
   配置 TZ**，否则主站读到的是 UTC。
2. **A6 未结束结束时间用表49 非法时标，而不是 NULL。** 表194 的 A6 记录单元没有 NULL
   槽位；表49 非法编码 = 年 `FFFFH`，其余字段 `FFH`，共 `1C FF FF FF FF FF FF FF`。
   时标存在但不可民用编码时失败关闭（-1）。A14 的最近一次发生/结束时间按表194 明确
   允许 NULL，未发生/未结束仍编 `0x00`。

## 时间源

- `clock_gettime(CLOCK_REALTIME)` 整秒，在 `dcu_event_monitor.c` 的 logger 阶段每条记录
  取一次。`date_time_s` 分辨率 1 s，所以只存 `tv_sec`。
- `uv_hrtime()` 继续供监测阶段的延时倒计时使用（`ev->ts`）；记录里的 `mono_ns` 是单调
  副本，只作存在性判定与审计，不参与 A6/A14 编码。
- 读时钟失败或值不在 [2000, 2100) 时 `epoch_s` 无效，A6/A14 回 `TYPE_MISMATCH`
  （线上 DAR 7），不把 0 或单调值编成公历日期。

## 存储 schema

`cmd/dcu/dcu_router.h`：

```c
typedef struct { uint64_t mono_ns; uint64_t epoch_s; } dcu_event_stamp_t;
```

- `dcu_event_record_t.start_time/end_time`、`dcu_event_obj_t.last_start_time/last_end_time`
  改为 `dcu_event_stamp_t`。
- 每条记录新增 `snapshot_len[8]` 与 `snapshot[8][64]`，保存生成记录时抓到的关联对象
  A-XDR 值（A6 记录单元的 Data）。
- `event_history` 的 `start_time/end_time` 从此写墙钟整秒（恢复记录的开始时间沿用触发
  时的 `epoch_s`）。

## A6/A14 映射（表194）

A6 事件记录表 1：`array(record_count) < structure { 序号 double-long-unsigned;
发生时间 date_time_s; 结束时间 date_time_s; 上报状态 array(0); n×关联数据 Data } >`。
结束时间在事件未结束时编表49 非法时标 `1C FF FF FF FF FF FF FF`。关联数据按 A2 的
OAD 顺序输出，抓拍不到编 NULL；上报状态没有按通道存储，编空数组。

A14 时间状态记录表：`array(1) < structure { 最近一次发生时间 date_time_s 或 NULL;
最近一次结束时间 date_time_s 或 NULL } >`，`mono_ns == 0` 表示未发生/未结束，编 `0x00`。

表 2 到 4 未实例化；A7/A8/A9 仍屏蔽。

## 旧行迁移规则

`event_history` 旧行存的是单调纳秒。规则是**不迁移、不渲染，旧行一律无效**：

- 该表今天没有产品读取路径（只有建表、`INSERT OR REPLACE` 和 `verify_db.py` 计数）。
- 幅度不能可靠区分两种单位（开机 1 到 4 秒的纳秒值落在合法 epoch 秒区间内），所以
  不能用数值范围做迁移依据。未来任何读取必须先加单位/版本字段识别旧行。
- `dcu_698_epoch_is_civil()` 是最后一道拒绝；测试写入一条旧代码风格的单调纳秒行，断言
  它过不了该判定且 A6 编码回 `TYPE_MISMATCH`。

## 仍屏蔽

- A5 配置参数：表 A.4 对 3000/3005 要求不同字段结构，对象只存单门限/延时，不能诚实编码。
- A7/A8/A9 表 2 到 4 未实例化；A10 没有按表的次数与累计时间；A13 不在表 193。
- 关闭位仍按 DAR 5（类定义有、实例未声明）拒绝。

## 重新暴露顺序

先完成时间源、编码与测试，再在 `dcu_698_object_model.c` 打开 A6（BIT 6）与 A14
（BIT 14）；A5 保持关闭。ACL `k_attrs_event` 原本已列 A6/A14。

## 测试

- `test_dcu_698_axdr.c::test_event_class24_record_and_status_times`：A6/A14 逐字节
  （TZ 固定 `CST-8`，epoch 1700000000 → `2023-11-15 06:13:20` → `1C 07E7 0B 0F 06 0D 14`）、
  A6 未结束结束时间 = `1C FF FF FF FF FF FF FF`、关联数据、epoch 0 与单调纳秒冒充都回
  `TYPE_MISMATCH`。组 setup/teardown 设置并恢复 `TZ`。
- `test_dcu_698_axdr.c::test_event_class24_complex_attrs`：A14 未发生/未结束编 NULL；
  有记录但无墙钟的 A6 回 `TYPE_MISMATCH`。
- `test_dcu_advanced_modules.c::test_dcu_advanced_features_dispatch`：真实监测 + logger
  管线；`epoch_s` 与 `time(NULL)` 相差不超过 5 s；线上 A6 解码回同一 epoch；落库行一致；
  旧代码风格的单调纳秒行被判为非法。组 setup/teardown 设置并恢复 `TZ`。
- `test_dcu_698_record.c`：冻结记录字节与区间窗口按设备本地时间展开（`TZ=CST-8`），
  Selector1 精确匹配、Selector2 前闭后开窗口；组 setup/teardown 设置并恢复 `TZ`。
- `test_dcu_698_object_model.c` / `test_dcu_698_value_table.c`：mask 与复位期望同步。

## 未决问题

1. ~~`date_time_s` 按 UTC 展开~~：已裁决为设备本地民用时间，部署需配置 TZ。
2. ~~A6 未结束记录用 NULL 还是非法 `date_time_s`~~：已裁决用表49 非法时标；A14 保持 NULL。
3. 事件对象是 RAM-only，重启读回不存在；没有发明持久化路径。
4. `event_history` 主键仍是 `(meter_address, oi, start_time)`，整秒后同秒同表同 OI 会覆盖。

## 门禁

见 `tests.log`（含本轮修订后的完整运行）与 `runtime.log`（验收）、`diffstat.txt`。
