# DCU 698 OAD 状态矩阵与修复队列

> 日期：2026-11-22　状态：只读审计，未改代码
> 范围：`cmd/dcu/dcu_698_object_model.c` 的 `g_objects[]`（17 行）、`dcu_698_var.{c,h}`、`dcu_north_adapter.c`、`dcu_698_axdr.c`、`dcu_storage.{h,c}`、`dcu_node.c`、`dcu_poller.*`、`dcu_security.*`（本仓库没有 `dcu_esam.*`）、`dcu_discovery.*`
> 标准：`docs/698_parsed.md` 7.3、7.4、附录 A（A.1/A.3/A.4/A.6/A.7/A.9/A.12/A.13）
> 设计：`docs/design/DCU_698_OBJECT_MODEL.md`

## 1. 结论摘要

对象表共 **17 行**，`attr_mask` / `method_mask` 共声明 **56 个位**。逐位核对后：

| 状态 | 数量 | 含义 |
| --- | ---: | --- |
| real | 0 | 真值/真写/真方法，且类型、量纲、属性号与标准一致 |
| partial | 37 | 有真实来源，但类型/量纲/属性语义/持久化至少一项不符合标准 |
| stub | 12 | 位已声明，处理器明确返回 `-2`/`-4`/TEMPORARY，或实现是模拟 |
| unimplemented | 3 | 位已声明，但产品装配下对象实例为 NULL，调用必然返回 `-1` |
| masked-but-undefined | 4 | mask 声明了标准类/OI 未定义的属性号或方法号 |
| **合计** | **56** | |

三个系统性结论：

1. **没有任何一位是 standard-correct 的。** 变量类全部返回未换算的工程值；对象类大量属性号照搬 Class 7 编号；结构/数组属性在北向 A-XDR 通道上不可达。
2. **每表寻址没有实现。** `dcu_698_var_read_value()` 固定用 `device_id=0` 读影子（`cmd/dcu/dcu_698_var.c:53`），`dcu_698_var_read()` 忽略 `elem_idx`（同文件 186 行），`dcu_698_value_for_oad()` 完全不看 OAD 低字节。所有 `DCU_OBJ_PER_METER` 行实际都读 0 号表。
3. **F100 仍挂成 NULL；6002 已装配（D2-09）。** `dcu_node_create_in()` 把 `esam` 传 NULL，F100 的读写和方法因此返回 `-1`；6002 搜表对象由 `dcu_node_create_in()` 持有并传入，`dcu_node_pump()` 推进其状态。6003 不再是独立对象。

## 2. 状态口径

- `real`：处理器存在，取真实数据源，且类型、量纲、属性号、方法与 7.3 / 附录 A 一致。
- `partial`：处理器存在且有真实数据源，但类型、量纲、属性号、持久化或寻址至少一项错。
- `stub`：位已声明，处理器明确拒绝（`-2`/`-4`/TEMPORARY），或实现是模拟/演示。
- `unimplemented`：位已声明，但产品中对象实例为 NULL 或处理器不可达。
- `masked-but-undefined`：mask 置位，但标准类定义或该 OI 的附录 A 行没有这个属性/方法号。

## 3. 逐位状态矩阵

