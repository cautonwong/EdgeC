# DCU-D0-05b `/no-comments` 审计记录

- 工具：`pstack:no-comments`（comment-sicko 子代理，只读审计）
- 范围：`dcu/D0-05b-dar` 工作树 diff（base `main`）
- 结论：删除 2 条；保留的注释均为协议/API 契约或非显然约束；无 `HOLD`、无 `MUST KILL`。

## 删除（2）

| 文件 | 注释 | 理由 |
| --- | --- | --- |
| `adapters/io_drivers/src/driver_dlt698.c` | `/* 无此对象：按标准回 DAR，主站不再等超时。 */` | 该分支对 CLASS_MISMATCH / TYPE_MISMATCH / UNDEFINED / TEMPORARY 同样触发，“无此对象”把范围说窄；后半句是变更叙述。 |
| `tests/test_dcu_698_axdr.c` | `/* 复杂属性还没有 A-XDR 处理器：返回对象不存在，不降级成标量。 */` | “还没有”是时间性状态，处理器落地后即过期；“返回对象不存在”复述下一行断言。 |

## 保留（代表项）

- `edge_slave_plane.h` 的 `EDGE_SLAVE_SRC_*` 枚举文档：公开端口契约，说明 698 DAR 映射、其它驱动只把非 0 当失败、旧 `-1` → NOT_FOUND 兼容。
- `dlt698_codec.h` 的 `dlt698_encode_get_response_dar` 文档：APDU 布局 + 标准 6.3.8.2 / 6.3.3.11。
- `dlt698_codec.c` 的 APDU 布局与 `/* CHOICE [0] DAR */`：解释裸字节常量。
- `driver_dlt698.c` 的 `_dar_for_source_rc` 映射注释、A-XDR 通道契约（完整值、不回落、不截断、单缓冲复用）。
- 测试中的缓冲约束、DAR 映射、9 字节帧头偏移、故意非法类型 `0x99` 的说明。

## 未处理（out of diff）

`driver_dlt698.c` 的“应答必须回请求里的地址…”长注释不在本单 diff 内，未审计；若做全文件清理再处理。
