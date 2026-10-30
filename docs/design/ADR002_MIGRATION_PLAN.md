# ADR-002 迁移计划

> **目的不是"把驱动迁一遍"，而是让下面这四样东西在代码里不存在。**
> 迁移只是手段；如果迁完还留着它们，这次改造就算没做。
>
> 前置：S1（645 纯 codec，#456）、S3-a（CAN 纯 codec + 请求双标识，#459）已完成。

---

## 0. 硬目标：这四样必须删掉

| # | 对象 | 现在的位置 | 终态 |
|---|---|---|---|
| 1 | `driver_contract_injection_t`（整个类型） | `modules/edge_core/include/contract_data_types.h` | **类型不存在** |
| 2 | `lookup_by_tag` | `edge_driver_ctx_t` 字段 | **字段不存在** |
| 3 | `lookup_by_key` | `edge_driver_ctx_t` 字段 | **字段不存在** |
| 4 | `read_shadow`（含 `edge_driver_read_shadow()` 内联） | `edge_driver_ctx_t` 字段 | **字段不存在** |

连带一起走的（同一个病根，不删就是留后门）：

| # | 对象 | 说明 |
|---|---|---|
| 5 | `publish` / `get_tag_count` / `get_tag_name` | 同样是"驱动反向读消费侧"，和 `read_shadow` 一类 |
| 6 | `meta_list` / `meta_list_len`（ctx 上的） | 已由 `edge_api_set_protocol_meta()` 取代（#459） |
| 7 | `driver_manager.c` 的 `__attribute__((weak)) edge_model_find` 兜底 | 隐藏单例的根源，两个产品同时链接会互相覆盖 |
| 8 | `cmd/dcu` 与 `cmd/iems` 里的 `edge_model_find` 强实现 | 弱符号的对偶，一起删 |
| 9 | `build/*/generated/contracts.c/.h` 里的引用 | 生成物，跟着模板一起改 |

### 验收标准（可执行，不靠感觉）

```bash
# 必须全部返回 0 行
grep -rn 'driver_contract_injection_t' --include='*.c' --include='*.h' . | grep -v '^./build/'
grep -rn 'lookup_by_tag\|lookup_by_key' --include='*.c' --include='*.h' modules cmd
grep -rn 'read_shadow' --include='*.c' --include='*.h' modules cmd
grep -rn 'edge_model_find' --include='*.c' --include='*.h' modules cmd
```

并且 `scripts/check_architecture_layers.sh` 增加一道门禁：

> 任何 `driver_*.c` / `*_driver.c` 出现 `lookup_by_*` / `read_shadow` / `edge_driver_read_shadow`
> → 直接判红。

**没有这道门禁，删完的东西会被下一个提交悄悄加回来。** 门禁是这次改造的"锁"，不是可选项。

---

## 1. 为什么不能先删再迁

`edge_driver_ctx_t` 的 8 个回调被 **10 个驱动**同时引用。删字段的那一刻，10 个文件一起编不过——
这不是"逐步清理"，是一次性引爆。

**#459 已经实测过这个坑**：把 `lookup_by_tag/key` 无条件置 NULL 后，IEMS 一条读数都没有
（`assert 0 > 0`）——因为 Modbus 等未迁移的驱动还靠它查表。

所以顺序只能是：**先逐个迁完 → 最后一步一次性删**。删是终点，不是过程。

---

## 2. 现状：注册表 ≠ 驱动目录

`modules/edge_io/src/` 下有 13 个 `driver_*.c`，但**三个产品实际注册的只有 6 个**：

| 驱动 | DCU | IEMS | SmartBox | 注入依赖 | legacy 槽位 | 状态 |
|---|:--:|:--:|:--:|--:|--:|---|
| DLT645 | ✅ | ✅ | — | 0 | 0 | **已完成（#456）** |
| CAN | — | ✅ | ✅ | 0 | 0 | **已完成（#459）** |
| ModbusRTU | ✅ | ✅ | ✅ | 16 | 6 | 待迁（覆盖最广） |
| ModbusTCP | — | ✅ | ✅ | 15 | 6 | 待迁 |
| DLT698 | ✅ | — | — | 7 | 6 | 待迁 |
| IEC104 | — | ✅ | — | 16 | 5 | 待迁（最重） |
| IEC101 | — | — | — | 17 | 6 | **无产品注册** |
| DLMS | — | — | — | 7 | 7 | **无产品注册** |
| CIA402 | — | — | — | 2 | 2 | **无产品注册** |
| HL7-FHIR | — | — | — | 1 | 0 | **无产品注册** |
| CCSDS | — | — | — | 1 | 0 | **无产品注册** |
| SparkplugB / RadarFMCW / Hotplug | — | — | — | 0 | 0 | 非会话型驱动，不涉及 |