| OI | Class | 位 | 类型 | 处理器 | 状态 | 值来源 | 测试 | 备注 |
| --- | ---: | --- | --- | --- | --- | --- | --- | --- |
| 6000 | 10 | A2 | attr | `_oi_6000_read` | partial | `dcu_inventory.count`（RAM） | `test_dcu_698_object_model::test_mask_rejects_missing_attr_and_method`、`test_dcu_router::test_dcu_router_dispatch`、`test_dcu_698_value_table::test_object_model_scalar_type_tags`、`test_dcu_698_axdr::test_product_axdr_rejects_small_buffer_and_complex` | 标准 A.7 属性 2 是 `array 采集档案配置单元`；这里返回 `long-unsigned` 计数 |
| 6000 | 10 | M1 | method | `_oi_6000_execute` | partial | `dcu_inventory_add`（RAM） | `test_dcu_698_object_model::test_dispatch_read_write_call` | Class 10 方法 1 是“复位”；这里实现为 Add |
| 6000 | 10 | M2 | method | `_oi_6000_execute` | stub | 无 | `test_dcu_router::test_dcu_router_dispatch`、`test_dcu_698_object_model::test_mask_rejects_missing_attr_and_method` | 明确 `-4`；标准 6000 方法 2 不存在 |
| 6000 | 10 | M3 | method | `_oi_6000_execute` | stub | 无 | 同上 | 明确 `-4`；标准 6000 方法 3 不存在 |
| 6000 | 10 | M4 | method | `_oi_6000_execute` | masked-but-undefined | `dcu_inventory_remove`（RAM） | 无 | Class 10 没有方法 4；产品用它做按地址删除 |
| 6000 | 10 | M5 | method | `_oi_6000_execute` | masked-but-undefined | `memset(inv->meters)`（RAM） | 无 | Class 10 没有方法 5；产品用它清空档案 |
| 6000 | 10 | M127 | method | `_oi_6000_execute` | partial | `dcu_inventory_add`（RAM） | `test_dcu_698_object_model::test_legacy_methods_127_and_128_dispatch`、`test_dcu_router::test_dcu_router_dispatch` | 标准 Add；只改 RAM，不写 `meter_inventory`，重启即丢 |
| 6000 | 10 | M128 | method | `_oi_6000_execute` | partial | `dcu_inventory_remove`（RAM） | `test_dcu_698_object_model::test_legacy_methods_127_and_128_dispatch` | 标准 128 是 AddBatch；这里实现为按地址删除 |
| 6012 | 10 | A2 | attr | `dcu_698_task_encode_axdr` | ok | `dcu_poller_t.tasks[16]`（DB 首启播种/重启装载） | `test_dcu_698_tasks::test_a2_bytes_are_exact` | 标准 A.7 属性 2 是 `array 任务配置单元`，按 task_id 升序 |
| 6012 | 10 | M1 | method | `dcu_698_acq_6012_execute` | ok | 运行态游标/`last_run_time`/monitor 清零 | `test_dcu_698_object_model::test_task_scheduler_reset_and_add` | Class 10 复位只清运行态；配置与落库由 129 Clear 负责 |
| 6012 | 10 | M127 | method | `dcu_698_accept_action_axdr` → `dcu_task_store_apply` | ok | `dcu_task_config` + poller tasks | `test_dcu_698_tasks::test_add_task_changes_plan_and_persists` | 标准 Add/upsert；整批事务化，未实现字段 DAR 2 |
| 6012 | 10 | M128 | method | 同上 | ok | `dcu_task_config` + poller tasks | `test_dcu_698_tasks::test_delete_stops_task` | 标准 Delete；缺 ID 回 DAR 6 |
| 6012 | 10 | M129 | method | 同上 | ok | `dcu_task_config` + poller tasks | `test_dcu_698_tasks::test_clear_persists_across_reload` | 标准 Clear；保留播种标记，重启保持为空 |
| 6012 | 10 | M130 | method | 同上 | ok | `dcu_task_config` + poller tasks | `test_dcu_698_tasks::test_status_update_stops_and_resumes` | 标准 Update{id,status}；status 1 恢复时重置时钟 |
| 6013 | 8 | A2 | attr | `dcu_698_task_encode_axdr` | ok | 6012 A2 的单条结构 | `test_dcu_698_tasks::test_a2_bytes_are_exact` | 元素索引 = task_id；0 = 最小现有 ID |
| 0010 | 1 | A2 | attr | `dcu_698_var_read` | partial | DataHub 影子 `OAD_ACTIVE_ENERGY`，device 0 | `test_dcu_698_var::test_six_ois_values_and_type_tags`、`test_legacy_scalar_output_types`、`test_energy_attr2_and_attr4_type_tags`、`test_dcu_698_axdr::test_product_variable_axdr_types` | 标签 `double-long-unsigned` 对，但 `raw=trunc(工程值)`；标准 scale −2，应为 `1234567` 而不是 `12345` |
| 0010 | 1 | A4 | attr | `dcu_698_var_read` | partial | 同一个影子值 | `test_dcu_698_var::test_energy_attr2_and_attr4_type_tags`、`test_dcu_698_value_table::test_scalar_rejects_64bit_raw`、`test_dcu_698_axdr::test_product_variable_axdr_types` | 标签 `long64-unsigned` 对，但值等于 A2 截断值；标准 scale −4，且高精度应是更细的值 |
| 0020 | 1 | A2 | attr | `dcu_698_var_read` | partial | DataHub 影子 `OAD_REACTIVE_ENERGY`，device 0 | 同 0010 A2 | 注释写“反向有功”，标准 0020 是反向有功；换算同 0010 |
| 0020 | 1 | A4 | attr | `dcu_698_var_read` | partial | 同一个影子值 | 同 0010 A4 | 同 0010 A4 |
| 2000 | 3 | A2 | attr | `dcu_698_var_read` | partial | DataHub 影子 `OAD_VOLTAGE`，device 0 | `test_dcu_698_var::test_six_ois_values_and_type_tags`、`test_legacy_scalar_output_types`、`test_dcu_698_axdr::test_product_variable_axdr_types` | 标准 A.3：`long-unsigned`、V、scale −1；这里 `float32` 且未换算 |
| 2001 | 3 | A2 | attr | `dcu_698_var_read` | partial | DataHub 影子 `OAD_CURRENT`，device 0 | 同 2000 A2 | 标准 A.3：`double-long`、A、scale −3；这里 `float32` |
| 2004 | 4 | A2 | attr | `dcu_698_var_read` | partial | DataHub 影子 `OAD_ACTIVE_POWER_TOTAL`，device 0 | 同 2000 A2 | 标准 A.3：`double-long`、W、scale −1；这里 `float32`；标准属性 2 是 total+A+B+C 数组 |
| 200A | 4 | A2 | attr | `dcu_698_var_read` | partial | DataHub 影子 `OAD_POWER_FACTOR_TOTAL`，device 0 | `test_dcu_698_var::test_power_factor_and_frequency_are_separated`、`test_six_ois_values_and_type_tags` | 标签 `long` 对；但 `raw=trunc(0.95)=0`，主站读到 0 而不是 950（scale −3） |
| 200F | 6 | A2 | attr | `dcu_698_var_read` | partial | DataHub 影子 `OAD_GRID_FREQUENCY`，device 0 | `test_dcu_698_var::test_power_factor_and_frequency_are_separated`、`test_six_ois_values_and_type_tags`、`test_dcu_698_axdr::test_product_variable_axdr_types` | 标签 `long-unsigned` 对；`raw=50` 而不是 5000（scale −2） |
| 5004 | 9 | A2 | attr | `_oi_5004_read` | partial | SQLite `count(dcu_freeze_history)` | `test_dcu_698_value_table::test_object_model_scalar_type_tags`、`test_dcu_698_axdr::test_product_axdr_rejects_small_buffer_and_complex`、`test_dcu_advanced_modules::test_dcu_advanced_features_dispatch` | 标准 A.6 / 7.3.9 属性 2 是冻结记录数组；这里返回记录条数；无 GetRecord/RSD |
| 5004 | 9 | M3 | method | `_oi_5004_execute` | stub | 无 | 无 | 明确 `-4`；触发一次冻结未实现 |
| 5004 | 9 | M4 | method | `_oi_5004_execute` | stub | 无 | 无 | 明确 `-4`；添加冻结对象未实现 |
| 5004 | 9 | M5 | method | `_oi_5004_execute` | stub | 无 | 无 | 明确 `-4`；删除冻结对象未实现 |
| 5004 | 9 | M7 | method | `_oi_5004_execute` | stub | 无 | 无 | 明确 `-4`；批量添加未实现 |
| 5004 | 9 | M8 | method | `_oi_5004_execute` | stub | 无 | 无 | 明确 `-4`；清空关联对象表未实现 |
| 8000 | 13 | M1 | method | `_oi_8000_execute` | partial | `dcu_inventory_by_slot` + `dcu_node_execute_control`（真实 645 控制帧） | `test_dcu_router::test_dcu_router_dispatch`、`test_dcu_698_object_model::test_facade_and_table_api_agree` | Class 13 方法 1 是“复位”；这里实现为按 slot 拉/合闸；`result` 被当作输入动作字节 |
| 8000 | 13 | M127 | method | `_oi_8000_execute` | stub | 无 | `test_dcu_698_axdr::test_action_127_128_null_param` | 参数解码通过，返回 TEMPORARY；产品无告警输出 |
| 8000 | 13 | M128 | method | `_oi_8000_execute` | stub | 无 | 同上 | 参数解码通过，返回 TEMPORARY |
| 8000 | 13 | M129 | method | `_oi_8000_execute` | stub | `dcu_action_param_t` 解码 | `test_dcu_698_axdr::test_action_129_trip_param` | 参数真实解码；无继电器 OAD→电表地址映射，返回 TEMPORARY |
| 8000 | 13 | M130 | method | `_oi_8000_execute` | stub | 同上 | `test_dcu_698_axdr::test_action_130_close_param` | 同上 |
| 8000 | 13 | M131 | method | `_oi_8000_execute` | stub | 同上 | `test_dcu_698_axdr::test_action_131_password_close_param` | 同上；密码字段只校验类型，不校验内容 |
| F100 | 21 | M1 | method | `_oi_f100_execute` | unimplemented | `dcu_security_authenticate`（测试实例） | `test_dcu_advanced_modules::test_dcu_advanced_features_dispatch` | 生产传 `esam=NULL` 必返 `-1`；测试里是 XOR 模拟“鉴权”；Class 21 方法 1 实际是复位 |
| F200 | 22 | A2 | attr | `_oi_f200_read` | partial | 静态 `g_f200_channel_cfg.baudrate` | `test_dcu_698_object_model::test_dispatch_read_write_call`、`test_dcu_advanced_modules::test_dcu_advanced_features_dispatch`、`test_dcu_698_axdr::test_product_accept_set_paths` | 标准 A.13 属性 2 是只读 `array 端口`；这里返回波特率标量；写只改 RAM，不落盘、不作用于 UART |
| F200 | 22 | A3 | attr | `_oi_f200_read` | partial | 静态 `g_f200_channel_cfg.parity` | `test_dcu_698_value_table::test_object_model_scalar_type_tags`、`test_dcu_698_object_model::test_dispatch_read_write_call` | 标准 Class 22 属性 3 是“设备对象数量”；这里返回校验位；配置参数应在属性 4 |
| 6002 | 11 | A2 | attr | `dcu_698_acq_6002_encode` | partial | RAM 计数（恒 0） | `test_dcu_698_axdr::test_discovery_standard_attribute_shapes` | 标准 A.7 是 `array 搜表结果`；产品没有下行搜表驱动，结果集恒空；计数非 0 时回 DAR 2，不编造 6003 字段（D2-09） |
| 6002 | 11 | A3 | attr | `dcu_698_acq_6002_read` | implemented | `dcu_discovery.count` | `test_dcu_698_object_model::test_discovery_exposes_results_count_and_status` | Class 11 当前元素个数（D2-09） |
| 6002 | 11 | A6 | attr | `dcu_698_acq_6002_read` | implemented | `dcu_discovery.count` | 同上 | 表 A.7 所有搜表结果记录数（D2-09） |
| 6002 | 11 | A10 | attr | `dcu_698_acq_6002_read` | implemented | `dcu_discovery.status` | 同上 | 表 A.7 空闲/搜表中（D2-09） |
| 6002 | 11 | M127 | method | `dcu_698_acq_6002_execute` | implemented | `dcu_discovery_start` 状态机 | `test_dcu_698_axdr::test_discovery_action_start_duplicate_and_clear` | 时长单位分钟，0 = 不限时；重复启动 DAR 3；由 `dcu_node_pump()` tick 到期回空闲（D2-09） |
| 6002 | 11 | M128/M129 | method | `dcu_698_acq_6002_execute` | implemented | `dcu_discovery_clear` | 同上 | 128 清结果；129 无跨台区结果可清，回成功（D2-09） |
| 3000 | 24 | A2 | attr | `_oi_event_read` | partial | `dcu_event_obj_t.records[]`（RAM） | `test_dcu_advanced_modules::test_dcu_advanced_features_dispatch`、`test_dcu_router::test_dcu_router_dispatch` | 标准 Class 24 属性 2 是关联对象属性表；这里返回事件记录结构；AXDR 通道不可达 |
| 3000 | 24 | A3 | attr | `_oi_event_read` | partial | `snapshot_oads[]` | 无 | 标准属性 3 是四张记录表的当前记录数结构；这里返回抓拍 OAD 列表 |
| 3000 | 24 | A4 | attr | `_oi_event_read` | partial | `record_count` | `test_dcu_698_value_table::test_object_model_scalar_type_tags` | 标准属性 4 是最大记录数；这里返回当前记录数 |
| 3000 | 24 | A5 | attr | `_oi_event_read` | partial | `MAX_EVENT_RECORDS` | 同上 | 标准属性 5 是配置参数结构；这里返回最大记录数 |
| 3000 | 24 | A6 | attr | `_oi_event_read` | partial | `evt->threshold` | 同上 | 标准属性 6 是事件记录表 1；这里返回判定门限 |
| 3000 | 24 | A8 | attr | `_oi_event_read` | partial | `evt->report_flag` | 同上 | 标准属性 8 是事件记录表 3；上报标识是属性 11 |
| 3000 | 24 | A9 | attr | `_oi_event_read` | partial | `evt->is_enabled` | 同上 | 标准属性 9 是事件记录表 4；有效标识是属性 12 |
| 3000 | 24 | A10 | attr | `_oi_event_read` | partial | `last_start_time` / `last_end_time` | 无 | 标准属性 10 是当前值记录表；时间状态是属性 14 |
| 3000 | 24 | M1 | method | `_oi_event_execute` | partial | `dcu_event_obj_t`（RAM） | 无 | 复位有效，但只清 RAM；不清 SQLite、不清四张记录表结构 |
| 3005 | 24 | A2 | attr | `_oi_event_read` | partial | `dcu_event_obj_t.records[]`（RAM） | `test_dcu_advanced_modules::test_dcu_advanced_features_dispatch` | 同 3000 A2 |
| 3005 | 24 | A3 | attr | `_oi_event_read` | partial | `snapshot_oads[]` | 无 | 同 3000 A3 |
| 3005 | 24 | A4 | attr | `_oi_event_read` | partial | `record_count` | `test_dcu_698_value_table::test_object_model_scalar_type_tags` | 同 3000 A4 |
| 3005 | 24 | A5 | attr | `_oi_event_read` | partial | `MAX_EVENT_RECORDS` | 同上 | 同 3000 A5 |
| 3005 | 24 | A6 | attr | `_oi_event_read` | partial | `evt->threshold` | 同上 | 同 3000 A6 |
| 3005 | 24 | A8 | attr | `_oi_event_read` | partial | `evt->report_flag` | 同上 | 同 3000 A8 |
| 3005 | 24 | A9 | attr | `_oi_event_read` | partial | `evt->is_enabled` | 同上 | 同 3000 A9 |
| 3005 | 24 | A10 | attr | `_oi_event_read` | partial | `last_start_time` / `last_end_time` | 无 | 同 3000 A10 |
| 3005 | 24 | M1 | method | `_oi_event_execute` | partial | `dcu_event_obj_t`（RAM） | 无 | 同 3000 M1 |

