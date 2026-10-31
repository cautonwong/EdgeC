# ADR-002 迁移计划

> **目的不是"把驱动迁一遍"，而是让下面这四样东西在代码里不存在。**
> 迁移只是手段；如果迁完还留着它们，这次改造就算没做。
>
> 前置：S1（645 纯 codec，#456）、S3-a（CAN 纯 codec + 请求双标识，#459）已完成。

---

## 0. 硬目标：这四样必须删掉

| # | 对象 | 终态 | 状态 |
|---|---|---|---|
| 1 | `driver_contract_injection_t`（整个类型） | **类型不存在** | ✅ 2026-10-31 已删 |
| 2 | `lookup_by_tag` | **字段不存在** | ✅ 已删 |
| 3 | `lookup_by_key` | **字段不存在** | ✅ 已删 |
| 4 | `read_shadow`（含 `edge_driver_read_shadow()` 内联） | **字段不存在** | ✅ 已删 |

连带一起走的（同一个病根，不删就是留后门）：

| # | 对象 | 说明 | 状态 |
|---|---|---|---|
| 5 | `publish` / `get_tag_count` | 同样是"驱动反向读消费侧"，和 `read_shadow` 一类 | ✅ 已删 |
| 5b | `get_tag_name` | 同上 | ✅ 已删（S6：sparkplug_b 改走北向端口 `name()`，名字经 `point_desc` 传入） |
| 6 | `meta_list` / `meta_list_len`（ctx 上的） | 改由 `edge_api_set_protocol_meta()` 承载，请求层在 `req_prepare` 时交给驱动 | ✅ ctx 字段已删，登记已接上（`dcu_driver_adapter_set_meters`） |
| 7 | `driver_manager.c` 的 `__attribute__((weak)) edge_model_find` 兜底 | 隐藏单例的根源，两个产品同时链接会互相覆盖 | ✅ 已删 |
| 8 | `cmd/dcu` 与 `cmd/iems` 里的 `edge_model_find` 强实现 | 弱符号的对偶，一起删 | ✅ 已删 |
| 9 | `build/*/generated/contracts.c/.h` 里的引用 | 生成物，跟着模板一起改 | ✅ 已无引用 |

**删完之后的实际形状**：`bus_orchestrator_setup_master/slave(bus, proto, device_id)` 不再收注入包；
`session_manager_add_session(sm, proto, device_id, user_data)` 也不再收。点位翻译只剩一处入口——
产品的 `*_driver_adapter_native_id()`，驱动拿到的是协议原生标识。

### 验收标准（可执行，不靠感觉）

```bash
# 2026-10-31 实测：三条都是 0 行（只剩注释里提到名字）
grep -rn 'driver_contract_injection_t' --include='*.c' --include='*.h' . | grep -v '^./build/'
grep -rn 'edge_driver_read_shadow\|ctx->read_shadow' --include='*.c' --include='*.h' modules cmd
grep -rn 'edge_model_find' --include='*.c' --include='*.h' modules cmd
```

`lookup_by_tag|lookup_by_key` 还剩 `edge_anti_corruption_lookup_by_tag`（ACL 防腐层自己的查找，
与驱动无关）和两行说明性注释。

门禁已落地：`scripts/check_architecture_layers.sh` 第 14 道。

> 任何 `driver_*.c` / `modbus_*_driver.c` 出现 `lookup_by_*` / `edge_driver_read_shadow` /
> `driver_contract_injection_t` / `edge_model_find` → 直接判红（纯注释行放行）。

**没有这道门禁，删完的东西会被下一个提交悄悄加回来。** 门禁是这次改造的"锁"，不是可选项。
正负样本都验过：干净树上通过，临时插一行 `edge_driver_read_shadow` 立刻判红。

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
| ModbusRTU | ✅ | ✅ | ✅ | 0 | 0 | **已完成（S3-b）** |
| ModbusTCP | — | ✅ | ✅ | 0 | 0 | **已完成（S3-b）** |
| DLT698 | ✅ | — | — | 0 | 0 | **已完成（S3-c）** |
| IEC104 | — | ✅ | — | 16 | 5 | **唯一剩余**（等 `session_ctx` 决策） |
| IEC101 | — | — | — | 0 | 0 | **已完成（S3-c2）** |
| DLMS | — | — | — | 0 | 0 | **已完成（S3-c3）** |
| CIA402 | — | — | — | 0 | 0 | **已完成（S3-e）** |
| HL7-FHIR | — | — | — | 0 | 0 | **已完成（S3-e，北向端口）** |
| CCSDS | — | — | — | 0 | 0 | **已完成（S3-e，北向端口）** |
| SparkplugB / RadarFMCW / Hotplug | — | — | — | 0 | 0 | 非会话型驱动，不涉及 |