**真正要迁的是 4 个。** 另外 5 个编译进产物但没有任何产品注册——
它们同样引用着注入包，是"删干净"的拦路石。

---

## 3. 阶段划分

### S3-0 从站面端口（前置，必须先做）

**这是 ModbusTCP 迁不动的原因。** 7 个驱动实现了 `handle_rx`（从站面）：
`modbus_rtu` / `modbus_tcp` / `dlt698` / `iec104` / `iec101` / `dlms` / `cia402`。

它们在 `handle_rx` 里做的是：

```c
const modbus_map_t *map = ctx->lookup_by_key(addr);      // ① 查表
edge_driver_read_shadow(ctx, map->tag_id, &shadow);      // ② 读当前值
edge_driver_publish(ctx, &am);                           // ③ 写回（写请求）
```

①②③ 正是 §0 要删的东西。而 `driver_manager_register` **禁止 legacy 与 native 混用**，
所以不能"主站路径先迁、从站路径留着"——要么整个驱动迁完，要么不迁。

从站面需要的是设计文档 §3.1 里那个端口，目前**还没实现**：

```c
/* modules/edge_io/include/edge_slave_plane.h —— 由消费方（edge_io）定义 */
typedef struct { double value; uint8_t quality; uint64_t ts_ms; } edge_point_value_t;
typedef struct {
    int (*resolve)(void *ctx, uint32_t point_id, edge_point_value_t *out); /* 取当前值 */
    int (*accept)(void *ctx, uint32_t point_id, double value);             /* 接受写入 */
} edge_slave_source_ops_t;
```

- 声明在 edge_io，**实现在 cmd**（只有产品认识自己的点位模型）
- 组合根注入：`bus_orchestrator_setup_slave(..., source_ops, source_ctx)`
- 驱动只接收"已填好的值"，不知道谁提供的 —— 否则就是把 `read_shadow` 换个名字请回来

**S3-0 交付物**：`edge_slave_plane.h` + `handle_rx` 改为 `req_slave_decode` /
`req_slave_encode` 两个纯函数 + 三个产品的 `resolve`/`accept` 实现。

**没有 S3-0，Modbus / DLT698 / IEC104 一个都迁不了。**

### S3-b ModbusTCP → ModbusRTU（主力）

- 覆盖 3 个产品，使用面最广。
- 先 TCP 后 RTU：TCP 无 CRC/帧间静默，先把 `req_*` 在无物理层约束下跑通，再处理 RTU 的 CRC16。
- **RTU 的 3.5 字符间隔已经在 arbiter 里**（`arbiter_set_bus_min_gap`），
  迁移时要顺手删掉驱动里那段等间隔代码，否则两处都在管。
- 量纲：Modbus 的 `scaler` 目前乘在驱动里，迁走后由调用方经 `scale` 参数给（#459 已铺好）。

### S3-b 补充：ModbusRTU 比 TCP 多三个隐藏消费方（实测记录）

TCP 迁完以为 RTU 是"照抄再补 CRC"，实际连撞三次：

| # | 隐藏消费方 | 症状 | 处理 |
|---|---|---|---|
| 1 | `driver_manager_add_poll_target` 的内置轮询器 | 驱动不再乘 scaler，影子表拿到原始值 | 该 API **只有测试在用**，建议连同测试一起改造或删 |
| 2 | `bus_orchestrator_setup_slave` → `engine_create_slave` → `slave_engine` | 从站面 resolve 不到值 | **已修**：引擎加 `set_slave_source` 槽位 + orchestrator 转发（`bcff830c`） |
| 3 | `driver_manager.c:125/153/620/637` | **直接调 `encode_request` / `decode_response`**，绕过请求层；驱动迁成 native 后这些调用直接失败 | 未修：这是第三套轮询实现，与 `dcu_poller`、`edge_api` 重复 |

**结论**：`driver_manager` 的内置轮询器是 ADR-002 的第三个消费方，必须先决定它的去向：

- **建议删**：它的能力与 `dcu_poller`（DCU）和 `edge_api_read_tag`（IEMS/SmartBox）完全重叠，
  且没有产品在用（只有两个测试引用）。留着它，每个驱动迁移都要额外照顾一条老路径。
