# ADR 0011: 北向通信分两族——推送型与服务型分离，地址规则归编排层

## 状态
已通过 (Accepted, 2026-10-31)

## 上下文

系统当前对"北向"只有一个实现：`northbound_mgr` 里的 **MQTT + Sparkplug B**，
协议名与 topic 规则都硬编码在管理器里（`northbound_mgr.c:251`、`_nb_topic()`）。

但代码事实表明"北向"不止这一种形态：

1. **IEMS 早已用从站协议做北向**：`cmd/iems/iems_app.c:534` 有一条
   `NORTHBOUND` 类连接，实现是 `bus_orchestrator_setup_slave(..., "ModbusRTU", 1)`
   —— 让本地 SCADA 来读，而不是往云推。
2. **8 个驱动已具备从站应答能力**：`modbus_rtu` / `modbus_tcp` / `dlt645` /
   `dlt698` / `iec101` / `iec104` / `dlms` / `cia402` 都实现了 `req_slave_exchange`。
3. **产品需求各不相同**：DCU 北向是 **DL/T 698.45**（对配网主站，DCU 是服务端）；
   IEMS 要 Modbus TCP 给本地 SCADA；SmartBox 目前完全没有北向；
   Edge-Ops 控制面要 **MQTT over WebSocket**。

若继续把"北向"当成"一个协议清单"，会得到一个错误的抽象：把从站应答塞进
`publish(topic, payload)` 模型。IEC104 从站不是"发布"，它是"应答"；
OPC UA Server 没有 topic，它的地址是 NodeId。

## 决策

1. **北向分两族，编排分开**
   - **族 A 推送型**（edge → 云）：`northbound_mgr` 独立线程 + `nb_transport_ops_t`
   - **族 B 服务型**（SCADA → edge 读）：`bus_orchestrator` 从站 + `req_slave_exchange`
     + `edge_slave_source_ops_t`
   - 两族**共用取数端口思想**（产品实现、组合根注入），但**不共用编排**。

2. **驱动只做 payload 编码，不给 `edge_driver_t` 加 `req_topic`**
   地址（MQTT topic / OPC UA NodeId）由编排层按**每路传输各自的模板**渲染。
   理由：OPC UA 没有 topic，把地址塞进驱动等于逼它拼一个假 topic；
   而 topic 前缀本来就是部署配置，不同现场不同。

3. **传输支持数组多路并存**
   工业现场"云端 MQTT + 本地 SCADA OPC UA"是常态，单传输做不到。

4. **下行控制必须过聚合根**
   `subscribe` → 驱动 `req_decode` → 产品端口 `accept()` → 聚合根领域方法。
   禁止直接写影子表（会绕过五防闭锁 / SOC 包络）。这是架构原则 3 的直接推论。

5. **SnF 送达判定改为整批确认（at-least-once）**
   `publish` 返回 0 只代表入队。新增 `pending()` 在途计数，为 0 才把记录标 `synced`。
   允许重复，不允许丢。

6. **第二协议选 OPC UA**，并救活 `opcua_engine_create()` 死代码。

7. **MQTT over WebSocket 纳入范围**（`EDGE_OPS_PRD.md:7` 的控制面通道）。

## 被否方案（及其被否理由）

| 方案 | 内容 | 为什么否 |
|---|---|---|
| B | 新定义 `nb_protocol_ops_t`（`topic_for`/`encode`/`decode`），南向北向彻底分离 | 与 ADR-002 已落地的 `req_*` 四件套形成两套 codec 抽象，同一份编解码要写两遍 |
| C′ | 给 `edge_driver_t` 加 `req_topic`，topic 归驱动 | OPC UA 没有 topic；驱动会被迫拼假 topic，且 topic 前缀是部署配置不是协议知识 |
| D | 把族 B 塞进 `nb_transport_ops_t`（从站也走 publish） | IEC104/Modbus 从站是"应答"不是"发布"，模型错位；且现有 `req_slave_exchange` 已能用 |
| E | SnF 逐条序号确认（publish 带出 seq，DB 加列） | 语义最精确，但要改 `publish` 签名 + `telemetry` 表结构；第一期用整批确认即可满足"不丢" |
| F | 第二协议用裸 MQTT JSON（最简单） | OPC UA 价值更高：本地 SCADA 刚需，且能把已编译但无人调用的 `opcua_engine` 救活 |
| G | 保留 `printf` 假成功（未连接时返回 0） | 与南向 OI 8000 的假成功同源：调用方以为成功，数据静默消失 |

## 后果

**正面**
- 族 B 的 8 个驱动能力被解锁，工作量从"重写协议"降到"接线 + 配置"
- 换北向协议不改编排代码（改配置模板 + 换驱动）
- 断网不再静默丢数据；重连能恢复会话（重发 BIRTH）

**负面 / 风险**
- 两族并存，读代码的人需要先分清在说哪一族（文档必须写清楚）
- 整批确认是 at-least-once，云端可能收到重复（遥测按 `(ts, tag_id)` 幂等）
- `edge_point_value_t` 需要新增 `type_tag` 字段（698/DLMS 的类型标签无法从 width 推出）
- 族 B 的监听端口扩大攻击面，鉴权要单独做

## 详细架构设计

现状核对、业界参照、实施计划与验收标准见：
[docs/design/NORTHBOUND_ARCHITECTURE.md](../design/NORTHBOUND_ARCHITECTURE.md)

## 相关

- ADR-0010（无全局契约 / 产品用行业原生标识）：北向 metric 名与 698 OAD 类型
  都由产品端口给，**不建全局名表**，与本 ADR 的"地址归编排、类型归产品"同源。
- ADR-0005（三后端 DataHub）：族 A 的 `nb_source_ops_t` 正是为了不直接依赖 `shm_ring_t`。
