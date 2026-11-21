# DCU 配网智能融合终端

DCU 是台区侧配电终端，向北对配网主站讲 DL/T 698.45（服务端），向南轮询 RS485 与载波表计。代码在 `cmd/dcu/`。本文件是入口；设计细节在文末列出的权威文档里，不在这里复制。

## 业务背景：五位一体

终端的价值不在单点抄表，而在于把五件事连成一个环：计量数据用于结算，采集数据用于运行监测，处理数据用于本地判断，控制用于物理动作，通信用于与主站交互。任何一环断开，终端就退化成数据透传盒。下面的模块划分按这五个环节切开。

三条工程约束来自现场，不是实现偏好：

- **Flash 寿命**：现场多用 SPI NAND。每次抄表单条写 SQLite 会因垃圾回收很快磨穿。采用 WAL 加内存缓存，凑满一批或到时间再事务落盘（`dcu_storage.c:44` 的 WAL、`dcu_storage.h:12` 的 `STORAGE_CACHE_MAX`、`dcu_storage.c:76` 的 10 秒定时器）。
- **通道串行**：RS485 是半双工，同一条总线不能并发抄表，只能排队。连续失败达到阈值的表计熔断挂起，队列跳过它，把总线时间还给在线表（`dcu_poller.c:19` 的默认阈值 5，`:205-214` 的触发，`dcu_inventory.h:29` 的 `is_online` 语义）。
- **掉电保护**：现场停电频繁。收到 SIGTERM 或 SIGINT 时先落盘再退（`main.c:188-190` 注册信号，`:49-60` 执行落盘）。

## 模块图

按职责分组。每组给代表文件，不逐文件列。

| 分组 | 代表文件 | 职责 |
| --- | --- | --- |
| 组合根 | `main.c`、`dcu_node.c` | 装配、生命周期、聚合根 |
| 档案 | `dcu_inventory.c` | 表计清单、在线状态、熔断标记 |
| 轮询 | `dcu_poller.c`、`dcu_driver_adapter.c`、`dcu_bus_adapter.c` | 周期抄读、通道策略、驱动注入 |
| 存储 | `dcu_storage.c` | 缓存写、批量事务、冻结查询 |
| 698 对象模型 | `dcu_698_object_model.c`、`dcu_router.c` | 17 个 OI 的静态表；router 只做查表门面 |
| 变量读取 | `dcu_698_var.c` | Class 1/3/4/6 的类型化读取，首批 OI 见 `dcu_698_var.h:38` |
| 北向从站 | `dcu_north_adapter.c`、`dcu_north_source.c` | 698 从站 UDS 通道；MQTT 云北向取数 |
| 事件与业务 | `dcu_soe_event_engine.c`、`dcu_outage_detector.c`、`dcu_event_monitor.c`、`dcu_topology_lineloss.c`、`dcu_topology_phase_id.c`、`dcu_load_control.c`、`dcu_peripheral_telecontrol.c` | SOE、停电、线损、相位识别、功控、四遥 |
| 运维 | `dcu_heartbeat.c`、`dcu_app_daemon.c`、`dcu_discovery.c`、`dcu_security.c` | 心跳、守护、发现、安全 |

北向两条通道都在组合根接线（`cmd/dcu/main.c:179-180`）。698 从站在 `dcu_north_adapter.c:112` 装配，UDS 监听 `DCU_NORTH_SOCK`，缺省 `/tmp/dcu_north_slave.sock`。

## 698 覆盖现状

| 项 | 状态 | 依据 |
| --- | --- | --- |
| 帧校验（HCS 与 FCS） | 已实现 | `foundation/src/protocol/dlt698_codec.c:230-236` |
| GET-Request-Normal | 已实现，失败回 DAR | `adapters/io_drivers/src/driver_dlt698.c:346` |
| GET-Request-NormalList | 已实现，逐项回 DAR | 同文件 `:295` |
| ACTION | 驱动分支已实现；产品钩子回失败，按 DAR=2 应答 | `:384`、`cmd/dcu/dcu_north_adapter.c:75` |
| SET-Request | 未实现 | D1-14 |
| GET-Record 与 RSD | 未实现 | D1-15 |
| REPORT.notification | 未实现 | D1-16 |
| Class 20 与 4400 应用连接 | 未实现 | D1-17 |

一处已知行为差异，不要当成已完成：

- ACTION 的 OI 8000 方法 129/130 参数是 `array structure{...}`（标准表 A.9）。当前未解参，主站收到 DAR=2（`dcu_north_adapter.c:74`）。

GET-Request-Normal 与 NormalList 的失败项都按 6.3.3.11 回 DAR（对象不存在 6、类不匹配 5、类型不匹配 7、标准未定义 4、暂时失效 2），不再静默超时。

A-XDR 结构属性端口已实现（`docs/adr/0013-698-slave-axdr-port.md`，D1-13）。`edge_slave_plane.h` 提供 `resolve_axdr`/`accept_axdr`，调用方给缓冲，NULL 时回退标量路径；NormalList 用流式逐项编码，单缓冲复用。`accept_axdr` 的 SET 侧随 D1-14。

## 运行与验收

仓库根目录：

```bash
./build.sh                       # 配置与构建
./test.sh                        # C 单元与集成测试
./test.sh --omnis dcu            # 验收，由 test-platform 驱动
```

验收任务在 `acceptance/dcu/tasks/`：`dcu_acceptance.yaml`（南向采集）与 `dcu_northbound.yaml`（698 北向从站）。产品壳是 `acceptance/dcu/dcu_product.py`。

DCU 读取的环境变量：

| 变量 | 作用 | 缺省 |
| --- | --- | --- |
| `EDGE_DB_PATH` | SQLite 文件路径 | 由 `EDGEC_DATA_DIR` 推导（`main.c:87-88`） |
| `EDGEC_DATA_DIR` | 数据目录 | 同上 |
| `DCU_NORTH_SOCK` | 698 从站 UDS 路径 | `/tmp/dcu_north_slave.sock` |
| `MQTT_BROKER` | 云北向 broker | `tcp://127.0.0.1:1883` |
| `MQTT_GROUP_ID`、`MQTT_NODE_ID` | 北向主题身份 | `EdgeOS`、`DCU_Node_A` |
| `MQTT_PROTOCOL` | 北向协议选择 | 由北向管理器缺省决定 |
| `EDGE_DCU_TWIN_SOURCE` | 双活数据源开关 | 见 `main.c:108` |

## 权威文档

- 领域上下文与 698 覆盖：`cmd/dcu/CONTEXT.md`
- 对象模型与 A-XDR 设计：`docs/design/DCU_698_OBJECT_MODEL.md`
- 北向架构与缺口：`docs/design/NORTHBOUND_ARCHITECTURE.md` 4.4 节
- 迁移与排期：`docs/plans/2026-11-21-dcu-698-production-plan.md`
- 从站 A-XDR 端口 ADR：`docs/adr/0013-698-slave-axdr-port.md`
