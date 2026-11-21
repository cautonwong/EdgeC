# DCU-D1-04 证据包：变量类（Class 1/3/4/6）带类型标签的读取处理器

- 分支：`dcu/D1-04-var-handlers`
- 起点：`81730385`（main）
- 提交：`feat(DCU-D1-04): add typed variable-class read handlers`
- 性质：新增首批 6 个 OI 的读取；2000/2001 行为不变。

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `cmd/dcu/dcu_698_var.h`（新） | `dcu_value_t`（type_tag/raw/value/ts_ms/quality）、类型标签常量、`dcu_698_var_read_value` 与对象表处理器 `dcu_698_var_read`。 |
| `cmd/dcu/dcu_698_var.c`（新） | 变量类通用读取：OAD → 影子 OAD、类型标签、旧式标量回写。 |
| `cmd/dcu/dcu_698_object_model.c` | 删除 `_oi_variable_read`，变量行改为 `dcu_698_var_read`；新增 0010、0020、2004、200F 四行；新增 `dcu_698_object_model_bus_ops()` 访问器。709 → 704 行。 |
| `cmd/dcu/dcu_698_object_model.h` | 修正过期的 `dcu_object_t` 注释；声明 bus ops 访问器。 |
| `cmd/dcu/dcu_router.h` | 新增 `IC_ENERGY`（1）、`IC_POWER`（4）、`IC_DATA_VARIABLE`（6）。 |
| `cmd/dcu/CMakeLists.txt` | `dcu_698_var.c` 加入 `dcu_app_lib`。 |
| `tests/test_dcu_698_var.c`（新） | 5 个用例：6 OI 值与 type_tag、影子缺失、属性 mask、旧式标量类型、重复读一致。 |
| `tests/CMakeLists.txt` | 注册 `test_dcu_698_var` 并链接 `dcu_app_lib`。 |

未改 `modules/`、`foundation/`、`adapters/`。

## 首批 OI 与类型标签

| OI | Class | 影子 OAD | type_tag | 属性 |
| --- | --- | --- | --- | --- |
| 0010 正向有功电能 | 1 | `OAD_ACTIVE_ENERGY` | 0x15 long64-unsigned | 2、4 |
| 0020 反向有功电能 | 1 | `OAD_REACTIVE_ENERGY` | 0x15 long64-unsigned | 2、4 |
| 2000 电压 | 3 | `OAD_VOLTAGE` | 0x17 float32 | 2 |
| 2001 电流 | 3 | `OAD_CURRENT` | 0x17 float32 | 2 |
| 2004 总有功功率 | 4 | `OAD_ACTIVE_POWER_TOTAL` | 0x17 float32 | 2 |
| 200F 电网频率 | 6 | `OAD_GRID_FREQUENCY` | 0x12 long-unsigned | 2 |

- 量纲不复制：`dcu_value_t` 的 value 直接取影子工程值，raw 由 value 截断；
  scale 仍归 `dcu_driver_adapter`。
- 2000/2001 的旧式标量输出仍是 `float`，`test_dcu_router` 的 220.5/4.95 断言不变。
- 0010/0020 的旧式输出是 `uint64_t`，200F 是 `uint16_t`，由 type_tag 决定。

## 已知不一致（不在本单修）

`docs/design/DCU_698_OI_CATALOG.md` 里 200F 是电网频率、200A 是功率因数；
现有 `OAD_GRID_FREQUENCY` 宏仍指向 0x200A0200，影子数据也写在那里。
本单按任务与目录用 OI 200F，数据源继续读 `OAD_GRID_FREQUENCY`；宏与目录的
对齐属 D1-01 的核对范围。

## 拆分与行数

- `dcu_698_object_model.c`：709 → 704 行。变量处理器已迁出，但新增 4 个 OI 行
  与 bus ops 访问器抵消了大部分收益，仍高于设计 600 行目标。
- 后续拆分点：Class 7 事件处理器（`_oi_event_*`，约 150 行）可迁到
  `dcu_698_event.c`（D3 系列）；Class 9 冻结（`_oi_5004_*`）随 D1-15 迁到
  `dcu_698_freeze.c`。本单不为拆而拆。

## 完成定义逐项

- [x] 新建 `dcu_698_var.{h,c}`，含 6 个 OI 的通用读取。
- [x] `dcu_value_t` 与 type_tag 按设计 4.2 / 5.1 落地。
- [x] 对象表补齐 class_id、scope、attr mask；无方法行 method mask 保持空。
- [x] 变量处理器迁出；表文件行数已记录（704）。
- [x] `dcu_698_object_model.h:7` 过期注释修正。
- [x] 新测试覆盖 6 OI 值/type_tag、影子缺失、mask 拒绝、重复读一致。
- [x] 现有 `test_dcu_router`、`test_dcu_advanced_modules`、`test_dcu_business_engines` 全绿。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `cmake --build build -j8` | exit 0，0 warning |
| `cd build && ctest --output-on-failure -R dcu` | 21/21 通过（新增 `test_dcu_698_var`） |
| `./build/tests/test_dcu_698_var` | 5/5 通过 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0 |
| ASAN：`test_dcu_698_var` / `test_dcu_698_object_model` / `test_dcu_router` | 5/5、7/7、3/3 通过，无泄漏 |
| `./test.sh --omnis dcu` | dcu-acceptance 2 passed、dcu-northbound 4 passed、退出码 0 |

## 证据文件

- `summary.md`：本文件
- `tests.log`：构建、ctest、新测试、ASAN、守卫、行数
- `runtime.log`：`./test.sh --omnis dcu` 完整输出
- `diffstat.txt`：代码提交的 `git show --stat`
