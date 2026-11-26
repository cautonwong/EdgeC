# DCU-F4-ADDTASK 注释自审

范围：测试提交 `acafecf4` 与修复提交 `93877f9e` 的新增注释（未跑 comment-sicko；
本单任务未要求，按仓库注释纪律人工自评）。

## 保留

- `tests/test_dcu_poller.c` 的测试头注释（`F4：add_task_ex 只增加一个槽位...`）：
  记录旧实现的具体副作用（exec_gen 递增、visited 清零、pending 丢弃）与“新任务没有
  ACTIVE 执行可重开”的选择依据；断言本身只展示结果，看不出为什么这样建场景。
- `tests/test_dcu_poller.c` 的 `/* 非零周期：... */`：解释为何不用 interval 0——零周期
  会在完成后立刻重发，重复计数就分不清是重开还是正常周期，测试会失去区分力。
- `cmd/dcu/dcu_poller.h` 的 `新增任务不改动已 ACTIVE 任务的执行；...`：行为契约，
  旧缺陷正是调用方无法从签名看出这个副作用。

## 删除

0 条。没有复述下文的注释；对象/字段含义由既有头文件注释承载，未重复。

## 行为核对

修复仅删一行调用并加一行文档；`test_dcu_poller` 36/36、`ctest -R 'dcu|dlt698'`
45/45 通过。
