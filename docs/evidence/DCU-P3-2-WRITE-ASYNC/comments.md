# 注释清单（DCU-P3-2-WRITE-ASYNC）

- `modules/edge_io/include/io_executor.h` `io_ops_t.write_async`：新增契约文档块。
  这是任务要求写入头文件的 API 契约（调用方 / 驱动 / 完成路径的引用归属），
  不是解释实现细节的代码注释。其余 `.c` 实现与测试未新增注释。
- 未删除任何既有 why 注释；`slave_engine.c` 既有“write_async 自己持有引用”的
  注释与新契约一致，保留原样。
- ASAN 复核修订新增一条测试注释：`tests/test_passthrough.c` 的共享 orchestrator
  夹具上方说明“全局单例存储，每用例各建一份会覆盖前一份造成泄漏”，解释夹具为何
  只 create 一次。
- HTTP 所有权修订新增两处 why 注释：`io_executor_linux.c` 的 `_on_curl_poll`
  （回调可能 destroy 句柄，先摘句柄/取走 pending）与 `_http_write_async`
  （`CURLM_ADDED_ALREADY` 视为成功）；
  `tests/test_io_executor.c` 新增一条用例说明（enable_read 已把句柄加入 multi）。