- 若保留，则要把它改成走 `req_legacy_driver_create`，等于再写一遍请求层。

**这也解释了 RTU 为什么两次都红**：不是驱动写错了，是这条老路径没人管。

### S3-c DLT698（同族）

- 与 645 同源，`dlt645_req_t` 的字段划分可照搬，差异在 OAD 四字节结构与对象属性访问。

### S3-d IEC104（唯一有设计风险的）

104 是面向连接的：STARTDT/STOPDT 握手、APDU 收发序号、I 帧确认。纯 codec 化要求把这些
会话状态从驱动里搬走，而 `edge_req_s` 目前没有"会话上下文"的位置。

| 方案 | 做法 | 代价 |
|---|---|---|
| A（建议） | 扩 `edge_req_s` 加 `session_ctx` + 生命周期钩子 | 请求模型变复杂 |
| B | 104 长期留 legacy | **注入包删不干净，本计划 §0 无法达成** |

选 B 等于放弃 §0，所以只认真做 A。

### S3-e 未注册的 5 个驱动（拦路石）

#### 实测后修正：`read_shadow` 有两种用途，不能一刀切

| 用途 | 谁在用 | 性质 | 归宿 |
|---|---|---|---|
| **南向从站应答取当前值** | modbus / dlt698 / iec104 / iec101 / dlms / cia402 | 驱动反向读消费侧，**是泄漏** | 已被 `edge_slave_source_ops_t` 取代 ✅ |
| **北向生产者读本地状态发帧** | **ccsds / hl7_fhir** | 驱动作为北向出口，读的是本节点状态，**不是泄漏** | 需要单独端口，**不能塞进从站端口** |

ccsds 读的是 `PIPEMSG_TYPE_BATCH`（`payload.batch.ptr` + count），
hl7 读的是 `PIPEMSG_TYPE_EVENT`（`payload.event.param`）——
两者都不是标量，而 `edge_point_value_t` 只带 `value/raw/width/quality/ts_ms`。
硬塞进去会把端口搞成"什么都能装"的万能结构，那和 read_shadow 没区别。

**正确做法**：另开一个端口，只给北向生产者用。

```c
/* modules/edge_io/include/edge_slave_plane.h —— 与从站端口并列 */
typedef struct {
    /* 看本节点某个 tag 的当前状态。用 pipe_msg_t 而不是标量，
     * 因为北向要发的可能是 BATCH / EVENT / 标量，形状由消费方决定。 */
    int (*peek)(void *ctx, uint32_t tag_id, pipe_msg_t *out);
} edge_north_source_ops_t;
```

**这是硬要求，不是可选项。** 三条纪律与从站端口完全一致：

1. **声明在 edge_io**（需要"读本地状态发帧"的是北向编排），**实现在 cmd**，
   组合根注入——和 `edge_slave_source_ops_t` 同构。
2. **端口不进 `edge_driver_ctx_t`**，每次调用现传。进了 ctx 就是把
   `read_shadow` 换个名字请回来。
3. **驱动不知道谁提供状态**：ccsds 只调 `src->peek(tag, &msg)`，
   不知道背后是 datahub、内存镜像还是测试桩。

**不允许的替代方案**：
- ❌ 把 BATCH/EVENT 塞进 `edge_point_value_t` —— 端口会退化成万能结构
- ❌ 给 ccsds/hl7 开"只用 read_shadow"的例外 —— 例外一旦开口，
  后面每个驱动都会来要一个
- ❌ 让北向生产者直接 `datahub_read_shadow` —— 那就是把泄漏写进驱动

#### 分类处理

| 驱动 | 类型 | 处理 |
|---|---|---|
| **IEC101** | 主站+从站，有会话状态 | 按既有模式迁（参考 IEC104） |
| **DLMS** | 主站+从站，COSEM 对象模型 | 按既有模式迁 |
| **CIA402** | 从站（`handle_rx`） | 迁，走 `req_slave_exchange` |
| **CCSDS** | 北向生产者（`on_tick`） | 先加 `edge_north_source_ops_t` 再迁 |
| **HL7-FHIR** | 北向生产者（`on_tick`） | 同上 |

**为什么不再考虑"直接删"**：仓库里有 `AERO_EDGE_PRD.md`（航天）与 `IOT_HUB_PRD.md`，
CCSDS 是航天遥测协议、HL7 是医疗协议——删掉等于砍掉规划中的产品线。
虽然当前没有产品注册它们，但这是"还没接"不是"不要了"。

### S3-e 未注册的 5 个驱动（拦路石）

