# DCU-F5-HEARTBEAT：演示冻结与跳闸触发带稳定 slot_id

分支 `dcu/F5-heartbeat-slot`，工作树 `/tmp/edgec-land/heartbeat-slot`，基线 `feat/dcu`
`77dd8e1e`。**未合并。**

## 问题与根因

`DCU_DEMO_MODE=1` 时 `dcu_heartbeat_tick` 遍历档案，把循环下标 `i` 写进
`pipe_msg_t.device_id`。F5R 后冻结管线在 `_freeze_proc` 用
`dcu_inventory_by_slot(ctx->inv, ev->device_id)` 解析地址与影子抓拍，`device_id`
的契约是稳定 `slot_id`，不是可变的档案下标。`dcu_inventory_remove` 用末元素覆盖
删除位：删一只表后，末元素的 slot 不变而它所在的下标变了，下标与 slot 从此分叉。
此时演示触发会把冻结断面挂到错表，或按不存在的 slot 直接丢弃。

同一循环的演示跳闸也把 `i` 传给 `dcu_698_control_legacy_action(uint32_t slot, ...)`，
该函数同样按 slot 经 `dcu_inventory_by_slot` 解析；同一身份证错位，一并修正。

## 数据形状

- `slot_id` 是表在影子表/存储里的稳定身份，随表整体搬移；`dcu_inventory_by_slot`
  是唯一反查入口。
- 档案下标只在同一 revision 的同步操作内有效，不能跨模块或跨消息传递。
- `pipe_msg_t.device_id` 与 `dcu_698_control_legacy_action` 的 slot 参数使用同一身份；
  不再从循环下标派生。

拒绝的替代：给 `pipe_msg_t` 增加第二个 slot 字段、在冻结侧再兜底按下标反查一次、
把 demo 触发改成按地址字符串寻址。前者重复真相，中者把歧义留在边界内，后者扩大
改动面；均不采用。

## 改动

- `cmd/dcu/dcu_heartbeat.c`：demo 循环取 `uint32_t slot = inv->meters[i].slot_id`；
  冻结触发、`[RELAY]` 日志与演示跳闸共用该 slot。
- `tests/test_dcu_runtime_health.c`：新增 `test_demo_freeze_trigger_uses_slot_identity`。
  删除 slot 0 后 slot 2 的表顶到下标 0；经真实冻结管线与 SQLite 断面断言两只表
  各 3 个源冻结，旧码下 slot 2 的表为 0 条。
- `ci/mutation_guards.tsv`：登记 `DCU-F5-heartbeat-slot`，把修复行变异回下标后
  新用例必须变红。
- `docs/plans/2026-11-21-dcu-698-production-plan.md` §12.5：该缺口行改为已收口。

无动态内存、无新抽象。

## 红先绿后

- 红（测试提交 `e242de07`，旧码）：`test_dcu_runtime_health` 4 例中 1 例失败，
  `0 != 3`，其余 3 例绿。
- 绿（修复提交 `b23b79c6`）：4/4。
- mutation guard（最终提交上重跑）：变异后 test rc=8，恢复后 rc=0，源文件 hash 还原。

## 验证命令与结果

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_WEB_BUILD=OFF \
      -DPython3_EXECUTABLE=/home/edge/EdgeC/.venv/bin/python3
cmake --build build --clean-first -j4            # rc=0, 1267/1267
python3 ci/scripts/check_build_warnings.py --log clean-build.log --root .
python3 ci/scripts/check_build_freshness.py --root . --build build
python3 ci/scripts/check_doc_references.py --root .
ctest --test-dir build -R 'dcu|dlt698' --output-on-failure
```

结果：

- clean build 0 第一方 warning（仅 third-party open62541 一条 unused-parameter）。
- warning guard：`No new violations. 0 tracked debt item(s) remain pinned.`
- freshness：`build is current for /tmp/edgec-land/heartbeat-slot`。
- docrefs：exit 0。
- `ctest -R 'dcu|dlt698'`：45/45 通过，67.26 秒。
- 本构建 `ENABLE_WEB_BUILD=OFF`：本单只改 C 源与测试，未触碰前端；C 编译告警
  集合与 CI 同源（CI 等价 flags 见 clean-build.log）。

## 未决

- `dcu_heartbeat_control_tick` 的演示遥控仍固定 `dcu_698_control_legacy_action(0, 1)`，
  目标就是种子档案第一只表 "Meter 10000001"；档案删除后 slot 0 可能不存在，该演示
  动作会按 NOT_FOUND 拒绝并打 WARN。demo-only，与本单冻结触发缺陷无关，未改。
- 本机未跑 ASAN/TSAN；该测试在 CI 的 sanitizer matrix 内会覆盖（无新增动态分配）。