**截至 2026-10-31：只剩 IEC104 一个。** 其余 12 个驱动全部纯 codec 化，
`lookup_by_*` / `read_shadow` / `driver_contract_injection_t` 在驱动目录里已归零；
最后两处是 `driver_manager.c` 的注入兜底和 `driver_iec104.c`。

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

### S3-c2 IEC101 / DLMS：从站端口缺一个能力（实测，2026-10-30）

迁 IEC101 时发现现有 `edge_slave_source_ops_t` **不够用**：

```c
/* IEC101 从站响应总召唤（C_IC_NA_1）时做的事 */
for (uint32_t i = 0; i < ctx->meta_list_len; i++) {   // ← 遍历所有点位
    const iec101_map_t *map = maps + i;
    edge_driver_read_shadow(ctx, map->tag_id, &vm);    // ← 逐个取值
    ...组 ASDU...
}
```

它需要**"这个从站有哪些点位"**（枚举），而不只是"给我点位 X 的值"。
现在的端口只有 `resolve(point_id)`，没有枚举能力，所以驱动只能靠
`ctx->meta_list`（产品点位表）——那正是要删的东西。

**缺口**：`edge_slave_source_ops_t` 需要补一个枚举方法，比如

```c
/* 遍历本从站的点位：回调式，避免动态内存 */
int (*enumerate)(void *ctx, void (*fn)(void *ud, uint32_t point_id), void *ud);
```

或者更贴合协议：`resolve_all(ctx, fn, ud)` 直接回调 (point_id, value)。

这是**端口设计问题，不是驱动问题**。IEC101 / DLMS / IEC104 都有总召唤/批量读，
都会撞上同一件事——所以这个能力要一次设计对，别一个驱动加一个方法。

**结论**：IEC101 / DLMS / IEC104 属于同一批，应该在端口补完枚举能力后一起做，
不适合按现在的模式逐个机械迁。

### S3-c3 DLMS：native_id 需要 64 位（实测，2026-10-30）

`dlms_map_t` 里**没有 key 字段**，点位标识是 **48 位 OBIS**（6 字节）：

```c
typedef struct {
    uint8_t  obis[6];      /* ← 48 位，装不进 uint32 */
    uint16_t class_id;
    uint8_t  attribute_index;
    edge_tag_id_t tag_id;
    ...
} dlms_map_t;

/* 驱动里是这么查表的 */
uint64_t key = 0;
for (int i = 0; i < 6; i++) key = (key << 8) | obis[i];
ctx->lookup_by_key(key);
```

而 `edge_api_read_tag_ex` 的 `native_id` 是 `uint32_t`。

**缺口**：`native_id` 要扩到 64 位。影响面：

- `edge_api_read_tag_ex(..., uint64_t native_id, ...)`
- `edge_driver_t.req_prepare / req_encode / req_decode / req_slave_exchange`
- 三个产品的适配层（DCU/IEMS 的 `native_id()` 返回类型）

**为什么不能像 Modbus 那样塞进 32 位**：Modbus 是「地址 16 + 长度 16」正好 32 位；
IEC101 是「类型 8 + IOA 24」正好 32 位；DLMS 的 OBIS 单独就 48 位，
再加 class_id/attr 就更宽。硬压会丢信息。

**最终解法：不动 `native_id`，加一条 `point_desc` 旁路**（2026-10-30 落地）。

原方案是"把 `native_id` 扩到 64 位"，但那条路要改 `edge_api_read_tag_ex` 签名、
四个 `req_*` 签名、三个产品适配层——为了一个**没有产品注册**的协议付全局代价，不划算。
改成：请求对象带一个 `const void *point_desc + size_t point_desc_len`，由调用方把
**协议原生的点位标识**按自己的结构体填进去（DLMS 就是 `{obis[6], class_id, attr}`），
驱动用 `point_desc_len` 校验尺寸、`memcpy` 进 `priv`，不再反查任何表。

