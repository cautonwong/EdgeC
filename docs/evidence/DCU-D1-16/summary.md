# DCU-D1-16 698 从站主动上报（REPORT.notification）

基线 `ed104b2b`（D1-18a/b/c、D1-19、D1-07 已合入），分支 `dcu/D1-16-report`。

目标：让 698 从站能对真实事件发起 `REPORT.notification`，而不再只打印一行"要上报"。

## 标准依据

- REPORT.notification 服务：6.1.8.1 表20。
- `REPORT-Notification` CHOICE：表94（`docs/698_parsed.md:3109`）：`[1]` 对象属性、`[2]` 记录型对象属性、`[3]` 透明数据。
- `ReportNotificationList`：表95（`:3124`）：`SEQUENCE { PIID-ACD, SEQUENCE OF A-ResultNormal }`，结果项与 GET-NormalList 同形。
- `REPORT-Response`：表99（`:3176`）：`SEQUENCE { PIID, SEQUENCE OF OAD }`。
- 确认/重发：6.2.3（`:1569`），收到 `REPORT.confirm` 才算发送成功。

## 端口（ADR-0017）

```c
typedef struct {
    /* 写一帧上报的 APDU 体（服务标识之后）：CHOICE | PIID | 个数 | 结果项。
     * *len == 0 表示当前没有待上报内容。 */
    int (*next)(void *ctx, uint8_t *body, size_t cap, size_t *len);
    void *ctx;   /* 上报队列自己的上下文 */
} edge_slave_report_ops_t;
```

`edge_slave_source_ops_t` 末尾追加 `edge_slave_report_ops_t report`；既有字段顺序不变。
端口里没有 698 类型：产品用 codec 组 `CHOICE + PIID + 结果项`，驱动补服务标识、时间标签并封帧。

装配点：`dcu_north_slave_attach` 的 `src`（include + `.report = {.next = dcu_698_report_next}` 两行）。

## 发送触发

- 驱动在 `req_tick` 轮询：从站引擎 1s 定时器驱动它，不依赖主站先说话。
- 只有同时满足才发：会话 `connected`、没有"已发未确认"、`report.next` 返回 `len > 0`。
- 发出后置 `report_pending` 并记 PIID；收到 `REPORT-Response`（0x88）且 PIID 相等才清。
- 有界解锁：超过 5s 未确认就清 `report_pending`，让下一帧能发；不重发同一帧（避免一次丢确认把后续上报永久卡死）。
- 无连接（未 CONNECT / 已 RELEASE）时不调用 `next`。
- 新连接接管：`session_manager_reset_sessions` 调驱动 `on_init`，清 `connected`/pending，旧队列不会发到新主站。

## 内容

- 本期只做 `CHOICE [1]`：`dlt698_encode_report_notification` 组体、`dlt698_encode_report_frame` 封帧。
- `CHOICE [2]` 返回独立码 `DLT698_REPORT_UNSUPPORTED_RECORD`；等 D1-07 记录路径接入后再实现，不用空列表假装成功。
- **生产来源本期不接**。D1-18b 后 Class 24 的 A4 是最大记录数（常量），不能当当前数上报；A3 才是当前记录数，
  且是四表计数结构，对象模型尚未暴露。在 `dcu_698_report.c` 里手工拼 A3 会变成第二份真相，所以等 D1-07 的
  `CHOICE [2]` 记录型上报落地后由事件侧直接读对象模型入队。端口、驱动与队列模块保留，用合成条目测试；
  生产队列为空时 `next` 返回 0 长度，驱动不发帧。

## 队列与线程

`cmd/dcu/dcu_698_report.c`：固定环形缓冲（32）+ `uv_mutex`。事件入库在 stream engine 线程，
出队在 IO 事件循环线程；入队只拷贝 OAD/类型/标量原始值，编码在出队侧用 `dcu_698_encode_report_notification`。
没有动态内存（`check_no_dynamic_memory.py` PASS）。

## 测试

- `test_driver_dlt698`：CONNECT 后 fake 源产生一帧，断言 APDU 字节 `08 01 07 01 30000400 01 12 00 01 00`
  与封帧；无连接不发且不取 `next`；PIID 匹配的 `REPORT-Response` 清 pending，之后可再发；
  确认丢失时 5s 后解锁，下一帧能发（慢测，真等 5s，同 IEC104 t1）。
- `test_dcu_698_report`：一个入队事件产生恰好一帧；两事件合帧；`[2]` 返回不支持码。

## 门禁

见 `tests.log` / `comments.md` / `diffstat.txt`：全量构建 0 warning，ctest 336/336，ASAN 2/2，
fanalyzer 0 new，组合根预算 clean，`git diff --check` clean。

## 没做 / 已知限制

- 记录型上报 `[2]`、重试与超时、SnF 存储转发：另立任务。
- 生产主动上报：等 D1-07 `CHOICE [2]` + 对象模型暴露 A3 后接入；本期事件侧不入队，避免 A4 假语义。
- 主站不回 `REPORT-Response` 时驱动停在 pending，本期后续事件不再上报（标准 6.2.3 的重发未做）。

## 交接注意

- 改了 `cmd/dcu/dcu_north_adapter.c`（两行）——该文件原属他人维护；改动只为把新端口接进唯一的从站源装配点，
  并已按 D1-13/D1-17 先例登记组合根预算。rebase 到 `2ee06750` 后保留了 main 的变量形状/类拆分与 D1-07
  record 回调，只在其上叠 `.report` 行；预算实测 203，与登记的 `201 -> 203` 一致
  （D1-07 的记录回调 191 -> 201，本项 201 -> 203）。合并时请与 north adapter 负责人排序。
