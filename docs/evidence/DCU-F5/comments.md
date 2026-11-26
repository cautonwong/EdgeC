# DCU F5 注释审查

审查者：`pstack:comment-sicko`（模型输出，建议性）。范围：修复 diff 的注释；
测试 commit `a695fd09` 仅抽查。未写文件。

## 采纳

| 位置 | 发现 | 处理 |
| --- | --- | --- |
| `dcu_node.c` `_node_publish_realtime` | 注释只是复述下标→slot 的代码；且该路由端口在树内无调用者 | 删除该行注释 |
| `dcu_poller.h` sink 端口 | 注释重复了 `dcu_inventory_remove` 的覆盖机制 | 只保留契约：device_id 必须是稳定 slot_id |
| `dcu_poller.h` `dcu_read_ctx_t.address` | 写成“落库/熔断归属”不准确：熔断/统计用 `visit_address` | 改为“落库归属” |
| `dcu_poller.h` 链内 `read` | “depth=1 时同时只有一个”像是在承认前提而不约束 | 改为“在途读的身份快照：提交时覆盖，回调只认它” |
| `dcu_poller.c` 链初始化注释 | 旧注释邀请改 `depth` 放开并发；单份 `read` 快照不支持并发，属于本改动造成的注释失真 | 改为“并发固定 1；放开 depth 前必须先把快照扩成按并发度分槽” |

保留的 why 注释：`bus_ctx_t` 快照新增字段、备用 DI 重试的身份校验、scale 用提交时
protocol、poller 发布适配不再反查、重开后迟到回调仍落库但不结算、装载时冻结身份、
`_on_read_done` 的“只认快照”不变量、sink/publish 端口的稳定身份契约。

## 拒绝（超出 F5 范围）

- `dcu_inventory_remove` 的“末元素覆盖”压缩：既有设计，且已有 slot_id 作为稳定身份；
  本单不改成 tombstone/稳定数组。
- `dcu_chain_t.depth` 死旋钮：本单只修注释，避免顺手重构；真正放开并发需要把
  `read` 扩成按并发度分槽的数组。
- `router_bus_ops_t.publish_realtime` 无树内调用者：属于另一个端口的既有装配；
  删除会扩大跨端口影响面，留给后续 finding。

## 自评

- 新增/修改注释均描述代码无法表达的 why（身份为何必须冻结、为何不能取消/丢弃、
  为何重试前要校验身份）。
- 没有用注释为 workaround 辩护：取消在途、丢弃迟到结果、按地址反查等替代都被拒绝，
  修的是身份来源本身。
- 未提交 `depth` 字段的删除或并发快照重设计；已用注释明确该限制的升级条件。

## 第二轮（`8b2b827c` / `f26118c6`）

范围：本轮 diff 的注释与结构删除；`/tmp/interrogate-f5/` 探针只读，未改。

### 采纳

| 位置 | 发现 | 处理 |
| --- | --- | --- |
| `dcu_poller.h` `dcu_read_ctx_t` | 原文用“提交时冻结”概括全部，实际 address/device_id 装载时冻结、tag/mask/exec 每项提交时写入 | 按冻结时机拆成字段级注释 |
| `dcu_poller.h` 链 `read` 字段 | 上一轮改成“提交时覆盖”同样以偏概全 | 改为 address/slot 装载时冻结，tag/mask/exec 每项提交时写入 |
| `dcu_poller.c` 链初始化 | “并发固定 1”仍留下 `depth` 这个可变旋钮 | 删除字段与 init 赋值，pump 固定最多 1；注释改为“在途读固定最多 1” |
| `bus_ctx_t` | `slot_id`/`address`/`channel_id` 只为旧守卫存在；revision 判定后成死字段 | 删除三个字段及赋值；只保留 revision/protocol 等仍在用的快照 |

### 结构删除（不用注释解释非法状态）

- `dcu_chain_t.depth` 删除：单份 `read` 快照 + 多在途读的非法组合不再可表达；
  pump 的 `outstanding >= 1` 是唯一约束点。
- `bus_ctx_t` 的三字段删除：身份判定只认 revision，不需要第二套“slot==下标”的
  等价检查；旧检查的注释随之删除，没有留下解释性注释为它辩护。

### 保留的 why 注释

pump 入口的 revision 栅栏（为什么迟到回调不能继续本轮）、重试守卫的“一次读与
重试同一 revision”、scale 用提交时 protocol、迟到回调仍落库但不结算新执行。

### 自评

- 本轮没有新增注释去解释被删字段；删除的是状态本身。
- 正例 `test_spare_retry_allowed_when_revision_unchanged` 证明栅栏没有把重试关掉。
