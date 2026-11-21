# DCU-D1-14 `/no-comments` 审计记录

- 工具：`pstack:no-comments`（comment-sicko 子代理，只读审计）
- 范围：`dcu/D1-14-set` 工作树 diff（base `main`）
- 结论：删除 2 条；1 条 `MUST KILL`（重复 A-XDR 标签表）已按根因修复；无 `HOLD`。

## 删除（2）

| 文件 | 注释 | 理由 |
| --- | --- | --- |
| `tests/test_dcu_698_axdr.c` | `/* 2400 */` | 同一向量已在别处标明 double-long-unsigned 2400，此行只复述数值。 |
| `tests/test_dcu_698_axdr.c` | `/* 没有 Data，不调产品 */` | 复述下一行断言与用例名，属测试意图叙述。 |

## `MUST KILL` 与根因修复（1）

`cmd/dcu/dcu_698_axdr.c::_decode_axdr_scalar` 维护了第二张 A-XDR 标签/宽度表。根因修复：

- 扩展 `foundation/src/protocol/dlt698_codec.c::dlt698_decode_data`，补 BOOLEAN、LONG64、LONG64_UNSIGNED、FLOAT32、FLOAT64；标签宽度只由 codec 一处拥有。
- `_decode_axdr_scalar` 改为调用 `dlt698_decode_data`，只做 tag → `int64_t`/`double` 的取值映射，不再复制宽度。
- 驱动 SET 的 `dlt698_decode_data` 解码门随之支持 float/bool/long64；新增 float32 SET 用例证明 `accept_axdr` 会被调用（审计指出的非注释风险）。

## 保留（代表项）

- `dlt698_codec.h` 的 SET-Response API 文档（外部协议布局）。
- `edge_slave_plane.h` 的 `EDGE_SLAVE_SRC_DENIED → DAR 3` 契约。
- 驱动 SET-Request 载荷布局、产品“完整带标签 / 结构暂不支持”约束。
- 测试中的 0x0A visible-string、0x12 long-unsigned、APDU 偏移 9 说明。

## `HOLD`

无。