## 4. 非显而易见的逐 OI 说明

### 4.1 0010 / 0020：类型对、量纲错、精度假

`k_vars` 的标签（`double-long-unsigned`、`long64-unsigned`）与表 A.1 一致，但 `dcu_698_var_read_value()` 把影子里的工程值直接截断成 `raw`：

```c
out->value = msg.payload.val.f;
out->raw = (int64_t)msg.payload.val.f;   /* cmd/dcu/dcu_698_var.c:57-60 */
```

影子里的值已经是 `线上整数 × scaler`（`dcu_node_record_reading()` 在 `cmd/dcu/dcu_node.c:237-243` 完成换算），所以：
- 0010 属性 2 标准要求 `kWh×100` 的整数；当前返回 `kWh` 整数，差 100 倍。
- 0010 属性 4 标准要求 `kWh×10000`；当前仍返回 `kWh` 整数，差 10000 倍，且与属性 2 同值，不是高精度。
- 测试 `test_energy_attr2_and_attr4_type_tags` 断言两者 `value` 相等，等于把错误固化。

### 4.2 2000 / 2001 / 2004 / 200A / 200F：标签与标准不符

表 A.3 规定：
- 2000 电压：`long-unsigned`，V，scale −1
- 2001 电流：`double-long`，A，scale −3
- 2004 有功功率：`double-long`，W，scale −1
- 200A 功率因数：`long`，scale −3
- 200F 频率：`long-unsigned`，Hz，scale −2

