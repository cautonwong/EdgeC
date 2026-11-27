# DCU-P3-2-WRITE-ASYNC 证据包

统一 `write_async` 的 net_buf 引用所有权契约。

- 基线：`feat/dcu` = `9d065b6d`（期间 feat/dcu 未前进，未 rebase）
- 分支：`dcu/P3-2-write-async`
- worktree：`/tmp/edgec-land/p3-2-write-async`
- 契约头：`modules/edge_io/include/io_executor.h`（不在 `contracts/schema/core_readonly_whitelist.txt` 保护范围内）
- spec：`docs/evidence/DCU-P3-2-WRITE-ASYNC/spec.md`（条目 WA-1..WA-8 / INV-1..INV-4 / AC-1..AC-6）
- 红测试提交：`2d6f9034`（只含 `tests/test_passthrough.c`、`tests/test_io_executor.c`）
- 实现提交：`f1f5a36e`（契约头 + linux_platform 驱动 + passthrough + 测试 mock 对齐）
- 证据提交：见本文件所在提交

## 选定契约

**A. 驱动成功后接管一份引用；调用方只在调用返回后释放自己那份。**

- 调用方 alloc（ref=1）→ `write_async` → 返回后无论 rc 立即 unref。
- 返回 0：需要跨调用用帧内容的实现（poll/UDP/HTTP）必须 `net_buf_ref` 自持，
  在完成/失败事件或 destroy 时释放；同步消费的实现（MQTT 由 Paho 拷贝、
  OPC UA 桩忽略内容并同步完成）不持。
- 返回 <0：同步错误路径不接管任何引用。
- 完成事件在 loop 线程投递，payload 为 NULL；回调不访问缓冲。

选择理由：TX_DONE payload 为 NULL 且可多帧在途，契约 B（调用方保留引用到完成）
无法把缓冲与事件配对；A 已是 poll 驱动与 slave/master 引擎的既有事实，改动面最小。
完整契约表与风险见 `design.md`。

## 现状缺陷（旧实现）

- `passthrough_mgr.c`：alloc 后从不 unref → 每帧泄漏一个缓冲。
- UDP/HTTP 驱动：不持引用，而调用方按 poll 的既有写法立即 unref → 异步发送/
  传输期间 use-after-free。
- poll 驱动：`uv_poll_start` 失败时已 ref 的 `pending_tx` 未归还 → 泄漏且
  下一次写卡在 `-EBUSY`。
- slave/master 引擎：已按 A 释放自己那份引用，无需修改（见 `design.md` 审计）。

## 修复

- `io_executor.h`：写入 A 契约（头文件文档块）。
- `io_executor_linux.c`：poll 启动失败归还 `pending_tx`；UDP 在请求包装中持有
  引用并在 send 回调释放；HTTP 持有引用到 `CURLMSG_DONE`/destroy，在途再写
  返回 `-EBUSY`。
- `passthrough_mgr.c`：调用返回后 unref 自己的引用。
- `test_passthrough.c` / `test_iems_full_stack.c`：删除“驱动替调用方 unref”的
  旧 mock 写法，改为同步消费不持引用。

## 红/绿结果

红（旧实现，`red.log`）：

```
$ ctest --test-dir build -R '^(test_io_executor|test_passthrough)$' --output-on-failure
test_passthrough: FAILED test_passthrough_write_async_delayed_completion_ownership (test_passthrough.c:279)
test_passthrough: FAILED test_passthrough_write_async_sync_failure_ownership (test_passthrough.c:272)
test_io_executor: FAILED test_io_udp_write_async_holds_ref_until_send_done (test_io_executor.c:465)
test_io_executor: FAILED test_io_http_write_async_holds_ref_until_complete (test_io_executor.c:532)
2 tests failed out of 2 (ctest exit=8)
```

绿（新实现，`tests.log`）：

| 套件 | 结果 |
| --- | --- |
| `test_io_executor` + `test_passthrough` | 2/2 通过 |
| `ctest -R '^(test_driver_|test_io|test_edge_io|test_slave)'` | 13/13 通过 |
| `ctest -R 'dcu|dlt698'` | 45/45 通过 |
| `test_iems_full_stack` + `test_iems_modbus_tcp_loopback`（mock 对齐） | 2/2 通过 |

## ASAN

`build-asan`（`-DENABLE_ASAN=ON`，RelWithDebInfo）构建 `test_io_executor`、
`test_passthrough` 后直接运行（绕过 ctest 的 `detect_leaks=0` 属性）：

