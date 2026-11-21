# 工业与细分领域通信协议覆盖与适配规划

> 日期：2026-11-21　状态：草案，待评审
> 读者：架构、驱动开发、产品
> 关联：`docs/plans/2026-11-21-industry-coverage.md`、`docs/plans/2026-11-21-terminal-common-components-plan.md`

## How to read this

一个任务行就是一个工作单元。每行都写了负责角色、估时、依赖、交付物和出口证据。出口证据必须是能复现的东西：抓包、测试输出、验收报告或提交 SHA。测试通过只是必要条件，完成定义是仓库口径：静态分析、测试、运行时验证证据三者齐全。

## 1. 结论

仓库已支持的协议覆盖电力、通用工业、医疗与航天的一部分，数量约十五种。缺的协议里，有的能按现有驱动框架补，有的做不了，还有的属于认证问题。

协议分三类，不能都按「写个驱动」估算：

1. 帧与应用层协议，可做成适配器。OPC UA 客户端、DNP3、BACnet/IP、IEC 61850 MMS、国网主站协议属于这一类。工作量在编解码、模型与安全栈。
2. 实时现场总线，用户态做不了。EtherCAT、Profinet RT 与 IRT、EtherNet/IP 的 CIP Motion 要实时内核、专用网卡或厂商协议栈。只能做网关硬件或旁路。
3. 安全协议，属于认证问题。PROFIsafe、CIP Safety、GOOSE 的时序保证，不在代码层解决。

## 2. 已实现的协议

| 层 | 协议 | 代码落点 |
| --- | --- | --- |
| 电力 SCADA | IEC 60870-5-101 链路层、IEC 104 主与从 | `foundation/src/protocol/iec101_link.c`、`iec104_apci.c`、`iec_asdu_codec.c`、`adapters/io_drivers/src/driver_iec101.c`、`driver_iec104.c` |
| 电力计量 | DL/T 645、DL/T 698 应用层、DLMS 与 COSEM（HDLC、GBT、安全平面） | `foundation/src/protocol/dlt645_codec.c`、`dlt698_codec.c`、`dlms_*` |
| 行业规约 | YD/T 1363.3 | `foundation/src/protocol/ydt1363_codec.c` |
| 通用工业 | Modbus RTU 与 TCP，含 HiCRC 变体 | `foundation/src/protocol/modbus_codec.c`、`adapters/io_drivers/src/modbus_*` |
| 总线与运动 | CAN、CiA402 | `foundation/src/protocol/can_codec.c`、`driver_can.c`、`driver_cia402.c` |
| 北向 | MQTT JSON、Sparkplug B | `adapters/io_drivers/src/driver_mqtt_json.c`、`driver_sparkplug_b.c`、`adapters/mqtt_transport/` |
| 数据面 | DDS 后端 | `modules/edge_core/include/datahub_dds.h` |
| 专用 | CCSDS、HL7 FHIR、雷达 FMCW、ADC | `driver_ccsds.c`、`driver_hl7_fhir.c`、`driver_radar_fmcw.c`、`adc_driver.c` |

## 3. 提到但未实现

| 协议 | 现状证据 | 判定 |
| --- | --- | --- |
| OPC UA | `foundation/include/edge_uns_engine.h` 有节点映射字段；`notes.md:80` 记录北向只有 MQTT 与 REST | 未实现 |
| IEC 61850 | 只有 `PROTOCOL_ROLE_PUB_SUB` 角色枚举；无 MMS 与 GOOSE 实现 | 未实现 |
| BACnet、Profinet、EtherNet/IP | 只出现在差距分析文档 | 未实现 |
| DNP3、HART、EtherCAT、J1939、NMEA、ASTERIX、IEEE 11073、CoAP 与 LwM2M | 全仓无实现 | 未实现 |

## 4. 优先级

按产品线排，P0 是当前有交付压力的，P2 是生态补全。