代码的 `k_vars`（`cmd/dcu/dcu_698_var.c:22-30`）把 2000/2001/2004 全设成 `float32`，把 200A/200F 的类型设对但没做 ×1000 / ×100 换算。因此：
- 200A 读 0.95 时 `raw=0`，主站收到 `long 0`。
- 200F 读 50.01 时 `raw=50`，主站收到 50 而不是 5001。
- 2000/2001/2004 的 A-XDR 第一个字节是 0x17（float32），不是标准要求的整数标签。

另外，标准 Class 3/4 的属性 2 是“分相/总分相数组”，不是单相标量。代码把 A/B/C 相拆成属性 3/4/5（`OAD_VOLTAGE_B`、`OAD_CURRENT_C`、`OAD_ACTIVE_POWER_A` 等），但这些属性号在对象表里没有置位，北向 GET 返回 NOT_FOUND。

### 4.3 6000：方法号是两套账

**2026-11-22 D2-02 更新**：产品已按 A.7 实现 6000 方法 127–134 与 6001 A2：
- 广告位只有 127–134；Class 10 的 1–5 不再广告（类定义过，未广告 → DAR 5）。
- Add/AddBatch/Update/Delete/Clear 经 `router_sink_ops_t` 的 archive 端口写 `meter_inventory`，失败回 DAR 2/3，不再只改 RAM。
- 6000 A2 是 `array 采集档案配置单元`；整组放不进 2048 字节 A-XDR 缓冲或超过 255 只表时回 DAR 2。
- 6001 A2 是单个配置单元结构，按 OAD 元素索引（= 配置序号）取。
- 129 `Update` 的附属信息（Annex）没有存储路径：非空时回 DAR 2。

