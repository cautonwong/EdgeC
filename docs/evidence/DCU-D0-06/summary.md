# DCU-D0-06 同步北向与 DCU 上下文文档

## 做了什么

`docs/design/NORTHBOUND_ARCHITECTURE.md` 的 4.4 节与 `cmd/dcu/CONTEXT.md` 里还有与代码不一致的说法，本次按代码现状改写。只改文档，未改代码。

## NORTHBOUND_ARCHITECTURE.md

4.4 节原表格写「DCU 无北向从站」「698 服务端接总线：manifest 0 条、main.c 只 setup_master」「GET 半成品，固定回 2 字节」「OI 8000 返回 -4」「SET/ACTION/Record/REPORT 全无」。这些已经过时。

改写后写清：

- 北向两条通道：698 从站（`cmd/dcu/dcu_north_adapter.c:112`，UDS 监听 `DCU_NORTH_SOCK`，缺省 `/tmp/dcu_north_slave.sock`）与 MQTT 云北向（同文件 `:157`），两者都在 `cmd/dcu/main.c:179-180` 装配。
- 服务覆盖：HCS 与 FCS 已校验（`foundation/src/protocol/dlt698_codec.c:230-236`）；GET-Normal（`driver_dlt698.c:295`）与 GET-NormalList（`:261`，逐项 DAR=6/7）已实现；ACTION 驱动分支（`:315`）已实现，DCU 产品钩子（`dcu_north_adapter.c:74`）对全部方法回失败，按 DAR=2 应答；SET、GET-Record/RSD、REPORT、Class 20 与 4400 未实现，分别指向 D1-14 到 D1-17。
- 对象与类型：对象表 `dcu_698_object_model.{h,c}`（17 个 OI），`dcu_router.c` 只剩查表门面；类型化读取 `dcu_698_var.{h,c}`（首批 OI 见头文件）；A-XDR 端口 ADR `docs/adr/0013-698-slave-axdr-port.md`。
- 已知限制：OI 8000 方法 129/130 的参数结构未解，主站收到 DAR=2；GET-Normal 读不到对象时静默不应答（`driver_dlt698.c:303`），与 NormalList 逐项回 DAR 的行为不一致。

同时修正两处连带过时的表述：

- 4.3 的 B-P1-3 原来写「DCU 无任何北向从站」，改为已接线并给出行号。
- 4.3 的 B-P1-1 原来写「只有 IEMS 1 个接了线，`iems_app.c:535`」，改为 IEMS 两处（`iems_app.c:276`、`iems_north_iec104.c:110`）与 DCU 一处（`dcu_north_adapter.c:116`），并标明其余产品未接。
- 4.1 的核对命令块里，「DCU 698 服务端有没有接（预期：无输出）」改成核查 `dcu_north_adapter.c` 与 `main.c` 的两条装配调用。

## cmd/dcu/CONTEXT.md

保留原有领域模型，追加第 4 节：

- 4.1 模块分组：组合根、采集、存储、698 北向、业务、运维六组的文件与职责。
- 4.2 698 北向覆盖：一张状态表，逐项给依据文件与行号，未实现的标 D1-14 到 D1-17。
- 4.3 设计文档链接：北向架构、对象模型、迁移计划。不复述计划内容与排期。

## 验证

| 命令 | 结果 |
| --- | --- |
| `git diff --check` | 通过，无空白错误 |
| `python3 -I scripts/extract_dcu_698_catalog.py --check` | 通过 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | 全部守卫与正反夹具通过 |

输出见 `tests.log`。

## 改动范围

`docs/design/NORTHBOUND_ARCHITECTURE.md`、`cmd/dcu/CONTEXT.md`、`docs/evidence/DCU-D0-06/`。未改代码，未改 `docs/plans/`。