IEC101 / DLMS / CIA402 / HL7 / CCSDS 没有任何产品注册，但都引用注入包。
两个选项：

- **删掉**（推荐）：13 个驱动里 5 个是纯维护成本
- 或 `EXCLUDE_FROM_ALL` 但**同时把注入引用摘掉**（只不编译不算解决，`grep` 还是会命中）

### P2 一次性删除（§0 的 9 项）

顺序不能颠倒：

1. `driver_manager.c` 弱符号兜底 + `edge_api.c` 对全局符号的直调
2. 两个产品的 `edge_model_find` 强实现
3. `edge_driver_ctx_t` 的 8 个回调 + 两个内联包装
4. `driver_contract_injection_t` 类型本身
5. `build/*/generated/contracts.*` 重新生成
6. 加门禁（§0 的 grep 变成脚本）

---

## 4. 抄表侧的两个尾巴

| 项 | 为什么还没做 | 现在能不能做 |
|---|---|---|
| 抄表请求降 `ARBITER_PRIO_LOW` | `edge_api_read_tag` 曾建在注入包上 | **S3-b 之后可以**：请求已能携带优先级 |
| 发布身份改 `meter.address` | 耦合 datahub 的 `(tag_id, device_id)` 索引 | 需 datahub 改造，单独排期 |

澄清：poller 传给 sink 的**已经是 address**。真正的问题在 `_node_publish_realtime`
用 `meter_idx` 当 `pipe_msg.device_id`，而 datahub 影子表按 `(tag, device_id)` 索引，
**档案重排后 device_id 会指向另一块表**——要在档案变更时重建映射。

---

## 5. 建议不做

- **S2 的 MasterEngine in-flight 表**：S4 的链式提交已用 `priv_data + on_finish` 完成关联，
  645 与 CAN 都端到端跑通。如果 104 走方案 A，它会被 `session_ctx` 取代。

---

## 5.1 边界目标：`datahub` 只出现在 cmd（用户 2026-10-30 明确）

> 规则：`datahub.h` / `datahub_t` / `datahub_*()` **只能出现在 `cmd/`（组合根）**，
> `modules/` 一律经端口。ADR-002 是这条规则在**驱动层**的落地。

### 现状（实测）

| 范围 | 引用 `datahub` 的文件数 | 说明 |
|---|---:|---|
| `modules/` 全体 | **79** | 引擎/管理面/流计算等，超出 ADR-002 范围 |
| `cmd/` | 15 | 组合根，正确 |
| **驱动层**（`driver_*.c` + `modbus_*_driver.c`） | **2** | 见下 |

驱动层剩的两个：

| 文件 | 性质 | 处理 |
|---|---|---|
| `driver_manager.c` | `_driver_port_read_shadow` / `_driver_port_publish` 等把 `ctx->user_data` 当 datahub 用——**这是 legacy 端口的实现**，只为喂 `edge_driver_ctx_t` 那 8 个回调 | **P2 删回调时一起消失** |
| `driver_radar_fmcw.c` | `parse_frame(buf, len, datahub_t *hub)` 直接 `datahub_write_shadow`；**全仓无人调用** | 改为接收 sink 端口，或直接删 |

**所以 P2 完成后，驱动层将完全不出现 `datahub`。** 这正是 ADR-002 的终点之一。

### 剩余的 77 个文件（不在 ADR-002 范围）

`session_manager` / `slave_engine` / `northbound_mgr` / `stream_engine` / `rest_server` 等
仍在模块内直接用 datahub。把 `datahub` 整体收进 cmd 是**另一个更大的议题**
（相当于给引擎层、管理面也各定义一套端口），建议单独立项，不要混进 ADR-002。

**ADR-002 的边界就是驱动层**：驱动不认识 datahub、不认识点位表、不认识 tag_id。
引擎与北向编排怎么拿数据，是下一个议题。

## 6. 顺序与依赖

```
S3-0 从站面端口 ──┬──→ S3-b ModbusTCP/RTU ──┐
                  ├──→ S3-c DLT698 ─────────┤
                  └──→ S3-d IEC104（另需 session_ctx）
                                            ↓
                              S3-e 处理未注册的 5 个
                                            ↓
                              P2 一次性删除（§0 九项）──→ 抄表优先级降 LOW
```

**S3-0 是所有从站型驱动的前置**：它不解决，§0 的 grep 永远清不干净。

**并行度**：b/c 互不依赖，可并行 PR；**P2 必须一次做完**，删一半比不删更糟。
