# DCU-D0-01 自审

## 逐条核对

| README 结论 | 依据 | 核对结果 |
| --- | --- | --- |
| WAL 模式 | `cmd/dcu/dcu_storage.c:44` | 一致 |
| 缓存上限 128 | `cmd/dcu/dcu_storage.h:12` | 一致（旧文写 50，已改） |
| 10 秒落盘定时器 | `cmd/dcu/dcu_storage.c:76` | 一致 |
| 熔断阈值默认 5 | `cmd/dcu/dcu_poller.c:19` | 一致（旧文写 3，已改） |
| 熔断触发条件 | `cmd/dcu/dcu_poller.c:205-214` | 一致 |
| 熔断标记语义 | `cmd/dcu/dcu_inventory.h:29` | 一致 |
| SIGTERM/SIGINT 落盘 | `cmd/dcu/main.c:188-190`、`:49-60` | 一致 |
| 北向两条通道接线 | `cmd/dcu/main.c:179-180` | 一致 |
| 698 从站装配与 UDS 路径 | `cmd/dcu/dcu_north_adapter.c:112`、`:113-114` | 一致 |
| 对象表 17 个 OI | `cmd/dcu/dcu_698_object_model.c` | 一致 |
| 变量读取首批 OI | `cmd/dcu/dcu_698_var.h:38` | 一致 |
| HCS/FCS 校验 | `foundation/src/protocol/dlt698_codec.c:230-236` | 一致 |
| GET-Normal / NormalList / ACTION | `driver_dlt698.c:295` / `:261` / `:315` | 一致 |
| ACTION 回 DAR=2 | `dcu_north_adapter.c:74`、`driver_dlt698.c:353` | 一致 |
| GET-Normal 静默不应答 | `driver_dlt698.c:303` | 一致 |
| A-XDR 端口只有 ADR | `docs/adr/0013-698-slave-axdr-port.md`，`edge_slave_plane.h` 无 `_axdr` 成员 | 一致 |
| 验收任务两个 | `acceptance/dcu/tasks/*.yaml` | 一致 |
| 环境变量清单 | `cmd/dcu/main.c:87-88`、`:108`，`dcu_north_adapter.c` 的 `MQTT_*` 与 `DCU_NORTH_SOCK` | 一致 |

## 刻意没写

- 没写文件总数。目录数会随任务变动，写进文档就等着过期。
- 没写对象表每个 OI 的属性清单与数据库表结构，那是 `DCU_698_OBJECT_MODEL.md` 的范围。
- 没写任务号与排期，那是 `docs/plans/` 的范围；README 只保留 D1-14 到 D1-17 作为缺口索引。
- 没保留「Flash 磨损降低 95%」这类数字，仓库内没有可核对来源。

## 复核

- 只改了 `cmd/dcu/README.md` 与证据目录。
- 三条验证命令全部通过，输出在 `tests.log`。