| 优先级 | 协议 | 面向行业 | 工作量级 | 备注 |
| --- | --- | --- | --- | --- |
| P0 | OPC UA 客户端 | 制造、楼宇、电力 | 大 | 证书与安全栈占一半以上 |
| P0 | IEC 61850 MMS 客户端 | 变电、站所 DTU | 大 | ASN.1 模型与数据集映射 |
| P0 | 国网主站与终端规约 | 配电 | 中 | 与项目招标规范逐条核对 |
| P1 | DNP3 | 电力出口、水务 | 中 | 含 SAv5 安全 |
| P1 | BACnet/IP | 楼宇、SmartBox | 中 | 对象模型与 COV 订阅 |
| P1 | MQTT 5 与 Sparkplug B 增强 | 物联网、云边协同 | 小 | 现有传输层扩展 |
| P1 | CoAP 与 LwM2M | 低功耗物联网 | 中 | 依赖 DTLS |
| P2 | IEEE 11073 | 医疗 | 中 | 与 HL7 FHIR 配套 |
| P2 | ARINC 429、MIL-STD-1553 | 航空 | 硬件为主 | 代码只做桥接 |
| 不做 | EtherCAT、Profinet RT 与 IRT、EtherNet/IP CIP Motion | 制造 | 不适用 | 需实时内核与专用网卡 |
| 不做 | PROFIsafe、CIP Safety | 功能安全 | 认证 | 不在软件层承诺 |

## 5. 组件与约束

- 编解码放 `foundation/src/protocol/`，纯函数、调用方分配。
- 协议适配器放 `adapters/io_drivers/`，遵循主从双角色与 `edge_poll_policy_t`。
- 驱动会话私有区上限 1024 字节（`driver_iec104.c:75` 有断言）。OPC UA 与 61850 的会话状态远超此值，必须做成独立传输组件，走 `edge_io` 的连接抽象，而不是塞进现有驱动槽。
- 安全栈单独组件：OPC UA 的证书与通道、DNP3 的 SAv5、61850 的认证，都不能与业务编解码混在一起。
- 协议类型不进入 `modules`。领域层只见到点值与事件。

## 6. 工作分解

角色：A 架构，D 驱动开发，Q 验证。

### PRT-P0 规约冻结与骨架（5 人日，第 1 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| TCP-P0-01 | 目标协议与规范版本确认 | A+Q | 2 | 无 | 协议清单与规范来源 | 评审记录 |
| TCP-P0-02 | 传输组件接口草案 | A | 3 | 01 | 端口头与连接生命周期图 | 评审通过 |

### PRT-P1 OPC UA 客户端（25 人日，第 2 到 5 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| TCP-P1-01 | 二进制编解码与节点读取 | D | 10 | P0-02 | 编解码与单元测试 | 与公开服务器对读 |
| TCP-P1-02 | 安全通道与证书管理 | D | 8 | 01 | 安全策略实现 | 证书轮换用例 |
| TCP-P1-03 | 订阅与数据变化通知 | D | 5 | 01 | 订阅模型 | 模拟服务器用例 |
| TCP-P1-04 | 产品接入与验收 | Q | 2 | 03 | 验收任务 | 端到端抓包 |

### PRT-P2 61850 MMS（25 人日，第 6 到 9 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| TCP-P2-01 | ASN.1 与 MMS 编解码 | D | 12 | P0-02 | 编解码库 | 标准样例报文对拍 |
| TCP-P2-02 | 数据集与报告控制块 | D | 8 | 01 | 模型映射 | 模拟 IED 用例 |
| TCP-P2-03 | 产品接入 | Q | 5 | 02 | 验收任务 | 端到端抓包 |

### PRT-P3 DNP3 与 BACnet（20 人日，第 10 到 12 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| TCP-P3-01 | DNP3 链路层与应用层 | D | 10 | P0-02 | 适配器 | 抓包与场景用例 |
| TCP-P3-02 | DNP3 SAv5 | D | 4 | 01 | 安全扩展 | 认证用例 |
| TCP-P3-03 | BACnet/IP 对象与 COV | D | 6 | P0-02 | 适配器 | 模拟器用例 |

合计约 75 人日，12 周。P1 与 P2 可并行，但共用一个人力时不建议同时开。

## 7. 风险与开放问题

1. OPC UA 与 61850 的安全栈能否复用现有 OpenSSL 封装，还是要独立依赖。
2. 61850 是否需要 GOOSE 与 SV，这是另一个量级，且涉及时序与网络设计。
3. 国网主站协议的版本与省份差异，需按项目规范逐条核对。
4. 是否引入第三方协议栈（如 open62541），许可证与可裁剪性要评估。
5. 实时现场总线明确不做，需与销售口径对齐，避免承诺。

## Appendix A. 参考

- `foundation/src/protocol/`、`foundation/include/`
- `adapters/io_drivers/src/`、`adapters/io_drivers/include/driver_manager.h`
- `docs/design/NORTHBOUND_ARCHITECTURE.md`
- `notes.md`（北向能力现状）
