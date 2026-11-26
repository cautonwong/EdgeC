# DCU-FIX-578-2：/no-comments（Comment Sicko）

范围：`git diff 45cdc244..HEAD`，5 个文件。

## 第 1 轮

- 接受删除：`tests/test_dcu_698_archive.c` 中 `/* 落库成功时 130 仍按新值生效。 */`。紧随其后的 5 条断言已说明成功路径，注释是纯复述。已提交 `88ea2561`。
- 保留（已核实）：`dcu_inventory.h` 新增 API 的契约注释（恢复快照 + revision 回退 + 6000 方法范围）；
  `docs/evidence/DCU-D2-02/summary.md` 的写入顺序说明（AGENTS.md 要求的行为文档同步）。
  第 1 轮曾保留测试中"落库失败时 6001 A2 必须仍服务旧档案"的注释，第 2 轮已删除，见下。
- 未改动：上下文里既有的 `/* 落库失败不留 RAM 半态 */`、`_add_batch` 全成全败注释等。

## 第 2 轮（独立 Comment Sicko）

- 接受删除：`tests/test_dcu_698_archive.c` 中 `/* 落库失败时 6001 A2 必须仍服务旧档案：RAM 快照回滚，revision 一并回退。 */`。
  测试名 `test_update_basic_save_failure_rolls_back` 与紧随其后的断言已逐条覆盖：保存失败、旧地址存在、新地址不存在、baud/channel 旧值、revision 不变、A2 只含旧地址。纯复述。
- 保留（已核实）：`dcu_inventory.h` 的 `dcu_inventory_restore_unit` `@brief`。这是模块公开 API 契约，和相邻导出的注释同风格，并写明调用方依赖的 revision 回退后置条件。
- `MUST KILL`：无。应用代码、测试断言、范围外注释均未改。

## 报告的残留（未改，未决）

- 130 的 `rc == -1` 分支在预查 `dcu_inventory_by_seq` 之后不可达，属防御性冗余；保留不删（一行成本，未来更新函数语义变化时仍成立）。
- `dcu_inventory_restore_unit` 在 `config_seq` 不存在时静默 no-op；单线程循环内紧邻更新后不可达。加断言会更显式，但不在本缺陷范围。

## 结论

删除 2 处注释（第 1 轮 1 处，第 2 轮 1 处）；无 workaround 代码（快照+回滚与 Add 路径同一模式，是根因形状）；应用代码与断言未因本环节改动。
