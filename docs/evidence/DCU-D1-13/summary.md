# DCU-D1-13 证据包：698 从站 A-XDR 端口

- 分支：`dcu/D1-13-axdr-port`
- 起点：`c4d28ebb`（main）
- 提交：
  - `feat(DCU-D1-13): add 698 A-XDR slave port`
  - `fix(DCU-D1-13): stream NormalList A-XDR and rename the port cap`
- 性质：端口扩展 + 驱动 GET 路径，IEC104/Modbus 标量路径不变。

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `foundation/include/edge_slave_plane.h` | `edge_slave_source_ops_t` 末尾追加 `resolve_axdr` / `accept_axdr`；新增 `EDGE_SLAVE_AXDR_MAX`（2048）。 |
| `adapters/io_drivers/src/driver_dlt698.c` | GET Normal 优先 `resolve_axdr`，NULL 才回落标量；NormalList 用流式 API 逐项编码，单缓冲复用；缓冲不足失败，不截断。 |
| `foundation/include/dlt698_codec.h` / `foundation/src/protocol/dlt698_codec.c` | 新增流式 NormalList 编码入口 `begin/item/item_dar/end`；`_frame_seal` 抽出按偏移封帧的 helper。 |
| `cmd/dcu/dcu_698_axdr.{h,c}`（新） | 产品 A-XDR 实现：变量类经 `dcu_698_var_read_value`，6000 档案计数单独编码；`accept_axdr` 返回 -1（待 D1-14）。 |
| `cmd/dcu/dcu_north_adapter.{c,h}` | 从站 ops 接线两个新回调；A-XDR 编码体在独立文件。 |
| `cmd/dcu/CMakeLists.txt` | `dcu_698_axdr.c` 加入 `dcu_app_lib`。 |
| `tests/test_dcu_698_axdr.c`（新） | 7 个用例：产品变量 A-XDR、缓冲不足/复杂属性拒绝、驱动标量回落、结构透传、NormalList 逐项、8 项近上限列表单缓冲复用、驱动缓冲不足。 |
| `tests/CMakeLists.txt` | 注册 `test_dcu_698_axdr` 并链接 `dcu_app_lib`。 |
| `ci/baseline/composition_root_budget.txt` | `dcu_north_adapter.c` 201 → 203（include + ops 两行）。 |

未改 `modules/`。

## 缓冲方案（review 修正）

- **单条 GET**：一个 `_Thread_local uint8_t s_axdr_buf[EDGE_SLAVE_AXDR_MAX]`，单线程事件循环一问一答串行复用；同一线程并行多路 698 从站时改为调用方/池缓冲（注释已写明）。
- **NormalList**：删除 `8 × 2048` 列表缓冲。用 `dlt698_encode_get_response_list_begin/item/item_dar/end` 逐项取、逐项编码进 `out`，同一个 2048 缓冲每项复用。
- 不再出现 `N × 2048` 隐藏状态；8 项测试每项 2040 字节，响应完整、不串、不截断。

## 决策落地

- 字段只追加在 `edge_slave_source_ops_t` 末尾；现有初始化点都是 designated initializer。
- 回落只认 NULL：`resolve_axdr` 非 NULL 时驱动只走 A-XDR，返回非 0 是真实失败。
- 单条属性上限 `EDGE_SLAVE_AXDR_MAX = 2048`；超限/放不下返回失败。
- 复杂属性（Class 7/9/10/20 结构）没有 A-XDR 处理器时产品返回 -1；驱动按对象不存在处理，不降级标量。

## 完成定义逐项

- [x] 两个可选回调追加在结构末尾。
- [x] 产品实现 `resolve_axdr`：变量 OI + 6000 档案计数；复杂属性返回 -1。
- [x] `accept_axdr` 先返回 -1，注释指向 D1-14。
- [x] 驱动 GET Normal / NormalList 优先 A-XDR，NULL 才回落；缓冲不足失败。
- [x] NormalList 流式逐项编码，无 `N × 2048` 缓冲。
- [x] 常量改名 `EDGE_SLAVE_AXDR_MAX`，ADR 与设计文档同步。
- [x] 测试覆盖标量回落、变量值与 type_tag、结构透传、8 项近上限单缓冲复用、缓冲不足、无动态内存。
- [x] `ctest -R 'dcu|dlt698'` 28/28 通过；守卫与验收通过。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `cmake --build build -j8` | exit 0，0 warning |
| `cd build && ctest --output-on-failure -R 'dcu\|dlt698'` | 28/28 通过 |
| `./build/tests/test_dcu_698_axdr` | 7/7 通过 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0 |
| ASAN：`test_dcu_698_axdr` | 7/7 通过，无泄漏 |
| `./test.sh --omnis dcu` | dcu-acceptance 2 passed、dcu-northbound 4 passed、退出码 0 |

## 证据文件

- `summary.md`：本文件
- `tests.log`：构建、ctest、新测试、ASAN、守卫、组合根预算、驱动缓冲
- `runtime.log`：`./test.sh --omnis dcu` 完整输出
- `diffstat.txt`：修正提交的 `git show --stat`
