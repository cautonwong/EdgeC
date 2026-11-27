# DCU-P3-2-WRITE-ASYNC Spec（write_async 的 net_buf 引用所有权）

本 spec 是测试、实现提交与本单元验收的共同依据。实现提交 `f1f5a36e` 的契约头
注释与此处条目一一对应；证据见同目录 `design.md`、`red.log`、`tests.log`、
`asan.log`、`acceptance-dcu.log`。

## 1. 业务规则

| 编号 | 规则 |
| --- | --- |
| WA-1 | `write_async` 返回 0 后，驱动必须自己保证帧内容在返回后继续有效：同步消费完的实现不持引用；跨调用使用内容的实现 `net_buf_ref` 自持，并在完成/失败事件或 destroy 时释放。 |
| WA-2 | 调用方创建缓冲（ref=1），在 `write_async` 返回后立即 `net_buf_unref`，无论返回码。 |
| WA-3 | `write_async` 返回 <0 的同步错误路径不接管任何引用；内部先 ref 的实现必须在返回前归还。 |
| WA-4 | 同步完成的实现（OPC UA 桩、MQTT 同步拷贝）在返回后不依赖调用方缓冲。 |
| WA-5 | poll 驱动的写事件启动失败属于同步错误：必须归还已持有的引用，不得让下一次写卡在 `-EBUSY`。 |
| WA-6 | HTTP 传输在途时再次写返回 `-EBUSY`，不接管新引用；UDP/HTTP 在异步传输期间自持引用。 |
| WA-7 | 透传（passthrough）作为调用方，不得泄漏其分配的引用。 |
| WA-8 | slave/master 引擎“alloc → write_async → unref 自己那份”的既有释放逻辑与本契约一致，不改。 |
| WA-9 | HTTP 完成回调可能在同一回调内 `destroy` 句柄：poll 必须在回调前取走 `pending_tx` 并 `curl_multi_remove_handle`；回调返回后不得再访问 `impl`/easy 句柄，只能 unref 本地保存的引用。 |
| WA-10 | `write_async` 在句柄已在 multi（`CURLM_ADDED_ALREADY`）时视为成功：保留引用，等既有在途传输的完成回调释放；其他 `curl_multi_add_handle` 错误返回负值，不接管引用并清空 POSTFIELDS。 |

## 2. 不变量

| 编号 | 不变量 |
| --- | --- |
| INV-1 | 成功异步路径在途时，缓冲池恰少一份仍被持有的缓冲（驱动自持）；完成/失败/destroy 后回到基线。 |
| INV-2 | 同步失败路径调用返回后，池立即回到基线。 |
| INV-3 | 任何路径都不出现 net_buf 双重释放或 use-after-free。 |
| INV-4 | 完成/错误事件的 payload 为 NULL，回调不访问帧缓冲。 |
| INV-5 | HTTP 回调内 destroy 的路径完成后，池回到基线，且清理阶段无 use-after-free / 二次 unref。 |

## 3. 可观测输出

| 编号 | 可观测输出 |
| --- | --- |
| OUT-1 | `net_buf_pool_audit().free_count` 在在途/完成/同步失败三个时刻的差值。 |
| OUT-2 | fake executor 记录的 write 次数、驱动自持 ref 数、释放次数。 |
| OUT-3 | UDP 回环收到的字节与发送内容一致；HTTP 完成事件到达。 |
| OUT-4 | ASAN 日志中的 net_buf Ref/Unref 序列，且无 heap-use-after-free / double-free 报告。 |
| OUT-5 | 验收门禁汇总“全部合格:2 个验收任务”。 |

## 4. 验收判据