```
ASAN_OPTIONS=detect_leaks=1 build-asan/tests/test_io_executor     -> 15/15 PASSED, exit 0
ASAN_OPTIONS=detect_leaks=1 build-asan/tests/test_passthrough    -> 3/3 PASSED, exit 0
```

- 无 heap-use-after-free、无 double-free。net_buf 日志显示在途 `Ref 1->2`、
  调用方 `Unref 2->1`、完成/销毁 `Unref 1->0` 的契约序列（UDP/HTTP/passthrough 同）。
- **复核修订（verifier 退回后）**：HTTP 用例结尾补 `uv_loop_close`（原 704 B/2 allocs
  的 loop 泄漏）；passthrough 三个用例改为共享一份 orchestrator 夹具——
  `bus_orchestrator_*` 使用全局单例存储，每个用例各 create 一次会覆盖前一份，
  旧 engine/session manager/池随即不可达（原报 43200 B/9 allocs）。共享夹具后
  `detect_leaks=1` 下两个二进制均 exit 0、无 LeakSanitizer 报告；未改产品逻辑。
- 三个新用例单独 `CMOCKA_TEST_FILTER` 运行同样 exit 0、无报告（见 `asan.log`）。

## 验收

```
$ flock /tmp/edgec-land/acceptance.lock env TESTPLATFORM=/home/edge/test-platform \
    PYTHON=/home/edge/EdgeC/.venv/bin/python3 EDGEC_BUILD_DIR="$PWD/build" \
    ./acceptance/run.sh dcu
任务 dcu-acceptance 结束,退出码 0（3 passed）
任务 dcu-northbound 结束,退出码 0（35 passed）
全部合格:2 个验收任务
acceptance exit=0
```

## 变异守卫

- 名称：`DCU-P3-2-write-async-ownership`（`ci/mutation_guards.tsv`）
- 变异：去掉 `passthrough_mgr.c` 中 write_async 后的 `net_buf_unref(nb)`。
- 结果：见 `mutation-guard.log`（mutated 红、restored 绿）。

## 对抗评审退回修订（HTTP 所有权两处）

Finding 1（高）：`_on_curl_poll` 原先先回调后 unref `impl->pending_tx`；公开 API
`io_executor_http_request` 的回调会在 TX_DONE/ERROR 内 `destroy` 句柄，`_http_destroy`
先 unref 并释放 impl，poll 随后读取已释放内存（ASAN heap-use-after-free）并可能
二次 unref，完成态 `free_count` 为 3（契约应 4）。修复：回调前取走 `pending_tx`
并置 NULL、先 `curl_multi_remove_handle`；回调后只 unref 本地保存的引用，不再碰
impl/easy。测试 `test_io_http_callback_destroy_keeps_pending_ref_count`：旧码
`3 != 4`（`red-http-ownership.log`），新码绿；ASAN 无 UAF。

Finding 2（中）：`_http_write_async` 在 `CURLM_ADDED_ALREADY` 时释放刚 ref 的
pending 并返回正数 7；调用方按契约 unref，而在途传输仍使用该缓冲。修复：该码视为
成功（保留引用、返回 0，完成回调释放）；其他错误返回负值、清 POSTFIELDS。测试
`test_io_http_write_after_enable_read_keeps_ref`：旧码 `7 != 0`
（`red-http-ownership.log`），新码 `free_count` 3→4。

- 变异守卫 `DCU-P3-2-http-poll-after-cb`、`DCU-P3-2-http-added-already`：
  mutated rc=8 红 / restored rc=0 绿，源文件 hash 校验（`mutation-http-ownership.log`）。
- 回归：`test_io_executor` 17/17；`ctest -R 'dcu|dlt698'` 45/45（175 s）；
  ASAN `detect_leaks=1` 全二进制 17/17 exit 0 无报告（`asan-http-ownership.log`）；
  acceptance 1 轮 exit 0、全部合格 2 个验收任务（`acceptance-dcu-http.log`）。

## 残余风险

- `uv_poll_start` 失败的泄漏修复没有构造性测试（需构造非法句柄状态），由代码
  审查与既有 `test_io_write_hard_error_releases_pending_tx` 兜底。
- MQTT/OPC UA 的“同步消费不持引用”按实现与第三方库语义审查确认，未跑 broker。
- 文件边界说明：真实 `write_async` 实现在 `adapters/linux_platform/src/io_executor_linux.c`
  （任务书笔误写作 adapters/io_drivers），属“适配器实现对齐”同一类，已逐处说明。
