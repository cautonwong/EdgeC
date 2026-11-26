# DCU-F5-HEARTBEAT 注释自审

范围：测试提交 `766d7643` 与修复提交 `788a9ae4` 的新增注释（未跑 comment-sicko；
本单任务未要求，按仓库注释纪律人工自评）。

## 保留

- `cmd/dcu/dcu_heartbeat.c` 的 `// pipe 消息的 device_id 是稳定 slot_id；档案下标删除后会变，不能当身份。`：
  解释为什么不能用循环变量 `i`。没有这行，下一位读者会自然地把触发“简化”回下标，
  这正是本次缺陷的根因。
- `tests/test_dcu_runtime_health.c` 的用例头注释（删除 slot 0 后末元素 slot 2 顶到下标 0）：
  记录 `dcu_inventory_remove` 的 swap-and-pop 语义与场景构造依据；断言只能看出结果，
  看不出为什么这样造档案。
- `tests/test_dcu_runtime_health.c` 的 `/* demo 分支只在“本轮有落库”时进冻结管线。 */`：
  心跳的 `saved > 0` 前置条件不是断言能表达的，不写会让人误以为任意 tick 都会推冻结。
- `tests/test_dcu_runtime_health.c` 的 `/* 管线复制 stage 字段，工厂对象只是临时描述符。 */`：
  沿用 F5R 的 ASAN 泄漏修复说明，解释为什么注册后可以 `free` 工厂对象。
- `tests/test_dcu_runtime_health.c` 的 `/* 每只表 3 个冻结源。按下标发触发时 slot 2 一条没有。 */`：
  交代 3 的来源（日冻结的三个源 OAD）与红绿判据。

## 删除

0 条。没有复述下文的注释；未改既有注释。

## 行为核对

红：`test_dcu_runtime_health` 4 例中 1 例失败（slot 2 表 `0 != 3`），其余 3 例绿。
绿：修复后 4/4。mutation guard `DCU-F5-heartbeat-slot` 变异后 test rc=8、恢复后 rc=0。