| 编号 | 判据 |
| --- | --- |
| AC-1 | 旧实现下 4 条新断言失败（`red.log`：passthrough 2 条、UDP 1 条、HTTP 1 条）。 |
| AC-2 | 新实现下 `test_io_executor` 15/15（收尾后 17/17）、`test_passthrough` 3/3 通过。 |
| AC-7 | 收尾复跑：callback-destroy 与 ADDED_ALREADY 两条新用例先红（3!=4、7!=0）后绿；ASAN `detect_leaks=1` 下 17/17 exit 0 无报告。 |
| AC-3 | `ctest -R '^(test_driver_|test_io|test_edge_io|test_slave)'` 13/13、`ctest -R 'dcu|dlt698'` 45/45、`test_iems_full_stack`+`test_iems_modbus_tcp_loopback` 2/2 通过。 |
| AC-4 | ASAN 构建下新测试全通过，无 UAF/double-free；LSAN 残留仅为测试 harness/既有 orchestrator 分配。 |
| AC-5 | 变异守卫“去掉 passthrough 的 unref”后新测试红，恢复后绿。 |
| AC-6 | 串行锁下 `./acceptance/run.sh dcu` 退出 0，全部合格:2 个验收任务。 |

## 5. 非目标

- 不改 `IO_EVENT_TX_DONE` 事件模型（不引入每帧票据；因此不选“调用方保留引用到完成”的契约 B）。
- 不改 slave/master 引擎的释放逻辑（已符合 WA-2/WA-8）。
- 不触碰 `cmd/dcu` 对象模型/ACL、`scripts/extract_dcu_698_catalog.py`、`.audit/`、`docs/plans/`。
- MQTT/OPC UA 不新增自持引用（同步消费已满足 WA-4）。
- 不覆盖 `uv_poll_start` 失败路径的构造性测试（该失败需非法句柄状态，无法稳定构造；由代码审查与既有 hard-error 测试兜底，列为残余风险）。

## 6. Spec 条目 → 测试断言映射

| Spec | 测试 | 断言 |
| --- | --- | --- |
| WA-1, WA-6, INV-1, OUT-1, OUT-3 | `test_io_udp_write_async_holds_ref_until_send_done` | 调用方 unref 后 `free_count == 3`；TX_DONE 后 `free_count == 4`；`recv` 内容与 payload 相等 |
| WA-1, WA-2, WA-7, INV-1, OUT-1, OUT-2 | `test_passthrough_write_async_delayed_completion_ownership` | `writes == 1`；在途 `free_count == base - 1`；完成释放后 `releases == 1` 且 `free_count == base` |
| WA-2, WA-3, WA-7, INV-2, OUT-1 | `test_passthrough_write_async_sync_failure_ownership` | 驱动 `-EBUSY` 不持引用时，调用返回后 `free_count == base` |
| WA-1, WA-6, INV-1, OUT-1, OUT-3 | `test_io_http_write_async_holds_ref_until_complete` | 调用方 unref 后 `free_count == 3`；CURLMSG_DONE 后 `free_count == 4`；收到 TX_DONE 或 ERROR |
| WA-5 | 代码审查 + `test_io_write_hard_error_releases_pending_tx`（既有） | 失败路径归还引用（hard error 已测；启动失败为残余风险） |
| WA-8, INV-3 | `test_driver_*`、`test_dcu*`、`test_dlt698*`、`test_iems_full_stack`、`test_iems_modbus_tcp_loopback` | 全绿，引擎路径无断言回归 |
| WA-9, INV-3, INV-5, OUT-1, OUT-4 | `test_io_http_callback_destroy_keeps_pending_ref_count` | 回调内 destroy 后 `free_count == 4`；ASAN 无 UAF/二次 unref |
| WA-10, WA-6, INV-1, OUT-1 | `test_io_http_write_after_enable_read_keeps_ref` | `write_async` 返回 0；在途 `free_count == 3`；destroy 后 `free_count == 4` |

## 7. 独立预期（供 verifier）

verifier 可独立推导的期望值：在 `test_passthrough_write_async_delayed_completion_ownership`
中，若契约 A 成立，则“调用方 unref + 驱动自持”的在途态池空闲数 = `base - 1`；
旧实现（调用方不 unref）为 `base - 2`，旧断言因此必然红。该差值不依赖具体池大小。

另一条不依赖池大小的推导：WA-9 要求回调 destroy 只释放一次引用；旧顺序在回调后还会再读 `impl->pending_tx`，要么二次 unref、要么丢失本地引用，完成态空闲数不可能稳定等于 base（4）。
