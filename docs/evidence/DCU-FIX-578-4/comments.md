# DCU-FIX-578-4：/no-comments（Comment Sicko）

范围：`git diff 45cdc244..HEAD`，5 个文件。

## 接受删除

- `tests/test_dcu_698_appconn.c` 中 `_set_dar` 上方的说明注释（1 行）：只是复述函数名与函数体；
  "session 串行复用"也与实现不符（每次调用都 memset 重置）。已提交 `5af12e6d`。

## 报告指出的过期断言（已修正）

- `cmd/dcu/dcu_698_appconn.c` 的 A6 注释原写"否则合法 SET 会落到写路径的 DAR 7"。同批修复把
  门面的无写种类归类改成 DENIED 后，该括号不再成立（ACL 若仍广告可写，合法 A6 SET 会得到 DAR 3）。
  已改为"能力与广告必须一致"，保留 A8/A9 的 DAR 5 可达性说明。已并入修复提交 `8060429c`。

## 核实后的保留

- `dcu_698_appconn.c`：A6 只读 + A8/A9 保留可写的分割理由（否则 DAR 5 在驱动 ACL 层被短路成 DAR 3）。
- `tests/test_dcu_698_appconn.c`：表182 对象列表字节布局、SET-Response payload 布局（OAD4|DAR|FollowReport|时间标签）。
- `docs/design/DCU_698_OBJECT_MODEL.md` 的 A6/A8/A9 行为说明与实现一致。

## 第 2 轮（独立 Comment Sicko）

- 接受删除 5 块 / 6 行：
  - `cmd/dcu/dcu_698_axdr.c`：无写种类 = 只读 → DENIED 的说明。枚举契约在 `edge_slave_plane.h`，测试与设计文档也覆盖同一分类。
  - `tests/test_dcu_698_axdr.c`：A6 断言后的两处尾注（`/* A6 只读：合法值也不是类型错 */`、`/* A6 只读 */`）。
  - `tests/test_dcu_698_appconn.c`：测试函数上方两行 ACL/写路径说明，与 `dcu_698_appconn.c` 的生产注释重复。
  - `tests/test_dcu_698_appconn.c`：门面直调断言上方的意图注释，紧随其后的两条断言已表达。
- 保留（有证明）：
  - `dcu_698_appconn.c` 的 ACL 分割理由。这是文件内唯一记录 A8/A9 故意保留 rights=3 以确保 DAR 5 可达的地方；删掉会让未来"统一 ACL"的清理悄悄改变线上 DAR。
  - `tests/test_dcu_698_appconn.c` 的表182 字节布局与 SET-Response payload 布局。两者是外部协议格式，裸下标不可自解释。
- `MUST KILL`：无。应用代码、断言、范围外注释均未改。

## 其他

- 无 MUST KILL，无 workaround 代码。
- 报告提示 `_build_frame` 与 `tests/test_dlt698_appconn.c` 的同名助手重复（测试代码重复，非 workaround）；
  不在本缺陷范围，未改。

## 结论

删除 7 行注释（第 1 轮 1 行，第 2 轮 6 行）；修正 1 处过期断言；应用逻辑与断言未因本环节改动。
