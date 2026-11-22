# ADR 0017: 698 从站主动上报（REPORT.notification）端口

## 状态

Proposed（2026-11-22，待架构评审）。

## 上下文

`driver_dlt698.c` 的从站交换只处理 GET/SET/ACTION 与 GET-Record，**没有**服务端主动发起的上报路径；`edge_slave_source_ops_t` 也没有"上报"取数端口。结果是：事件已经落库（`dcu_event_monitor.c` 的 3000/3005），主站却永远收不到 `REPORT.notification`。

标准依据：

- 6.1.8（`docs/698_parsed.md:1248`）：`REPORT.notification` 由服务器应用进程发起，参数是"通知类型 + 对象属性描述符及其数值"。
- 表94（`:3109`）：`REPORT-Notification ∷= CHOICE { [1] 上报若干个对象属性, [2] 上报若干个记录型对象属性, [3] 上报透明数据 }`。
- 表95（`:3124`）：`[1] ReportNotificationList ∷= SEQUENCE { PIID-ACD, SEQUENCE OF A-ResultNormal }`，与 GET-Response-NormalList 的结果项同形。
- 表99（`:3176`）：`[1] ReportResponseList ∷= SEQUENCE { PIID, SEQUENCE OF OAD }`。
- 6.2.3（`:1569`）：收到 `REPORT.indication` 后客户机回 `REPORT.response`；服务器收到 `REPORT.confirm` 才认为发送成功，超时重发。

约束：

- `foundation/include/edge_slave_plane.h` 保持协议中立：`dlt698_get_item_t`、DAR、A-XDR 不得进入公开端口。
- 组件不用动态内存；生产者在自己的线程里组包，缓冲由调用方给。
- 上报只能在**活动连接**上发出，且不得发到另一条会话。

## 决策

### 1. 端口追加一个上报子结构

```c
typedef struct {
    /* 写一帧上报的 APDU 体（服务标识之后）：CHOICE | PIID | 个数 | 结果项。
     * *len == 0 表示当前没有待上报内容。缓冲不足返回非 0。 */
    int (*next)(void *ctx, uint8_t *body, size_t cap, size_t *len);
    void *ctx; /* 上报端口自己的上下文，可与 src_ctx 不同 */
} edge_slave_report_ops_t;

struct edge_slave_source_ops_s {
    ...
    edge_slave_report_ops_t report;
} edge_slave_source_ops_t;
```

- `next` 返回的是**服务体**，不是整帧。产品用 codec 组 `CHOICE + PIID + 结果项`；驱动补服务标识 `0x08`、时间标签并封帧。协议类型（OAD、A-XDR 标签）留在产品与 codec，端口只传字节。
- `PIID` 归产品分配：它是上报的服务序号，驱动从 `body[1]` 读出，用来匹配 `REPORT-Response`。
- `ctx` 放进子结构而不是复用 `slave_src_ctx`：`slave_src_ctx` 是所有取数回调共享的点位模型上下文，上报队列是另一个拥有者。

### 2. 驱动在 `req_tick` 轮询，不在每次交换里轮询

`req_tick` 是已有的"周期发帧"钩子，从站引擎的 1s 定时器（`slave_engine.c` `_slave_on_timer`）驱动它。选择它而不是在 `req_slave_exchange` 里顺带上报，理由：

- 交换回调只在收到主站帧时触发。事件不依赖主站先说话，主站可能长时间沉默。
- `req_tick` 已经有 `net_buf_pool_t` 与 `out_tx` 输出口，不需要新接口。

发送条件（缺一不发）：

1. 会话已 `connected`（CONNECT 成功）；
2. 会话里没有"已发未确认"的上报（`report_pending` 为假）；
3. 端口 `report.next` 存在，且 `next` 返回 `len > 0`。

### 3. 确认、无连接与会话隔离

- 驱动发出上报后置 `report_pending = true` 并记下 PIID；收到 `REPORT-Response`（服务 `0x88`，CHOICE `[1]`）且 PIID 相等时清掉 pending。PIID 不等的响应忽略（防止串会话误清）。
- 无连接（未 CONNECT 或已 RELEASE）时驱动不调用 `next`。
- 新连接接管时 `_slave_attach` 调 `session_manager_reset_sessions`，它调驱动的 `on_init`；`on_init` 清 `connected` 与 pending，因此旧会话里排队的上报不会被发到新连接的主站。
- 有界解锁：发出时记 `report_sent_ms`；超过 `DLT698_REPORT_TIMEOUT_MS`（5s）仍未确认就清 `report_pending`，
  让下一帧能发。**不重发同一帧**（best-effort），只是不让一次丢确认把后续上报永久卡死。
- 标准 6.2.3 的"未确认重发、达到次数放弃"与存储转发（SnF）仍不在本期：需要跨请求的重发状态与持久化，另立任务。

### 4. 内容分阶段

- 本期只实现 `CHOICE [1]` 对象属性列表：codec 的 `dlt698_encode_report_notification` 组体，`dlt698_encode_report_frame` 封帧。
- `CHOICE [2]` 记录型上报返回独立的"不支持"码（`DLT698_REPORT_UNSUPPORTED_RECORD`），等 D1-07 的记录取数路径接入后再实现。**不**用空列表假装成功。
- 生产来源本期**不接**。原先想报事件对象（Class 24，OI 3000/3005）的属性 4，但 D1-18b 之后 A4 是
  **最大记录数**（常量），把它当"当前记录数"上报就是假语义；A3 才是当前记录数，且是四张记录表的计数结构，
  当前对象模型尚未暴露（只有记录表 1 的 `record_count`）。在 `dcu_698_report.c` 里手工拼 A3 会制造第二份
  真相，所以等 D1-07 的记录路径（`CHOICE [2]` 记录型上报）落地后由事件侧直接读对象模型入队。
  端口、驱动与队列模块保留，用合成条目测试；生产队列为空时 `next` 返回 0 长度，驱动不发帧。

### 5. 线程边界

事件入库在 stream engine 的独立线程；`req_tick` 在 IO 事件循环线程。上报队列用固定环形缓冲 + `uv_mutex` 保护，编码在出队（IO 线程）一侧完成；入队只拷贝 OAD、类型与标量原始值，不读共享影子。

## 后果

- `edge_slave_source_ops_t` 末尾多一个子结构；既有字段顺序不变，产品用指定初始化器，编译不受影响。
- 连接/断开时多了 `on_init` 的清理；其他从站驱动未实现 `on_init`，行为不变。
- 主站侧需要按标准对上报回 `REPORT-Response`；不回则驱动停在 pending，后续事件在本期内不会上报（已知限制，见 §3）。
- 组件新增 `cmd/dcu/dcu_698_report.c`（线程安全的固定环形队列 + `next`）。上报端口与既有取数端口在
  `dcu_north_slave_attach` 里一次性装配进同一个 `edge_slave_source_ops_t`，不改动组合根（`main.c`）的
  行数预算。