- `native_id` 保持 `uint32_t`，只作为"快路径/调试标识"，不再是唯一标识通道。
- 尺寸不匹配就**拒绝请求**——`void*` 不校验长度就是隐患，这条是硬约束。
- 从站侧（`req_slave_exchange` 的 `resolve`）同样接收 `point_desc`，主站/从站两侧统一。
- 收益：以后任何"标识比 32 位宽"的协议（OBIS、OID、长地址）都能直接接，不用再改契约。

### 三个 API 决策汇总（做完才能动最后两个驱动）

| 缺口 | 影响 | 状态 |
|---|---|---|
| 从站端口**枚举**（总召唤/批量读） | IEC101 / DLMS / IEC104 | ✅ 已加（`enumerate`） |
| 宽标识（>32 位） | DLMS（OBIS 48 位） | ✅ 已解决：`point_desc` 旁路，不动 `native_id` |
| **`session_ctx`**（请求级会话状态） | IEC104（STARTDT/APDU 序号） | ❌ 待决策（只剩这一个） |

**建议**：后两个一起设计——都是请求模型的宽度/状态问题，
分两次改会让 `req_*` 签名动两遍。

### 关于测试覆盖（test-platform）

- **已注册到产品的驱动**（645 / CAN / ModbusTCP / RTU / DLT698）：`acceptance/run.sh`
  通过产品端到端覆盖，这是主验证手段。
- **没有产品注册的驱动**（CCSDS / HL7 / CiA402 / IEC101 / DLMS / IEC104）：
  test-platform 没有对应用例可挂（它的用例是产品级的），只能靠单元测试覆盖。
  所以这几个驱动都补了 `tests/test_driver_*.c`。

### S3-d IEC104（已完成，2026-10-31）

104 是面向连接的：STARTDT/STOPDT 握手、APDU 收发序号、I 帧确认。纯 codec 化要求把这些
会话状态从驱动里搬走，而 `edge_req_s` 原来只有一块"请求级 scratch"。

**最后选的不是 A 也不是 B，是 C：把两种 priv 分开。**

`edge_req_s` 早就有 `ctx_size`（设备级持久上下文，SessionManager 持有一份），
只是一直没交给 native 槽位用。所以不需要新增"会话对象"，只要把已有的那块传进去：

| 槽位 | priv（scratch） | session（持久） |
|---|---|---|
| `req_prepare` | 本次要读/写的点 | — |
| `req_encode` / `req_decode` | 同上 | APCI 收发序号、HDLC/COSEM 关联状态 |
| `req_slave_exchange` | 从站一般不用 | 同上 |
| `req_tick` | — | 整个就是 session |

签名变化（一次改完，只改 native 槽位，legacy 四件套不动）：

```c
int (*req_encode)(void *priv, void *session, net_buf_t *out);
int (*req_decode)(void *priv, void *session, const uint8_t *buf, size_t len,
                  size_t *consumed, double *out_value);
int (*req_slave_exchange)(void *priv, void *session, const uint8_t *in, size_t in_len,
                          const edge_slave_source_ops_t *src, void *src_ctx,
                          net_buf_t *out, size_t *consumed);
int (*req_tick)(void *priv, const edge_north_source_ops_t *north, void *north_ctx,
                const edge_slave_source_ops_t *slave, void *slave_ctx,
                net_buf_pool_t *pool, net_buf_t **out_tx);
```

`req_tick` 同时收北向端口和从站端口：北向是"发北向报文"，从站是"测点变化主动上送"，
两者用途不同不能互替（104 从站的自发上报就靠后者）。

**这个拆分顺手修掉了两个隐藏 bug**：

1. DLMS 的 `req_encode` 原来从 scratch 里读 `session_state`——scratch 每请求清零，
   状态永远是 IDLE，主站读**必然失败**。DLMS 没有产品注册，所以一直没暴露。
2. DLMS 没有 `req_priv_size`，`driver_manager_register` 的 `-5` 校验会让注册直接失败。

IEC104 的点位标识用 `IEC_NATIVE_ID(type_id, ioa)`（`iec_asdu.h`），与 IEC101 同一套
32 位编码——两个协议的 ASDU 都是「类型 8 + IOA 24」，正好装得下，不需要 `point_desc`。

