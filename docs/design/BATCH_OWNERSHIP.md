# BATCH 负载所有权

状态：决定记录（2026-11-15）。只解决 `pipe_msg_t` 的 `PIPEMSG_TYPE_BATCH`，不改标量、事件和 ref 负载。

## 1. 问题

`pipe_msg_t` 的 batch 分支只有 `void *ptr` 和 `int count`，没有容量、释放回调或生命周期标记。

当前 `stage_compactor` 每凑满一批就 `malloc` 一个 `out_buf`，复制数据后把指针放进消息，再调用 `stream_engine_push`。管道同步执行，但没有代码释放这个缓冲。下游如果返回 `STAGE_ASYNC`，引擎会把整条消息存进 `pipeline->current_event`，指针随管道挂起，提前释放会 UAF。

同时，`stream_bus_publish` 会把 64 字节消息写进 DataHub 的共享内存。如果消息是 BATCH，SHM 里留下的是生产者进程的堆地址，其他进程解引用必然无效。

## 2. 规则

1. **BATCH 是进程内、单次 push 内有效的负载。** 生产者在调用 `stream_engine_push` 前准备好缓冲，`stream_engine_push` 返回后可以复用或释放。
2. **管道阶段不得把 BATCH 指针保存到 push 之外。** 需要延后处理时必须自己复制。
3. **异步阶段不接收 BATCH。** 引擎在 `STAGE_ASYNC` 且消息类型为 BATCH 时拒绝挂起，记录错误并丢弃该消息。当前 `sqlite_sink` 和 `js_stage` 的异步路径都只处理 SCALAR，这条规则不改变现有行为。
4. **BATCH 不进入 `stream_bus_publish`。** 总线端口只接受标量、事件和明确共享内存化的 ref。BATCH 需要跨进程时，先转换成共享内存或序列化，不能直接传指针。
5. **容量由生产者保证。** batch 消息没有容量字段，生产者在写入 `count` 个 double 前必须确认缓冲足够。消费者只信任 `count`。

## 3. 对 compactor 的修改

`stage_compactor` 在创建时按 `max_elements` 分配一次 `out_buf`，每批直接写入并复用，不再每批 `malloc`。因为 BATCH 只在同步 push 内有效，复用不会覆盖下游还在读的数据；异步下游按规则 3 被拒绝。

`max_elements` 仍然来自 stage 参数，创建时校验上界，避免一次分配过大。

## 4. 拒绝而不是克隆

另一种做法是引擎在 `STAGE_ASYNC` 时深拷贝 BATCH，再在 resume 完成后释放。当前没有异步阶段消费 BATCH，这个克隆是 speculative 的，而且会把所有权藏进引擎。

先拒绝，等出现真实的异步 BATCH 消费者，再按那时的需求设计克隆或共享缓冲。拒绝路径有日志和测试，不会静默丢数据。

## 5. 验收

- compactor 连续产生两批，捕获阶段看到两批数据都正确，且 compactor 没有每批分配。
- 一个故意对 BATCH 返回 `STAGE_ASYNC` 的测试 stage 被引擎拒绝，管道不挂起，后续消息仍能处理。
- `stream_bus_publish` 对 BATCH 返回错误，不写 SHM。
- 全量测试和 ASAN 通过。

## 6. 未决

- 跨进程 BATCH 传输需要共享内存 batch 区或序列化格式，当前不做。
- batch 消息没有容量字段，未来如果要让消费者校验，需要在 `pipe_msg_t` 里加字段或改成带 header 的共享缓冲。这会改 64 字节 ABI，单独评审。
