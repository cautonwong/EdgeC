# DCU-FIX-578-2：6000 方法 129/130 落库失败仍改 RAM

## 结论

129/130 先改 RAM 再 `_save_unit`，失败直接返回：ACTION 回 DAR 2，但主站随后 GET 6001 A2 读到新地址/新扩展参数。修复后保存前快照 `production_meter_t` 与 `revision`，保存失败恢复快照，GET 仍服务旧档案；129 的旧地址键仍在、新地址不在，130 的 collector/asset/PT/CT 恢复。成功路径不变。

## 根因

- `cmd/dcu/dcu_698_class_acq.c` 方法 129：`dcu_inventory_update_basic` 已写入 RAM，`_save_unit` 失败 `return save`，无回滚。
- 同文件方法 130：`dcu_inventory_update_extended` 已写入 RAM，`_save_unit` 失败同样无回滚。
- 对照：Add 路径 `_add_unit` 落库失败会 `dcu_inventory_remove_seq`（落库失败不留 RAM 半态）；证据文档 `docs/evidence/DCU-D2-02/summary.md` 声称"失败不改 RAM"。

## 修复

- `dcu_inventory.h/.c`：新增 `dcu_inventory_restore_unit(inv, snapshot, revision)`，按 `config_seq` 回填整条档案并回退 `revision`。
- `dcu_698_class_acq.c` 129/130：更新前快照旧单元与 `inv->revision`；`_save_unit` 失败时恢复快照并返回 `EDGE_SLAVE_SRC_TEMPORARY`（驱动映射 DAR 2）。129 成功路径仍用快照里的旧地址决定是否删旧主键。
- 文档同步：`docs/evidence/DCU-D2-02/summary.md` 的写入顺序改为"先按新值改 RAM 再落库；失败恢复快照（含 revision 与地址主键）"。

## 完成定义

- 静态分析：`check_build_warnings.py` 通过（tests.log 末段，0 violations）。
- 测试：`tests/test_dcu_698_archive.c`
  - `test_update_basic_save_failure_rolls_back`：失败 sink 下 129 回 `EDGE_SLAVE_SRC_TEMPORARY`；旧地址 `21000081` 仍在、新地址 `21000082` 不在；`baud_code`/`channel_id`/`revision` 回旧值；6001 A2 编码仍含旧地址；SQLite 重载仍是旧地址。
  - `test_update_extended_save_failure_rolls_back`：130 失败后 collector/asset/PT/CT 与 `revision` 回旧值，6001 A2 不含新值；随后成功 130 仍按新值生效。
  - 修复前：两条均失败（`(nil) == (nil)`、`"COLL-1" != ""`），exit=2。
  - 修复后：`test_dcu_698_archive` 9/9；ASAN 通过；`test_dcu_inventory`、`test_dcu_698_object_model`、`test_dcu_698_var`、`test_dcu_node`、`test_dcu_router`、`test_dcu_poller`、`test_dcu_698_tasks`、`test_dcu_698_schemes` 等 9 个 DCU 目标 9/9。
- 运行时：`repro_archive_rollback`、`repro_archive_action_onwire`（adv-3）
  - 前：`after: seq=1 addr=000000000002 baud_code=5` / `RAM CHANGED DESPITE FAILED PERSIST`；线上动作 `FAILED ACTION STILL MUTATED RAM`。
  - 后：`after: seq=1 addr=000000000001 baud_code=3` / `RAM unchanged`；6001 A2 回旧地址；线上动作 `no divergence`。
- 证据文件：`tests.log`、`runtime.log`、`diffstat.txt`、`comments.md`。

## 未决项

- Add 路径 127/128 在"落库成功但 `archive_delete` 失败"时的极端组合不在本 finding 范围。
- `dcu_inventory_restore_unit` 找不到 `config_seq` 时静默 no-op；单线程循环内紧邻更新后不可达，未加断言（见 `comments.md`）。

## 提交

- `c37f9dd2` test: 失败复现（先失败）
- `c424394b` fix: 快照回滚
- `eda86a3b` docs: 同步 DCU-D2-02 写入顺序
- `88ea2561` style: /no-comments 接受的注释精简

分支 `dcu/fix-578-2-archive-rollback`，worktree `/tmp/edgec-land/fix2-archive-rollback`。未合并，等 dcu-verifier 验证。
