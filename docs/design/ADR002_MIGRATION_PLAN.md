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

### S3-b ModbusTCP → ModbusRTU（主力）

- 覆盖 3 个产品，使用面最广。
- 先 TCP 后 RTU：TCP 无 CRC/帧间静默，先把 `req_*` 在无物理层约束下跑通，再处理 RTU 的 CRC16。
- **RTU 的 3.5 字符间隔已经在 arbiter 里**（`arbiter_set_bus_min_gap`），
  迁移时要顺手删掉驱动里那段等间隔代码，否则两处都在管。
- 量纲：Modbus 的 `scaler` 目前乘在驱动里，迁走后由调用方经 `scale` 参数给（#459 已铺好）。

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

## 6. 顺序与依赖

```
S3-b ModbusTCP/RTU ──┐
S3-c DLT698 ─────────┤
S3-d IEC104（session_ctx）──┴──→ S3-e 处理未注册的 5 个 ──→ P2 一次性删除（§0 九项）
                                                              ↓
                                                    抄表优先级降 LOW
```

**并行度**：b/c 互不依赖，可并行 PR；**P2 必须一次做完**，删一半比不删更糟。
