# DCU-F5S：事件 A6 快照按稳定 slot_id 取表

分支 `dcu/F5-snapshot-identity`，工作树 `/tmp/edgec-land/f5-snapshot-identity`，基线
`feat/dcu` `b1907349`（D5-04 已合入）。

## 背景

F5R 已把事件监控/日志/冻结/record reading 的设备身份改为 slot_id，但
`dcu_event_monitor.c::_capture_record_snapshots` 仍调用只带 OAD 的
`dcu_698_var_encode_axdr`，后者把 OAD 低字节当 `device_id`。事件 A2 的关联 OAD（如
`OAD_VOLTAGE = 0x20000200`）低字节为 0，于是每条事件记录的关联数据都从 slot 0 取表；
档案删除重排后，D 的事件快照可能来自顶替下标的另一只表。F5R 的移位用例当时把
`snapshot_count` 置 0 隔离该缺陷。

## 数据流（修复前）

1. `dcu_poller` 读完成 → `publish_realtime(tag, slot_id, value)` → DataHub 影子按
   `(tag, slot_id)` 存（`dcu_node.c` 的 `_node_publish_realtime_device`）。
2. 事件判定 `stage_event_monitor` 把 `event_msg.device_id = slot`。
3. `stage_event_logger` 用 `dcu_inventory_by_slot(ev->device_id)` 取地址（已是 slot），
   并调用 `dcu_obj_snapshot(evt->oi, ev->device_id, ...)`（已按 slot 取表，仅用于计数）。
4. A6 记录构建 `_capture_record_snapshots(evt, rec)` **没有 device 参数**，内部
   `dcu_698_var_encode_axdr(oad)` 取 `slot = oad & 0xFF = 0` → `read_shadow(tag, 0)`
   → 永远读 slot 0（A 表），不是事件发生的 D 表。

## 修复

- `cmd/dcu/dcu_698_var.c`：把原编码体抽成 `_var_encode_axdr(oad, device_id, ...)`；
  `dcu_698_var_encode_axdr(oad, ...)` 保持旧语义（`device_id = oad & 0xFF`，北向 GET
  路径不变）；新增 `dcu_698_var_encode_axdr_device(oad, device_id, ...)` 供快照使用，
  声明在 `dcu_698_var.h`（device_id 是稳定 slot 身份，OAD 低字节仍只作属性/元素选择）。
- `cmd/dcu/dcu_event_monitor.c`：`_capture_record_snapshots(evt, device_id, rec)`，
  发生与恢复两条路径都传 `ev->device_id`（稳定 slot_id），调用新的 device 版编码器。

A6 的 blob 仍是快照 OAD 的标准 A-XDR（数组/标量+量纲），只把“从哪只表取”从 OAD
元素序号改为事件 slot_id。OAD 低字节继续只作属性/元素选择。

## 拒绝的替代

1. **把 slot 写进快照 OAD 低字节再调旧编码器**：违反不变量（OAD 元素序号只能作属性/
   元素选择），且把设备身份混进属性键。
2. **在 `dcu_event_monitor.c` 里用 `dcu_698_snapshot` 的 pipe_msg 结果自行编码**：
   数组形状/tag/scaler 是 `dcu_698_var.c` 的静态单一来源；在事件模块复制会形成第二份
   量纲与帧规则（仓库明令禁止同一协议族规则多处来源）。
3. **改 `dcu_698_var_encode_axdr` 的签名加 device 参数**：调用方在 `dcu_698_axdr.c`
   （D4-10 文件锁内），且会改变 resolver 契约。
4. **在两处 `.c` 各留本地原型**：重复契约会在签名漂移时静默出事；返工已删除，
   声明只保留在 `dcu_698_var.h`（该头从未被 D4-10 占用，"文件锁"是初版误判）。

## 红先绿后

- 测试提交 `d99574b6`：移位用例恢复 `snapshot_count=1`、`snapshot_oads[0]=OAD_VOLTAGE`；
  为 slot 0（A）与 slot 3（D）发布不同电压影子，断言 D 的发生记录快照为
  `1500/1510/1520`（raw）。旧码红：`1000 != 1500`（读到 A 表）。
- 修复提交 `85cb3a04`：两个文件；`test_dcu_advanced_modules` 2/2 绿，
  `ctest -R 'dcu|dlt698'` 45/45 绿。

## 验证命令与结果

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DPython3_EXECUTABLE=/home/edge/EdgeC/.venv/bin/python3
cmake --build build --clean-first -j4
ctest --test-dir build -R 'dcu|dlt698' --output-on-failure
python3 ci/scripts/check_build_warnings.py --log <clean-build.log> --root .
python3 ci/scripts/check_build_freshness.py --root . --build build
python3 ci/scripts/check_dcu_698_catalog.py
python3 ci/scripts/check_doc_references.py --root .
```

结果：clean build 0 第一方 warning（仅 third-party open62541 一条）；45/45 绿；
四个守卫 0。详见 `runtime.log`。

## 未决项

- `test_dcu_advanced_features_dispatch` 也设置了 `snapshot_count=1` 但未向 hub 发布
  电压影子，其快照保持 NULL（未断言）；生产环境影子存在。不属缺陷，未改该用例。
- `_capture_record_snapshots` 与 `dcu_obj_snapshot` 各读一次影子（一次为计数、一次为
  A-XDR 编码）；无正确性问题，未在本单合并。
- 计划/交接文字由 verifier 更新。

## 返工记录（verifier 2026-11-26）

- 返工提交：`dcu_698_var_encode_axdr_device` 的原型移入 `dcu_698_var.h`（带注释：
  device_id 是稳定 slot 身份），删除 `dcu_698_var.c` 与 `dcu_event_monitor.c` 的两份
  本地原型。
- 重跑：`test_dcu_advanced_modules` 2/2；`ctest -R 'dcu|dlt698'` 45/45；clean build
  0 第一方 warning；warnings/freshness/catalog/docrefs 0。原文见 `runtime.log` 补记。
