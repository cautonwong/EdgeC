# DCU-D0-06 自审

## 逐条核对

| 结论 | 依据 | 核对结果 |
| --- | --- | --- |
| 698 从站监听 `DCU_NORTH_SOCK`，缺省 `/tmp/dcu_north_slave.sock` | `cmd/dcu/dcu_north_adapter.c:112-116` | 一致 |
| 云北向在 `dcu_north_cloud_attach` | `cmd/dcu/dcu_north_adapter.c:157` | 一致 |
| 两条通道在 main 装配 | `cmd/dcu/main.c:179-180` | 一致 |
| HCS 与 FCS 都校验 | `foundation/src/protocol/dlt698_codec.c:230-236` | 一致（D0-05a 已合并） |
| GET-Normal 已实现 | `adapters/io_drivers/src/driver_dlt698.c:295` | 一致 |
| GET-NormalList 已实现，逐项 DAR | `driver_dlt698.c:261`，DAR 6/7 在 `:280`、`:285` | 一致 |
| GET-Normal 读不到对象不应答 | `driver_dlt698.c:303` | 一致 |
| ACTION 驱动分支存在 | `driver_dlt698.c:315` | 一致 |
| ACTION 产品钩子回失败，驱动回 DAR=2 | `dcu_north_adapter.c:74`、`driver_dlt698.c:353` | 一致 |
| SET 无分支 | `driver_dlt698.c` 全文无 `SVC_SET_REQ` | 一致（D1-14） |
| 对象表 17 个 OI | `cmd/dcu/dcu_698_object_model.c` 表内 17 条 | 一致 |
| 类型化读取首批 OI | `cmd/dcu/dcu_698_var.h:38` | 一致 |
| ADR 存在 | `docs/adr/0013-698-slave-axdr-port.md` | 一致 |
| IEMS 两处从站接线 | `cmd/iems/iems_app.c:276`、`cmd/iems/iems_north_iec104.c:110` | 与旧文的「唯一调用点 iems_app.c:535」不符，已改正 |
| DCU 从站接线 | `cmd/dcu/dcu_north_adapter.c:116` | 与旧文的「DCU 无北向从站」不符，已改正 |

## 未写的结论

- 没有写 DCU 对象表里每个 OI 的具体属性清单。那属于 `docs/design/DCU_698_OBJECT_MODEL.md`，本文件只链接。
- 没有写计划的阶段与排期。`docs/plans/` 由 verifier 维护。
- 没有断言 `manifest.yaml` 的 NORTHBOUND 条目数。装配已由 `dcu_north_adapter.c` 承担，该配置项不再是判断依据。

## 复核

- 两个文件都只做了文字改写与追加，无代码块语法改动。
- `git diff --check` 通过；目录校验脚本通过；守卫自检通过。