下面两段保留为修复前的历史记录。

对象表同时声明了两套方法：
- 1–5：Class 10 的通用方法号（1 复位、2 执行、3 清空记录表；4/5 Class 10 未定义）。
- 127/128：附录 A.7 的 6000 Add/AddBatch。

实际实现（`cmd/dcu/dcu_698_object_model.c:188-232`）：
- 1 = Add、4 = Delete、5 = Clear，是产品自定义语义，不是 Class 10 语义。
- 127 = Add（对），128 = Delete（标准是 AddBatch，错）。
- 129–134 完全没进 mask。
- Add/Delete/Clear 只改 `dcu_inventory_t` 内存；全仓库没有写 `meter_inventory` 的 INSERT/UPDATE，只有 `dcu_inventory_load_from_db()` 的 SELECT。所以“添加成功”的档案重启即丢。

### 4.4 6012：已按 A.7 实现（D2-03）

- 标准 A.7：属性 2 = `array 任务配置单元`，属性 3 = `array 记录单元`；方法 127 Add、128 Delete、129 Clear、130 Update。
- 代码：属性 2 由 `dcu_698_task_encode_axdr()` 输出 `array<6013 结构>`（按 task_id 升序）；127–130 由 `dcu_698_accept_action_axdr()` 用有界游标严格解码后交给 `dcu_task_store_apply()`。
- 持久化：`dcu_task_config` 表建在既有 `storage->db` 上；首次启动播种任务 1（5 s、方案 1），Clear 后保留播种标记，重启保持为空。
- 未实现的字段（时间窗口、延时、优先级、脚本、非空时段表、月/年 TI、方案类型 2–5）按 DAR 2 拒绝；属性 3 仍未暴露。

### 4.5 5004：把“条数”冒充“冻结表”

- 标准 A.6 / 7.3.9：属性 2 是冻结记录数组，每条含冻结序号、冻结时间、各关联对象数据。
- `_oi_5004_read()` 调用 `count_freezes` 只返回 `uint16_t` 条数（`cmd/dcu/dcu_698_object_model.c:266-281`）。
- 方法 3/4/5/7/8 全部 `-4`，没有触发冻结、没有 RSD/GetRecord。
- 因此北向 698 的 GET 5004 A2 会收到一个 `long-unsigned` 条数，而不是数组。test_dcu_advanced_modules 里的 3 条冻结记录只能通过本地 `dcu_storage_query_freeze()` 验证，北向读不到。

### 4.6 8000：真正能拉闸的方法号不是标准方法号

- 标准 A.9：方法 127 触发告警、128 解除报警、129 跳闸、130 合闸、131 电表明文合闸；属性 2 配置参数、4 告警状态、5 命令状态。
- 代码 `attr_mask` 为空（属性全不暴露）；方法 1 走真实控制下沉（`dcu_node_execute_control()` 组 645 1AH/1BH 帧）。
- 方法 129/130/131 的 A-XDR 参数解码是真实且严格的（`dcu_698_decode_action_param()`），但 `_oi_8000_execute()` 解码后一律返回 TEMPORARY，因为产品没有“继电器 OAD → 电表地址”映射。
- 所以：主站按标准 129 跳闸会得到 DAR 2；按非标准方法 1 才会真的发帧。这是把唯一可用的执行路径挂在错误的方法号上。

### 4.7 F100：模拟 ESAM，且生产未装配

- 标准 Class 21 方法 1 是“复位”；代码把方法 1 实现成 `dcu_security_authenticate()`。
- `dcu_security_authenticate()` 是 XOR 模拟（`param_in[i] ^ 0xA5 ^ i`），不是国密/ESAM 真实运算；却把 `is_authenticated` 置 true 并返回 0。这会让“已鉴权”变成可伪造状态。
- 生产 `dcu_node_create_in()` 传 `esam=NULL`（`cmd/dcu/dcu_node.c:178`），`_oi_f100_execute()` 第一个判断 `!esam` 就返回 `-1`。所以 F100 在生产中不可用；测试 `test_dcu_advanced_modules` 自己 `dcu_security_init()` 后才走通模拟路径。
- Class 21 的属性 2–15 和方法 2–12 全部未暴露。

### 4.8 F200：属性 2/3 是把配置参数塞进了设备列表/数量

- 标准 A.13：F200 属性 2 是只读 `array 端口`（每项含端口描述符、COMDCB、端口功能）；Class 22 属性 3 是设备对象数量；属性 4 才是配置参数；方法 127 是配置端口。
- 代码：属性 2 = 波特率 `uint32`，属性 3 = 校验位 `uint8`，并且都可写。
- 写入只改文件作用域 `g_f200_channel_cfg`（`cmd/dcu/dcu_698_object_model.c:349-384`），不落盘，也不调用任何串口重配置。主站 SET 成功，波特率实际不变。
- 没有暴露方法 127，也没有属性 4。

### 4.9 6002 / 6003：搜表（D2-09 已按标准重做）

