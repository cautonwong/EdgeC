# DCU-FIX-578-3：/no-comments（Comment Sicko）

范围：`git diff 45cdc244..HEAD`，2 个文件。

## 第 1 轮

- 接受删除：测试中 `/* GET 6014 A2 仍能看到方案 7：删除只是被拒绝，对象没有消失。 */`。理由成立：原实现直接调
  `dcu_698_scheme_encode_axdr`，没有走 `dcu_north_resolve_axdr` 的 GET 入口，注释指的是错误的层。
- 保留（已核实）：128 的 `LOG_WARN` 文本（携带方案号与拒绝原因，与 127/129/130 对齐）；
  上下文里既有的 `/* 列不变的重下发允许 */`（说明相邻"拒绝改 CSD、允许原样重下发"不是矛盾）。

## 按报告 MUST KILL 落实（提交 `067684be`）

- 改用既有 `_a2()` 助手（`dcu_north_resolve_axdr`）驱动真实 GET 路径；
- 抽出 `_a2_has_scheme_id()`，让断言自解释，替代匿名的 4 字节 `memcmp` 扫描；
- 代码已走真实 GET 后，恢复一行意图注释："GET 6014 A2 仍服务方案 7：删除只是被拒绝，对象没有消失。"

## 第 2 轮（独立 Comment Sicko）

- 接受删除：`_a2_has_scheme_id()` 上方 `/* 方案单元以 structure{6} | unsigned(id) 开头：按这个前缀判定 A2 是否含指定编号。 */`。函数体已经构造同样的 `head[]`，注释是复述。
- 接受删除：`test_referenced_scheme_refuses_csd_change_and_delete` 中恢复过的一行 `/* GET 6014 A2 仍服务方案 7：删除只是被拒绝，对象没有消失。 */`。测试名与紧随其后的 `_delete_ids` DENIED、`dcu_poller_get_scheme` 非空、`_a2_has_scheme_id` 三条断言已表达同一不变量。
- 保留：无新增注释。`LOG_WARN` 是运行期消息，不是注释；范围外既有注释不动。
- `MUST KILL`：无。应用代码、断言、范围外注释均未改。

## 结论

删除 3 处注释（第 1 轮 1 处错层，第 2 轮 2 处复述）；其 MUST KILL（真实 GET 路径 + 具名断言）已落实；无 workaround 代码。
