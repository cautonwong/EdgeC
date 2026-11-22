# DCU-D6-08a 注释与决定自审

本单没有单独的 comment-sicko 运行；改动只加了三处说明与一处抑制文件注释，逐条自审。

- `cmd/dcu/dcu_storage.c` 的 init 注释：写明 storage 的单线程所有权，并解释 TSAN 的 lock-order 报告来自 SQLite 内部 open/close 的相反顺序。这是本次决定的"为什么"，代码本身无法表达。保留。
- `ci/tsan.supp` 的两行注释：说明抑制只覆盖 SQLite 的 `pthreadMutexEnter`，以及为什么不是可达死锁。抑制文件必须有理由，否则下一个人会把它当万能阀门。保留。
- `tests/test_dcu_runtime_health.c` 新用例上方的两行注释：说明该用例证明的是 create/write/flush/destroy 同线程，而不是普通功能。保留。
- `tests/CMakeLists.txt` 四处 TSAN_OPTIONS 只追加 `suppressions=...`，无新增注释。

没有删除任何既有注释；没有把第一方锁（uv_mutex）纳入抑制范围。

## 决定复核

- 两个环的栈帧都是 SQLite 的 unix mutex 包装；uv 的锁不走该函数，所以抑制不会掩盖第一方锁序问题。
- 单线程所有权有代码依据：`cmd/dcu` 无 `pthread_create`/`uv_thread_create`；新增用例在四个点比较 `pthread_self()`。
- 未改 SQLite 编译选项（预编译库在 `.local`，不在本仓库构建范围）。
