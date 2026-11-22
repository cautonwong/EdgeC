# DCU-D1-12 证据包：698 从站 A-XDR 端口 ADR

- 分支：`dcu/D1-12-axdr-port-adr`
- 起点：`8d8089bf`（main）
- 提交：`docs(DCU-D1-12): add 698 slave A-XDR port ADR`
- 性质：纯文档，不改代码。

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `docs/adr/0013-698-slave-axdr-port.md`（新） | 新增 ADR：`resolve_axdr` / `accept_axdr` 签名、语义、回落规则、2048 上限、兼容性逐点分析、被否方案、D1-13 到 D1-17 后续。状态 Proposed。 |
| `docs/design/slave_plane_architecture.md` | 新增 3.3 节“从站取数端口（Consumer Ports）”：标注第 2 节为 ADR-0002 前形态；列出端口字段用途与 ADR-0013 的四条规则。 |
| `docs/design/DCU_698_OBJECT_MODEL.md` 4.5 | 与 ADR 对齐：补 ADR-0013 链接，明确“回落只认 NULL、已实现但返回非 0 不得回落”。 |

未改 `modules/`、`foundation/`、`adapters/`、`cmd/`。

## 为什么

计划 11.3 的 B1：`resolve` 只返回标量，Class 7、9、10、20 的数组与结构传不出去；
`driver_dlt698.c` 只有 GET 与 ACTION 分支，没有 SET/Record/Report/Class 20。
D1-12 是 B1 的唯一解除路径，也是 D1-05 到 D1-07 与 P2 到 P4 的前置。

## 完成定义逐项

- [x] ADR 写清背景与问题，引用 `edge_slave_plane.h` 与 `driver_dlt698.c` 的具体行。
- [x] 决策给出两个可选回调的完整签名、缓冲语义与 2048 上限。
- [x] 驱动行为：GET 先 `resolve_axdr`、SET 先 `accept_axdr`，NULL 才回落标量；
      IEC104/Modbus 不变；驱动仍不认识点位模型。
- [x] 兼容性硬问题：列出全部 14 个直接初始化点与 6 个拷贝/转传点，逐个判定
      designated initializer；结论为“追加到末尾安全”，并定下只追加规则。
- [x] 被否方案 5 个：仅标量、对象模型进驱动、进 foundation、内联 2048 缓冲、驱动动态分配。
- [x] 影响与后续映射到 D1-13 到 D1-17，并说明本 ADR 不实现代码。
- [x] 状态 Proposed。
- [x] 设计文档同步更新；`DCU_698_OBJECT_MODEL.md` 4.5 原本已接近一致，本次只补链接与回落语义。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `grep -rn "edge_slave_source_ops_t" --include=*.c --include=*.h` | 20 条命中：14 个直接初始化 + 6 个拷贝/转传；全部为 designated initializer 或整结构拷贝。明细见 `review.md` 与 `tests.log`。 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0，全部守卫与 live 检查通过。 |
| `git diff --check` | exit 0，无空白错误。 |

本任务不改代码，未跑构建与测试；文档改动未触及守卫输入。

## 证据文件

- `summary.md`：本文件
- `review.md`：兼容性结论与逐点清单
- `tests.log`：grep、`git diff --check`、guard selftest 输出
- `diffstat.txt`：`git show --stat`