- 标准 A.7：6002 IC=11（集合接口类），属性 2 = 所有搜表结果数组、5 = 跨台区结果、6/7 = 记录数、8 = 配置、9 = 周期参数、10 = 状态（空闲/搜表中）；方法 127/128/129。
- 代码：6002 `class_id` 改为 `IC_COLLECTION`（11），广告 A2/A3/A6/A10 与 127/128/129。A2 编成 `array(0)`；A3/A6 都是 RAM 计数（无扫描驱动时恒 0）；A10 是空闲/搜表中。方法 127 的时长按分钟，重复启动回 DAR 3；到期由 `dcu_node_pump()` 的 `dcu_discovery_tick()` 回到空闲。
- 结果集只在 RAM，没有搜表结果存储表。产品没有广播/地址段扫描驱动，因此不会发现新表；不伪造 6003 的通信地址/信号品质等硬件字段。计数非 0 时 A2 回 DAR 2。
- 6003 是“一个搜表结果”结构（IC=8），只作为 6002 A2（以及 3303 事件）的元素，不再是独立对象。对象表与 Class 20 ACL 删除 6003 行；目录仍保留它的类型定义。
- 生产装配：`dcu_node_create_in()` 持有 `dcu_discovery_t` 并传给 `dcu_router_init_ex()`，不再传 NULL。

### 4.10 3000 / 3005：Class 24 的属性号被写成了 Class 7 的

这是最集中的标准错位。标准 Class 24 与代码的对照：

| 属性号 | 标准 7.3.24 | 代码实际返回 |
| ---: | --- | --- |
| 2 | 关联对象属性表（array OAD） | 事件记录结构 |
| 3 | 当前记录数（四张表的结构） | 抓拍 OAD 列表 |
| 4 | 最大记录数 | 当前记录数 |
| 5 | 配置参数（structure） | 最大记录数 |
| 6 | 事件记录表 1 | 判定门限 |
| 7 | 事件记录表 2 | 未暴露 |
| 8 | 事件记录表 3 | 上报标识 |
| 9 | 事件记录表 4 | 有效标识 |
| 10 | 当前值记录表 | 最近发生/结束时间 |
| 11 | 上报标识（enum） | 未暴露 |
| 12 | 有效标识（bool） | 未暴露 |
| 14 | 时间状态记录表 | 未暴露 |

也就是说，代码把 Class 7 的编号（2/3/4/5/6/8/9/10）直接搬到了 Class 24。事件数据本身是真实的（`dcu_event_monitor.c` 的流式判定 + `records[]` 环），但按标准读 3000 A2 会得到 NOT_FOUND/TYPE_MISMATCH（无标量表示），按 A6 会得到门限而不是记录表。写入 A6/A8/A9 只改 RAM，不持久化，而且改的是错误属性。

### 4.11 北向 A-XDR 的数组/结构通道（D2-09 更新）

`dcu_north_resolve_axdr()` 已按 OI 分派数组/结构编码：3000/3005 事件表、6000/6001 档案、6002 搜表、6012/6013 任务以及 Class 8 参数各有编码器。6002 A2 返回 `array 一个搜表结果`（产品没有扫描驱动时为空数组），A3/A6 是 `long-unsigned`，A10 是 `enum`。6003 不再是独立对象，只作为 A2 的元素类型。
标量回退仍走 `dcu_698_value_for_oad()`；5004 A2 是冻结记录表，Normal GET 仍不暴露。

### 4.12 装配事实

- `dcu_node_create_in()` 把 `esam` 传 NULL，把节点持有的 `&node->discovery` 传给 `dcu_router_init_ex()`。
- `dcu_discovery_init()` 在生产路径中调用；`dcu_node_pump()` 每轮调用 `dcu_discovery_tick()`。
- 结论：F100 的运行期仍不可达；6002 已装配且可读可调用；6003 不再是对象行。

## 5. 产品 OAD 常量解析表

| 常量 | OAD | 对象表行 | 是否经对象表解析 | 说明 |
| --- | --- | --- | --- | --- |
| `OAD_ACTIVE_ENERGY` | 0x00100200 | 0010 A2 | 是 | `dcu_698_var_read`；但量纲错、device 0 |
| `OAD_REACTIVE_ENERGY` | 0x00200200 | 0020 A2 | 是 | 同上 |
| `OAD_VOLTAGE` | 0x20000200 | 2000 A2 | 是 | 同上 |
| `OAD_VOLTAGE_B` | 0x20000300 | 2000 A3 | **否** | mask 无 A3；仅驱动点表/北向 metric 名 |
| `OAD_VOLTAGE_C` | 0x20000400 | 2000 A4 | **否** | mask 无 A4 |
| `OAD_CURRENT` | 0x20010200 | 2001 A2 | 是 | 量纲错、device 0 |
| `OAD_CURRENT_B` | 0x20010300 | 2001 A3 | **否** | mask 无 A3；且标准 2001 A4 是零线电流，不是 C 相 |
| `OAD_CURRENT_C` | 0x20010400 | 2001 A4 | **否** | mask 无 A4；且标准 A4 是零线电流 |
| `OAD_ZERO_SEQ_CURRENT` | 0x20020200 | 无（OI 2002 不在表） | **否** | 仅驱动点表；北向 GET NOT_FOUND |
| `OAD_ACTIVE_POWER_TOTAL` | 0x20040200 | 2004 A2 | 是 | 量纲错、device 0 |
| `OAD_ACTIVE_POWER_A/B/C` | 0x20040300/0400/0500 | 2004 A3/A4/A5 | **否** | mask 无；仅驱动/北向 metric |
| `OAD_REACTIVE_POWER_TOTAL` | 0x20050200 | 无（OI 2005 不在表） | **否** | 仅驱动/北向 metric |
| `OAD_POWER_FACTOR_TOTAL` | 0x200A0200 | 200A A2 | 是 | 标签对、raw 截断为 0 |
| `OAD_GRID_FREQUENCY` | 0x200F0200 | 200F A2 | 是 | 标签对、未 ×100 |
| `OAD_DAILY_FREEZE` | 0x50040200 | 5004 A2 | 是 | 返回冻结条数，不是冻结表 |
| `dcu_soe_event_engine.h` 的 `OAD_EVT_*` | 0x3000 等 | 无 | **否** | 实际存的是 OI，不是 OAD；SOE 引擎硬编码，且 0x3000 被注释成“过压”，标准是失压 |

