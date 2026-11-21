# DCU-D1-13 证据包：698 从站 A-XDR 端口

- 分支：`dcu/D1-13-axdr-port`
- 起点：`c4d28ebb`（main）
- 提交：`feat(DCU-D1-13): add 698 A-XDR slave port`
- 性质：端口扩展 + 驱动 GET 路径，IEC104/Modbus 标量路径不变。

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `foundation/include/edge_slave_plane.h` | `edge_slave_source_ops_t` 末尾追加 `resolve_axdr` / `accept_axdr`；新增 `DCU_698_AXDR_MAX`（2048）。 |
| `adapters/io_drivers/src/driver_dlt698.c` | GET Normal / NormalList 优先走 `resolve_axdr`，为 NULL 才回落标量；缓冲不足失败，不截断。编码缓冲用线程局部静态数组。 |
| `cmd/dcu/dcu_698_axdr.{h,c}`（新） | 产品 A-XDR 实现：变量类经 `dcu_698_var_read_value`，6000 档案计数单独编码；`accept_axdr` 明确返回 -1（待 D1-14）。 |
| `cmd/dcu/dcu_north_adapter.{c,h}` | 从站 ops 接线两个新回调；A-XDR 编码体移到独立文件，组合根只多 include + 两行。 |
| `cmd/dcu/CMakeLists.txt` | `dcu_698_axdr.c` 加入 `dcu_app_lib`。 |
| `tests/test_dcu_698_axdr.c`（新） | 6 个用例：产品 7 类变量 A-XDR、缓冲不足/复杂属性拒绝、驱动标量回落、结构透传、NormalList 逐项、驱动缓冲不足。 |
| `tests/CMakeLists.txt` | 注册 `test_dcu_698_axdr` 并链接 `dcu_app_lib`。 |
| `ci/baseline/composition_root_budget.txt` | `dcu_north_adapter.c` 201 → 203（include + ops 两行），编码体不在组合根。 |

未改 `modules/`。

## 决策落地

- 字段只追加在 `edge_slave_source_ops_t` 末尾；现有初始化点都是 designated initializer，省略成员零初始化为 NULL。
- 回落只认 NULL：`resolve_axdr` 非 NULL 时驱动只走 A-XDR，返回非 0 是真实失败，不回落标量。
- 缓冲上限 `DCU_698_AXDR_MAX = 2048`（单条属性）；超限/放不下返回失败，不截断。
- 复杂属性（Class 7/9/10/20 结构）还没有 A-XDR 处理器时，产品返回 -1；驱动按对象不存在处理，不降级成标量。

## 缓冲策略

驱动不在栈上放 2048/16 KiB 数组（`check_stack_usage.py` 上限 512 字节）。用线程局部静态缓冲：

```c
static _Thread_local uint8_t s_axdr_buf[DCU_698_AXDR_MAX];
static _Thread_local uint8_t s_axdr_list_buf[DLT698_GET_LIST_MAX][DCU_698_AXDR_MAX];
```

从站交换在 io 线程一问一答串行；线程局部避免多线程共享。若未来同一线程并行处理多路 698 从站，需要改为调用方/池缓冲（注释已写明）。

## 完成定义逐项

- [x] 两个可选回调追加在结构末尾。
- [x] 产品实现 `resolve_axdr`：变量 OI（0010、0020、2000、2001、2004、200A、200F）+ 6000 档案计数；复杂属性返回 -1。
- [x] `accept_axdr` 先返回 -1，注释指向 D1-14。
- [x] 驱动 GET Normal / NormalList 优先 A-XDR，NULL 才回落；缓冲不足失败。
- [x] 测试覆盖标量回落、变量值与 type_tag、结构透传、缓冲不足、NormalList、无动态内存。
- [x] `ctest -R 'dcu|dlt698'` 28/28 通过；守卫与验收通过。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `cmake --build build -j8` | exit 0，0 warning |
| `cd build && ctest --output-on-failure -R 'dcu\|dlt698'` | 28/28 通过 |
| `./build/tests/test_dcu_698_axdr` | 6/6 通过 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0 |
| ASAN：`test_dcu_698_axdr` | 6/6 通过，无泄漏 |
| `./test.sh --omnis dcu` | dcu-acceptance 2 passed、dcu-northbound 4 passed、退出码 0 |

## 证据文件

- `summary.md`：本文件
- `tests.log`：构建、ctest、新测试、ASAN、守卫、组合根预算
- `runtime.log`：`./test.sh --omnis dcu` 完整输出
- `diffstat.txt`：代码提交的 `git show --stat`
