# DCU-D1-18d 控制与安全类诚实化

基线与分支：`dcu/D1-18d-control-security`，工作树 `/tmp/dcu-d1-18d`，父提交 `13230256`（D1-18c 合并后）。

## 目的

控制（OI 8000 / Class 13）与安全（OI F100 / Class 21、F200 / Class 22）对象原先对外广告的位与实际行为不一致：方法 1 挂的是演示跳合闸、假鉴权或 RAM 全局。本单把标准命名空间里不成立的位摘掉，把演示路径移出台面，并按 DAR 规则如实分类。

## 位 → 状态

| 对象/位 | 类定义 | 对外广告 | 实际行为 |
| --- | --- | --- | --- |
| 8000 方法 1–7 | Class 13 定义 1–7 | 否 | DAR 5（类定义但未广告）；演示跳合闸移到私有入口 |
| 8000 方法 127、128 | 表 A.9 定义 | 是 | DAR 2：产品没有告警输出 |
| 8000 方法 129、130、131 | 表 A.9 定义 | 是 | 参数解码通过后 DAR 2：无继电器 OAD→电表地址映射 |
| 8000 方法 8 及以上（127–131 除外） | 未定义 | 否 | DAR 4 |
| F100 方法 1 | Class 21 定义 | 是 | 标准复位：清随机数、会话密钥、鉴权标志，返回成功 |
| F100 方法 2–12 | Class 21 定义 1–12 | 否 | DAR 5 |
| F100 方法 13 及以上 | 未定义 | 否 | DAR 4 |
| F200 A2、A3、A4 | Class 22 定义 1–4 | 否 | DAR 5；`g_f200_channel_cfg` 及其读写处理器删除 |
| F200 A5 及以上 | 未定义 | 否 | DAR 4 |
| F200 方法 1、2 | Class 22 定义 1–2 | 否 | DAR 5 |
| F200 方法 3 及以上 | 未定义 | 否 | DAR 4 |

DAR 分类只在 `dcu_698_call` 一处产生：未广告位先查类定义（`_class_defines_method`），定义了回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5），没定义回 `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4）。对象不在对象表里回 `EDGE_SLAVE_SRC_NOT_FOUND`（DAR 6），与属性读写路径一致。

## 移出标准命名空间的东西

- **8000 方法 1 的跳闸/合闸**：改为 `dcu_698_control_legacy_action(uint32_t slot, uint8_t action)`，声明在 `dcu_698_object_internal.h`，只给演示心跳（`dcu_heartbeat.c`）调用；不再占用 OI 8000 的方法位。
- **F100 方法 1 的假鉴权**：`dcu_698_security_f100_execute` 不再调用 `dcu_security_authenticate`，只做标准复位。鉴权仍是安全模块能力（`dcu_security_authenticate`），不经 OI 暴露。
- **F200 A2/A3 的波特率/校验位**：删除 `g_f200_channel_cfg` 与 `dcu_698_security_f200_read/write`。写入从未重配串口，只是改一个没被读的 RAM 全局。

## 广告表同步

`dcu_698_appconn.c` 的 `k_methods_8000` 由 `{1,126,127..131}` 收敛为 `{127,128,129,130,131}`，对象列表项的方法数 7 → 5，与对象表 mask 一致。

## 阻塞与限制

- 8000 方法 129–131 保持 DAR 2，直到有继电器 OAD→电表地址映射；本单不实现映射。
- F200 A4 没有实现：产品侧没有串口参数重配的端口与适配器，写配置无处落地。要对外必须先有真实的串口重配能力。

## 范围外观察（未改）

1. 线上 ACTION 路径先解码参数再进对象表：`dcu_698_axdr.c:155` 对 127–131 以外的 8000 方法直接回 `EDGE_SLAVE_SRC_NOT_FOUND`（DAR 6），所以经北向调用 8000 方法 1–7 拿到的是 DAR 6，不是对象表内的 DAR 5。分类规则因此有两处来源。
2. `adapters/io_drivers/src/driver_dlt698.c` 的 `_dar_for_action_rc` 把 `rc == -1` 映射为 DAR 2，处理器的裸 `-1` 会伪装成“暂时失效”。
3. ESAM/F200 的方法不走北向 ACTION（`dcu_north_accept_action` 只认 OI 8000），所以这些位的 DAR 目前只有对象表层语义，没有线上证据。

## 门禁

- 干净构建 + `check_build_warnings.py`：0 warning
- 全量 `ctest`：336/336
- ASAN + UBSAN（dcu/dlt698 33 项）：33/33（并行首次 `test_dcu_runtime_health` 负载超时，串行全量通过）
- `run_guard_selftest.py`：全部通过
- 验收 `test.sh --omnis dcu`：dcu-acceptance 3/3、dcu-northbound 10/10
- `-fanalyzer`（cmd/dcu）：0 新报告
- `git diff --check`：干净

## 测试覆盖

- `tests/test_dcu_698_object_model.c::test_dar_classification_by_class_definition`：8000 的 1、7（DAR 5）与 8（DAR 4）；F100 的 2（DAR 5）与 13（DAR 4）；F200 方法 1（DAR 5）与 3（DAR 4）；广告位 127/128（DAR 2）与 129 空参数（DAR 7）。
- `tests/test_dcu_698_axdr.c`：127–131 线上 DAR（127/128/129/130/131 全为 DAR 2，参数错误为 DAR 7）；F200 A2 写入回 DAR 5。
- `tests/test_dcu_698_value_table.c`：F200 A2/A3/A4 标量取值回 DAR 5，A5 回 DAR 4。
- `tests/test_dcu_advanced_modules.c`：F100 方法 1 复位清鉴权状态与会话密钥；F200 读写均失败。
- `tests/test_dcu_router.c`：8000 方法 1 回 DAR 5。