对象表内部还会用无名字面量构造 OAD，例如 `0x60000200`、`0x60120200`、`0x60020200/0300/0600/0A00`、`0xF2000200/0300`、`0x30000200`、`0x30050200` 等；这些没有对应的 `OAD_*` 宏，测试里手写。北向 resolve 对 6002 走 `dcu_698_acq_6002_encode()`，其余标量走 `dcu_698_value_for_oad()`。

## 6. 标准与实现的错位汇总

| 项 | 标准 | 代码 | 影响 |
| --- | --- | --- | --- |
| 6000 类 | A.7 IC 列写 11；7.3.10 是 Class 10 | 10 | 项目 catalog 也取 10；视为原文 OCR 列错位，非代码缺陷 |
| 6002/6003 类 | A.7 IC 列写 11/8 | 6002=11，6003 无对象行 | D2-09 按标准修正；6003 本身是结构类型，只作为 6002 A2 的元素 |
| 8000 类 | A.9 IC 列写 8；7.3.13 是 Class 13 | 13 | 项目 catalog 取 13；视为原文列错位 |
| 6000 方法 | 127–134 | 1–5、127、128 | 1–5 是产品自定义；128 语义错；129–134 缺失 |
| 6012 方法 | 127–130 | 1、127–130 | 已按 A.7 实现；方法 1 是运行态复位 |
| 5004 方法 | 1、2、3、4、5、7、8 | 3、4、5、7、8（全 `-4`） | 冻结不可触发、不可配置 |
| 8000 方法 | 127–131 | 1、127–131 | 1 执行真实控制；127–131 全 TEMPORARY |
| 8000 属性 | 2、4、5 | 无 | 配置/告警/命令状态不可读 |
| F100 方法 | 1–12 | 1 | 方法 1 语义错（复位→鉴权） |
| F200 属性 | 2（只读端口列表）、3（数量）、4（配置） | 2（波特率）、3（校验位） | 读写语义全错 |
| 3000/3005 属性 | 2/3/4/5/6–9/10/11/12/14 | 2/3/4/5/6/8/9/10（Class 7 编号） | 所有已声明属性语义错 |
| 变量类 | 整数 + Scaler_Unit | float32 / 截断工程值 | 数值与类型都不符合 A.1/A.3 |

## 7. 按优先级修复队列

### P0 — 已声明但说谎（返回假值/假成功）

1. **变量类量纲与类型**（0010 A2/A4、0020 A2/A4、2000 A2、2001 A2、2004 A2、200A A2、200F A2）。按 A.1/A.3 恢复整数标签与 scale；`raw` 必须由工程值反算（`value / 10^scale`，带四舍五入），不能在 `dcu_698_var.c` 里截断。200A 当前把 0.95 读成 0，是最直接的假值。
2. **每表寻址**（所有 `DCU_OBJ_PER_METER` 行）。`dcu_698_var_read_value()` 固定 `device_id=0`，`dcu_698_var_read()` 忽略 `elem_idx`。必须把 OAD 元素索引/表实例映射成 `slot_id`，否则多表现场所有表返回 0 号表数据。
3. **3000/3005 属性号整体错位**（A2–A6、A8–A10）。按 7.3.24 重排：A2 关联对象表、A3 四表记录数结构、A4 最大记录数、A5 配置参数、A6–A9 四张记录表、A10 当前值、A11 上报标识、A12 有效标识、A14 时间状态。
4. **5004 属性 2 用条数冒充冻结表**。实现冻结记录数组/A-XDR 或明确返回 DAR 5；不能把 `count_freezes` 的 `uint16_t` 当属性值。
5. **6000 方法假成功**。Add/Delete/Clear 只改 RAM，无 `meter_inventory` 持久化；6000 方法 128 是 Delete 但标准是 AddBatch。需要落库、按 A.7 方法号重排，或明确拒绝。（6012 已在 D2-03 落库。）
6. **F200 属性 2/3 假写**。写只改静态全局，不落盘、不重配串口；标准属性 2 是只读端口列表、属性 3 是数量、属性 4 是配置。按 A.13/Class 22 重做。
7. **8000 方法 1 挂错语义**。真实拉/合闸在 Class 13 方法 1（复位）下执行；标准 129/130/131 是跳闸/合闸/明文合闸，却返回 TEMPORARY。把控制下沉接到 129/130/131；方法 1 恢复为复位。
8. **F100 方法 1 假鉴权**。XOR 模拟把 `is_authenticated` 置 true 并返回 0；Class 21 方法 1 是复位。安全路径要么接真实 ESAM，要么明确 DAR 5，不能返回成功。
9. ~~**6002 搜表假启动**~~（D2-09 已修）：方法 127 走真实状态机，`dcu_node_pump()` tick 到期回空闲；A2/A3/A6/A10 按表 A.7 暴露；6003 不再作为独立对象。
10. **北向结构属性部分可达（D2-09 更新）**。`dcu_north_resolve_axdr()` 已支持 3000/6000/6002 的数组/结构编码；5004 的冻结记录走 GetRecord，Normal GET 仍回 DAR 5。6003 作为 6002 A2 元素，产品没有扫描驱动时结果集为空。

