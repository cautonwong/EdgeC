# D0-13 Comment Sicko 结果

- 删除 1 句部署说教（头文件“生产与 keep-alive 默认永不自退”）：保留的契约句
  “0 = 不启动自退定时器 / 未设…返回 0”已由测试 `test_unset_is_zero` 钉住。
- 保留：模块/API 契约注释、include guard 标签（仓库 22+ 个头文件同款约定）。
- 接受 MUST KILL：定时器/回调/日志从 safety 改名为 self-exit
  （`on_self_exit_timeout`、`self_exit_timer`、`[SELF_EXIT]`），避免把选择加入的自退
  伪装成无条件安全网。
- 无其他注释改动；diff 内无 suppressions、无注释掉的代码。

## F4 追加

- `dcu_heartbeat_stop` 的头文件注释保留：它表达“拆除阶段必须在销毁 storage 前调用”这一
  代码外约束（心跳 tick 会 flush storage），单测与运行时都依赖该顺序。
- `dcu_product.py` 处理器注释保留：解释“不能在信号处理器里 wait（waitpid 锁同线程重入）”，
  这是实测踩过的非显然 why。
