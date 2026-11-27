# DCU-P3-2-WRITE-ASYNC 契约设计：write_async 的 net_buf 引用所有权

- 基线：`feat/dcu` = `9d065b6d`
- 分支：`dcu/P3-2-write-async`
- 契约头：`modules/edge_io/include/io_executor.h`（不受 `contracts/schema/core_readonly_whitelist.txt` 保护，未登记）
- 规范条目与测试映射：`spec.md`（WA-1..WA-8、INV-1..INV-4、AC-1..AC-6）

## 1. 选定的契约

**A. 驱动在成功后接管一份引用，调用方只在调用返回后释放自己那份。**

- 调用方创建 `net_buf`（ref=1），调用 `write_async`；返回后无论 rc 都
  `net_buf_unref(buf)`，不再依赖该缓冲。
- `write_async` 返回 0 后，驱动保证不再依赖调用方的引用：
  - 需要跨调用使用帧内容的实现（poll、UDP、HTTP）必须 `net_buf_ref` 一份自持，
    在完成事件（TX_DONE / ERROR）、写失败或 destroy 时释放；
  - 同步消费完的实现（MQTT 由 Paho 同步拷贝、OPC UA 桩忽略内容并同步完成）
    不持有引用，返回后与缓冲再无关系。
- `write_async` 返回 < 0 的同步错误路径**不接管任何引用**；若实现内部先 ref
  后失败，必须在返回前自己 unref。
- 完成/失败回调在 loop 线程投递，payload 为 NULL，回调不接触缓冲；驱动在帧
  不再使用后释放自持引用（事件前后均可），最晚在 destroy 时释放。同步完成的
  实现在 `write_async` 返回前投递事件，且不持有引用。
- `destroy` 必须释放所有尚未完成的驱动自持引用。

## 2. 为什么选 A 而不是 B

B（调用方保留全部引用到完成事件）要求调用方把每一帧和对应的 TX_DONE 配对。
现有 `IO_EVENT_TX_DONE` 的 payload 为 NULL，且 `session_manager_tick` 一次可
返回多帧、poll 驱动允许一帧在途，调用方无法知道哪一帧完成；要实现 B 必须改
事件模型（每帧票据），波及所有驱动和引擎，收益仅是让驱动的引用负担转移给调用
方，风险更大。A 已经是 poll 驱动的既成事实，也是 slave/master 引擎的既有写法，
改动面最小。

## 3. 实现 × 路径 × 引用归属（现状 → 目标）

| 实现 | 成功（异步） | 成功（同步完成） | 同步失败 rc<0 | 完成/销毁 | 现状缺陷 | 目标 |
| --- | --- | --- | --- | --- | --- | --- |
| poll（UART/TCP/fd） | `net_buf_ref` 存 `pending_tx` | 不适用 | `-EBUSY` 不 ref；`uv_poll_start` 失败时仍持有 `pending_tx` 的 ref | TX_DONE / 硬错误 / FLUSH / destroy 时 unref | 启动写事件失败 → 引用泄漏且 `pending_tx` 卡死 | 失败时 unref 并清 `pending_tx` |
| UDP | 不 ref，uv_udp_send 异步读取 `buf->b.data` | 不适用 | `malloc`/`uv_udp_send` 失败：req 泄漏，缓冲不持 | 发送回调不 unref | 调用方返回后立即 unref → 异步发送 use-after-free | 成功前 ref，send 回调 unref；同步失败释放 req、不持引用 |
| HTTP（libcurl multi） | 不 ref，curl 在传输期间读取 `buf->b.data` | 不适用 | `curl_multi_add_handle` 失败不持引用 | CURLMSG_DONE 不 unref | 调用方返回后立即 unref → 传输期间 use-after-free | 成功时 ref 存 `http_tx_nb`，CURLMSG_DONE / destroy 时 unref；在途时再写返回 `-EBUSY` |
| MQTT（Paho Async） | `MQTTAsync_send` 在返回前同步拷贝 payload | 不适用 | `MQTTASYNC_SUCCESS` 之外不持引用 | 无需释放 | 无（不依赖调用方引用） | 保持不持引用，契约注明“同步拷贝实现可不持” |
| OPC UA 引擎 | 忽略帧内容，同步投递 TX_DONE | 是 | 无 | 无需释放 | 无（不依赖调用方引用） | 保持；契约注明同步完成不持引用 |

调用方侧（谁 alloc / 谁 unref）：

| 调用点 | 现状 | 目标 |
| --- | --- | --- |
| `slave_engine.c` `_slave_io_cb` / `_slave_on_timer` | alloc 后调用，返回后立即 unref | 已符合 A，不改 |
| `master_engine.c` `_process_req_result` / `_dispatch` | alloc 后调用，返回后立即 unref | 已符合 A，不改 |
| `passthrough_mgr.c` `_on_read` | alloc 后调用，**从不 unref** | 调用返回后 unref 自己的引用（无论 rc） |

## 4. 泄漏 / 双释放风险

- 旧 `passthrough_mgr.c`：alloc 的引用无人归还 → 每个透传帧泄漏一个缓冲；
  若驱动按旧习惯自己 unref 调用方的引用（历史 mock 即如此），则表现为双释放。
- 旧 UDP/HTTP：驱动不持引用，调用方按 poll 的 A 契约立即 unref → 异步使用已
  归还池子的内存（use-after-free）。
- 旧 poll：`uv_poll_start` 失败时 `pending_tx` 已 ref，返回值却是错误；调用方
  按 A 只归还自己那份 → 驱动自持的那份永久泄漏，且下一次写被 `-EBUSY` 卡死。
- 修复后统一的断言：成功异步路径在途时池中恰少一个缓冲（驱动自持）；完成/失败
  /销毁后回到基线；同步失败路径调用返回后立刻回到基线。

## 5. 证据与验证

- 红测试（commit 1，仅测试）：fake 异步 executor（记录 ref/unref 次数、可延迟
  完成、可同步失败）驱动 `passthrough_mgr`，以及真实 UDP 回环验证驱动自持引用。
- 修复（commit 2）：契约头注释 + 上述驱动/透传对齐。
- 守卫：`ci/mutation_guards.tsv` 追加本单条目，去掉 passthrough 的 unref 后
  定向新测试必须变红。
