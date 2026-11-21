# DCU-D1-15b 注释审查

工具：`pstack:no-comments`（comment-sicko 子代理，opus），范围为 D1-15b 未提交改动。ADR-0014 作为端口形状的约束来源，未被修改。

## 接受的处理

删除 9 处注释：

- `_handle_get_record` 解码失败分支的“回空列”说明——它描述的分支已在 MUST KILL 1 中改写，说明随之消失。
- 一致性用例里“选择方法 3 不在子集”的文档注释：复述测试名与断言。
- 驱动用例的段落横幅、`SIZE_MAX` 哨兵说明，以及五个测试函数的意图注释：都在复述测试名或下面的字节向量。

## 执行的 MUST KILL

1. `_handle_get_record` 原先把所有解码错误都回 DAR 2，截断报文也得到应答；现在区分返回码：`UNSUPPORTED_SELECTOR` / `UNSUPPORTED_DATA` 回 DAR 2，`-1`（截断/结构错）直接不应答。新增驱动用例 `test_slave_get_record_malformed_is_dropped` 断言返回 -1 且响应缓冲为空，覆盖了这条此前没有测试的分支。
2. `fake_record_t` 不再用 `SIZE_MAX` 当“没有行失败”的哨兵，改为显式 `bool row_fails`；用例初始化器自解释。

## 审查报告附带发现（已修）

`dlt698_encode_get_record_row` 失败时原先直接返回 -1、没有调用 `record.end()`，begin/end 配平在缓冲耗尽路径上被破坏；现在先 `end` 再返回。

## 保留的注释

- `_handle_get_record` 的表74 说明：整条结果才有 DAR，行失败丢弃整页——这是非显然的“为什么”，ADR-0014 §3 是同一条决策。
- 记录条数一字节的上限与“没有 GetResponseNext 不得截断”：表74 与表76。
- 端口文档（NULL 三连表示不支持、raw selection、行写 N 列含标签、调用方缓冲、按绝对行号续页）：公开 API 契约，与 ADR-0014 一致。
- `consumed` 参数文档、RSD 结构文档、H.3.3 向量出处、协议字段标签（SA/CA/PIID/RSD 长度）。

审查人对协议类注释逐条核对了 `docs/698_parsed.md`（表74 :2723-2741、表76 :2760-2778、表67 :2592-2606、表30 :1797）。

## HOLD

无。

## 审查后的验证

- 重建：0 错误 0 告警。
- `test_driver_dlt698`：10/10；一致性套件：30/30。
- `ctest -R 'dcu|dlt698'`、ASAN、守卫、验收随后在最终树上重跑（见 tests.log）。
