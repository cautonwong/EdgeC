# DCU-D0-20：注释与命名

本轮为状态机重写，新增/改写的注释集中在：
- `dcu_poller.h`：执行状态枚举、target/visited/settled/succeeded 位图语义、
  success<=total 的结构性保证、熔断状态枚举与冷却常量。
- `dcu_poller.c`：单一资格谓词、快照、结算、完成锚定、熔断迁移函数。
- `tests/test_dcu_poller.c`：每个迁移测试的意图一行。

人工复核：注释只写代码无法表达的“为什么”（生产理由、归属代际、快照冻结），
不逐行复述。未跑独立 Comment Sicko（状态机改动大，先以单测反向验证与 verifier
复核为主）；如需要可另跑一轮。

## F6 修复（edge_io 载荷所有权）

新增注释只写代码无法表达的所有权约定与后果：
- `io_executor.h`：`IO_EVENT_RX` payload 是借用、io 持有到回调返回后释放，
  跨回调使用必须自行 ref；写明"回调内释放会让回调栈里新申请的缓冲被收尾 unref
  误释放（TX 帧内容被覆盖）"这一真实事故后果。
- `master_engine.c` / `slave_engine.c`：在回调入口标注"只读、不 unref"及其原因。
- `passthrough_mgr.c`：说明异步 uv_write 前必须自持引用。
- `tests/test_master_link_lifecycle.c`：回归用例说明哨兵法与它在生产里的对应关系。
未删除既有 why 注释；替换掉的 master 回调旧代码旁新增了更明确的 why 注释。