### P1 — 已声明但占位（-2/-4/TEMPORARY/NULL）

1. 6000 方法 2、3：`-4`。
2. 5004 方法 3、4、5、7、8：`-4`（触发冻结、增删冻结对象、批量添加、清空关联表全不可用）。
3. 8000 方法 127、128：TEMPORARY（产品无告警输出）。
4. 8000 方法 129、130、131：参数解码真实，但无继电器 OAD→电表地址映射，全 TEMPORARY。
5. F100：生产装配传 NULL，所有调用返回 `-1`；需要 `dcu_security_init()` 并挂实例，或从对象表移除。（6002 已在 D2-09 装配。）

### P2 — 标准定义但未暴露

1. 6000：方法 129–134；属性 2 的数组/结构表示。
2. 6012：属性 3。
3. 5004：方法 1、2；属性 3。
4. 8000：属性 2、4、5。
5. F100：属性 2–15；方法 2–12。
6. F200：属性 4；方法 127。
7. 6002：属性 5、7、8、9（跨台区结果/配置/周期参数）未暴露；产品没有对应的跨台区与周期搜表能力。
8. 6003：保留为 6002 A2 的元素/结构类型，不再作为独立对象。
9. 变量类：属性 1 逻辑名、属性 3 换算及单位、Class 1 属性 5；Class 3/4 属性 2 的分相/总分相数组表示。
10. 3000/3005：属性 1、7、11、12、14；方法 2、4、5。
11. 标准 OI 2002（零序电流）、2005（无功功率）、2003、200B–200E、2010+ 等未进对象表，尽管驱动点表/北向 metric 已使用其中一部分。

### P3 — 一致性与可维护性

1. ~~`_oi_6002_execute()` 无论 `dcu_discovery_start()` 返回什么都返回 0；应透传。~~（D2-09 已透传，重复启动回 DAR 3。）
2. `_oi_6000_execute()` 方法 4 删除失败时 result 写 `-2`，函数仍返回 0；应统一。
3. `dcu_698_read/write/call` 对 mask 未命中返回 `-1`，驱动按 NOT_FOUND（DAR 4）映射；设计 4.1 要求属性/方法不支持返回 DAR 5。应在门面层区分“OI 不存在”和“位未定义”。
4. 测试 `test_dcu_698_var` / `test_dcu_698_value_table` / `test_dcu_698_axdr` 断言的是当前非标准标签和截断值，等于给错误背书；修 P0 时必须同步改测试向量。
5. `g_objects[]` 不是 `const`，设计 4.1 写的是 `const` 表；运行期 `_set_instance()` 在改它。
6. 设计文档 4.2 的处理器签名是 `dcu_axdr_buf_t`，实现是 `void *out_val` + 旧式标量；设计 8 列出的 `dcu_698_event.c`、`dcu_698_param.c` 等文件不存在，实际都在 `dcu_698_object_model.c`。
7. 设计/README 中的 `dcu_esam.*` 不存在，实际是 `dcu_security.*`。
8. `dcu_soe_event_engine.h` 的 `OAD_EVT_*` 名字是 OAD 但值是 OI，且 0x3000 注释为“过压”（标准 3000 是失压）；与对象表两套事件模型并存。

## 8. 测试覆盖缺口

已有测试覆盖“类型标签/编码/DAR 路径/方法分发”，但没有一条以标准附录 A 为 oracle：

| 缺口 | 现状 |
| --- | --- |
| 标准类型/量纲 | `test_dcu_698_var`、`test_dcu_698_value_table`、`test_dcu_698_axdr` 断言 `float32` 和截断值，未对照 A.1/A.3 |
| 多表寻址 | 没有“两个 slot 返回各自影子”的用例；所有 mock 都忽略 `device_id` |
| 3000/3005 属性号 | 只测了 4/5/6/8/9 的标量值，没有按 7.3.24 断言属性含义 |
| 5004 冻结表 | 只测本地 `dcu_storage_query_freeze()` 和计数，没有北向 GET Record/RSD |
| 6000/6012 方法号 | 只测 127/128 和 1，没有标准 129–134 / 127–130 的用例 |
| 8000 标准方法 | 129–131 只断言 TEMPORARY；没有“真实控制下沉接到 129”的正向用例 |
| F100/F200 标准语义 | F100 只测 XOR 模拟；F200 只测波特率读写，没有端口列表/数量/配置参数 |
| 持久化 | 没有“Add 后重启仍存在”“SET 后串口实际重配”的用例 |
| catalog 语义 | `test_dcu_698_catalog` 只检查 class_id 和位号上界，不检查该 OI 的属性/方法列表是否与附录 A 一致，所以上述错位全部漏过 |

## 9. 计数复核

| 状态 | 数量 |
| --- | ---: |
| real | 0 |
| partial | 37 |
| stub | 12 |
| unimplemented | 3 |
| masked-but-undefined | 4 |
| **合计** | **56** |

按 OI 汇总的已声明位数（D2-09 前的审计快照）：6000 8、6012 2、0010 2、0020 2、2000 1、2001 1、2004 1、200A 1、200F 1、5004 6、8000 6、F100 1、F200 2、6002 3、6003 1、3000 9、3005 9，合计 56。

D2-09 后 6002 声明 A2/A3/A6/A10 四个属性与 127/128/129 三个方法；6003 不再有对象行。上表其余行的状态与计数未重算。
