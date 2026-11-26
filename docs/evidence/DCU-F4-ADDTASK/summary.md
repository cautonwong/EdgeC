# DCU-F4-ADDTASK：add_task_ex 只影响新任务

分支 `dcu/F4-addtask-targeted`，工作树 `/tmp/edgec-land/f4-addtask-targeted`，基线
`feat/dcu` `a3421169`（F5R 已合入）。

## 背景与目标

F4 复审发现 `dcu_poller_add_task_ex` 在新增任务时调用 `_poller_reopen_active`，把全部
ACTIVE 任务重开：exec_gen 递增、visited/settled/succeeded 清零、链上 pending 丢弃。
新增任务只是追加一个槽位，旧任务的执行身份与进度不该被重置。这是形状不一致的工程债，
不是数据完整性缺陷；目标是只把变化收敛到新任务。

## 根因

`bd2b6289` 把“任务/方案/档案变化”统一建模成全 poller 执行失效，`add_task_ex` 也调了
`_poller_reopen_active`。但新增任务不改变既有任务的身份（slot 下标不变、target 快照
不变、scheme/revision 不变），全量重开会把旧任务已装载、已 served 的标签作废并重抄。

## 修复（最小形状）

删除 `dcu_poller_add_task_ex` 末尾的 `_poller_reopen_active(poller, poller->inv)`。
`dcu_poller.h` 的 `add_task_ex` 文档补一行行为契约：新增任务不改动已 ACTIVE 任务的执行；
新任务按自己的 begin/due 开始。

修复后：

- 既有 ACTIVE 任务保留 exec_gen、target 快照、visited/settled/succeeded、settled_left、
  success 计数与链上在途批次；回调按原 exec_gen 正常结算。
- 新任务 monitor 全零（IDLE），在第一个到期步由既有 `_task_begin` 建立自己的执行
  （新 exec_gen、target 快照），随后由链按需装载。
- 链上 `served_ident`/`exec_gen` 围栏仍按任务身份清理 served 位与旧失败；新执行第一次
  装载时身份不同，自动清掉该槽位的陈旧 served 位。
- 档案 revision 变化仍 `_poller_reopen_active`（step 内），方案内容变化仍只重开引用它的
  任务，upsert 仍只重开被改任务（F4/F4R 语义不变）。

## 拒绝的替代

1. **保留全量重开**：新增任务时重置无关任务、丢弃在途批次，就是本单要修的缺陷。
2. **只 `_task_reopen` 新槽位**：新任务 monitor 为 IDLE，`_task_reopen` 会直接返回
   （只处理 ACTIVE）；它既不会也不能清链上状态。调用它是空操作，掩盖意图。
3. **递增 exec_seq 或清链上 pending 来“帮助”新任务**：不必要。新执行在第一次
   `_chain_load_pending` 时按 `_exec_ident_same` 比较，身份不同会自动清 served 位；
   主动清 pending 反而丢弃其它任务的在途读。

## 可观察行为（新测试）

一个任务 A 已 ACTIVE、16 项批次已装载（1 项在途），此时 `add_task_ex` 新增 B：

| 观察点 | 修复后 |
| --- | --- |
| A 的 exec_gen | 不变 |
| A 的 visited / settled_left | 保留（bit0 已 visited，settled_left=1） |
| 链上 pending_n | 保留 16（不丢弃在途批次） |
| B 的 monitor.status | IDLE，等自己的 due 步 |
| B 首次到期 | 自己 `_task_begin`，exec_gen > A 的 |
| A 的 16 个标签 | 各提交恰好 1 次（不重抄） |
| B 的 16 个标签 | 各提交恰好 1 次 |
| 结束状态 | A、B 各 success_meters=1、DONE |

旧码在第一个断点即红：A 的 exec_gen 从 1 变 2（`2 != 1`）。

## 红先绿后

- 测试提交 `acafecf4`：新增 `test_add_task_does_not_reopen_active_execution`；在
  a3421169（旧码）上 36 例中 1 例红（`tests.log`）。
- 修复提交 `93877f9e`：删一行调用 + 一行头文件契约；`test_dcu_poller` 36/36 绿，
  `ctest -R 'dcu|dlt698'` 45/45 绿。

## 验证命令与结果

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DPython3_EXECUTABLE=/home/edge/EdgeC/.venv/bin/python3
cmake --build build --clean-first -j4
ctest --test-dir build -R 'dcu|dlt698' --output-on-failure
python3 ci/scripts/check_build_warnings.py --log <clean-build.log> --root .
python3 ci/scripts/check_build_freshness.py --root . --build build
python3 ci/scripts/check_dcu_698_catalog.py
python3 ci/scripts/check_doc_references.py --root .
```

结果：clean build 0 第一方 warning（仅 third-party open62541 一条）；45/45 绿；
四个守卫 0；`test_dcu_poller` 36/36（原 35 + 新 1）。详见 `runtime.log`。

## 未决项

- 无功能未决；本单只收敛重开范围，不改方案/target/结算语义。
- 若未来有生产者依赖“新增任务触发所有任务重新快照”，需要显式调用档案 revision 或
  既有 upsert 路径，而不是靠 add_task_ex 的副作用；当前仓库无此依赖。
- 交接文档/计划状态由 verifier 更新。