**一个踩过的坑**：`iec104_session_t` 一开始 32 个死区槽位，加上 APCI 的 512 字节
超过了 `DRIVER_PRIV_SIZE`(1024)，`driver_instance_create` **静默返回 NULL**，
现象是"会话建不出来、日志什么都没有"。现在驱动里有 `_Static_assert`，
`driver_manager.c` 也补了 `LOG_ERROR`。

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

### S6 北向生产者改纯 codec：`get_tag_name` 退场（2026-10-31）

`edge_driver_ctx_t` 最后剩下的一处"驱动反查消费侧"是 `get_tag_name`：
sparkplug_b 组 metric 名时要读 datahub 元数据。它虽然是**北向**而非南向，
但病根一样——驱动自己去消费侧取状态。

**改法**：名字是"点位模型"的一部分，归调用方。北向端口补一个 `name()`：

```c
struct edge_north_source_ops_s {
    int (*peek)(void *ctx, uint32_t tag_id, pipe_msg_t *out);
    const char *(*name)(void *ctx, uint32_t tag_id);   /* metric 名 */
};
```

名字经 `point_desc` 传给驱动（"这一个点的描述"本来就该包含它的名字），
驱动只负责编进报文。于是：

- `edge_driver_ctx_t` 里**一个消费端指针都不剩**；
- sparkplug_b 变成 native：`req_prepare` / `req_encode`（NDATA）/ `req_tick`（NBIRTH）；
- 北向管理器不再自己复制一份 `spb_ctx_t` 去 cast `ctx->priv`（布局一改就静默错位），
  改成调 `sparkplug_b_identity()`；
- 顺手放宽注册校验：`prepare + encode` 必须成对，`decode` 只有读路径才需要
  （纯北向生产者只发不收）。

回归测试：`tests/test_sparkplug_b.c` 断言 metric 名出现在报文里——证明名字来自调用方，
而不是驱动自己去查。

### S5 写路径：native 驱动拿不到写入值（2026-10-31 发现并修复）

**这是迁移带出来的一个静默 bug，不是新功能。**

native 驱动的 encode 只有一个入口 `req_encode`，而 `req_prepare` 的签名里
**没有 value、也没有"这是读还是写"**。于是：

- `mb_rtu_req_encode` 永远发 `FC03`（读保持寄存器）；
- `legacy_encode` 见 `req_encode` 非空就直奔它，**永远走不到 `encode_write`**；
- 结果：`edge_api_write_tag()` → IEMS 的控制下发（`iems_app.c`）与 REST 写点
  （`rest_server.c`）都会**发出一个读帧**，控制失效且不报错。

`dlt645_req_encode` 里那句 `if (p->ctrl == DLT645_CTRL_WRITE)` 同理——`ctrl`
从来没被置成 WRITE，是死分支。

**修法**：`req_prepare` 补两个形参。

```c
int (*req_prepare)(void *priv, uint32_t slave_id, uint32_t point_id, const void *meta_list,
                   uint32_t meta_list_len, const void *point_desc, size_t point_desc_len,
                   bool is_write, double value);
```

`value` 是 **raw**（调用方负责工程量 → 寄存器值的换算），驱动不碰标定。

**各协议现状**：

| 协议 | 写支持 | 说明 |
|---|---|---|
| ModbusRTU / ModbusTCP | ✅ FC05/06 | 产品实际在用；多寄存器写显式拒绝（请求模型只带一个标量，截断比失败更危险） |
| **DLT645** | ✅ 控制码 0x14 | 2026-10-31 补齐：宽度来自 `dlt645_write_desc_t`（经 `point_desc` 传），值按 BCD 编进数据段；缺宽度或宽度非 2/4 一律拒绝 |
| DLT698 / CAN / IEC101 / DLMS | ❌ 显式失败 | `req_prepare` 里 `if (is_write) return -1` + WARN。**宁可失败，也不发读帧** |
| CiA402 | — | 没有 `req_prepare`（纯从站） |

**为什么停在 645**：其余协议的写编码没有可复用的正确实现，硬写等于猜——

| 协议 | 阻塞点 |
|---|---|
| DLT698 | `dlt698_codec.c` 是简化实现（源码里就写着 "For simplicity"），没有 SET-Request 编码器；要先补 codec 并拿到报文样例 |
| IEC101 / IEC104 | `iec_asdu_add_c_sc_na_1/_c_se_nc_1` 已有，缺的是"写请求的 type_id 从哪来"——控制点位与测量点位的类型不同，需要产品在 `native_id` 里带控制类型，并补一条控制点位的用例 |
| DLMS | 缺 `dlms_cosem_build_set_request` |
| CAN | 数据帧语义由产品定义（哪几个字节是值、字节序），没有统一样例 |

补任何一个之前，先要有**能对照的报文样例**——没有样例的编码器只是"看起来对"。

回归测试：`tests/test_modbus_v3.c` 里三条——写发 FC06、读仍发 FC03、多寄存器写被拒。

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

| 项 | 状态 |
|---|---|
| 抄表请求降 `ARBITER_PRIO_LOW` | ✅ **已做**（2026-10-31）：`edge_api_read_tag(_ex)` 显式收 `priority`，DCU 的 `dcu_bus_source._submit` 与 IEMS 的采集任务都传 `ARBITER_PRIO_LOW`。抄表是后台任务，交互式请求先过总线 |
| 发布身份改 `meter.address` | ✅ **已做**（2026-10-31）：不是把 address 当 device_id，而是给每只表一个 `slot_id` 当影子身份（见下节） |

为什么优先级要显式传而不是在 `edge_api` 里写死：写死的默认值会让"这次读是什么性质"
变成库的猜测。`edge_api_read_tag` 原本固定 `ARBITER_PRIO_NORMAL`，于是抄表和交互式单点读
同优先级——两者抢总线时没有任何区分依据。

### 尾巴 2 的前置发现：`device_id` 是"槽位号"，不是身份

做"发布身份改 address"之前先查了影子表的索引方式，结论是**不能直接把 address 当
`device_id`**：

```c
/* cmd/dcu/dcu_router.c */
slot = device_id * DCU_SLOTS_PER_DEVICE + tag_slot(tag_id);   /* SLOTS_PER_DEVICE = 32 */
```

`device_id` 必须是一个**稠密小整数**（0..N），因为它是槽位号的乘数；而 `meter.address`
是 12 位字符串。直接换会让 idx 冲到天上。所以"改 address"的真实含义是：
**让槽位号跟着电表身份走，而不是跟着数组下标走**（加一个 add 时分配、永不复用的
`slot_id`，并在 `load_from_db` 时按稳定顺序重建）。

查这条链时还发现一个**正在丢数据的隐患**，先修了：

- DCU 的 `datahub_capacity` 原来写死 `32768`，而槽位公式要
  `MAX_METERS(4096) × 32 = 131072`。**第 1025 只表起，影子写入 `idx >= capacity`
  被 `datahub_write_shadow_idx` 静默丢弃**——不报错、不计数、没日志。
- 修法：容量按 `MAX_METERS × DCU_SLOTS_PER_DEVICE` 推导；`ring_capacity` 与
  影子容量解耦（队列只要几千条，早先两者共用一个值才把影子压到 1024 只表）；
  容量不足时启动告警；越界写入打一次性告警（不再静默）。
- 回归：`test_dcu_router` 断言槽位公式的上界落在容量内。

**尾巴 2 已落地（同日）**：`production_meter_t` 增加 `slot_id`，在 `add` 时分配
（单调递增，不复用——复用会让新表继承旧表在影子表里的残留值），`load_from_db` 按
`ORDER BY address` 加载以保证重启后 slot 可复现。`_node_publish_realtime` 用
`meters[idx].slot_id` 当 `device_id`，事件监控改用 `dcu_inventory_by_slot()` 反查地址
（原来直接 `meters[device_id]` 当下标用）。

真正被修掉的 bug 在这里：`dcu_inventory_remove` 是**末元素覆盖删除位**，删一只表就会让
最后一只表的数组下标变化——而影子表按 `device_id * 32 + tag_slot` 索引，于是
**被挪动的那只表的数据会落到别人的槽位上**。`slot_id` 跟着表走（swap 是整体 memcpy），
下标变了身份不变。回归用例：`test_dcu_inventory.test_slot_id_survives_remove`。

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
