# DCU-D1-04 证据包：变量类（Class 1/3/4/6）带类型标签的读取处理器

- 分支：`dcu/D1-04-var-handlers`
- 起点：`149d2634`（main，含 D1-03 注释清理与 D1-04 review 记录）
- 提交：
  - `feat(DCU-D1-04): add typed variable-class read handlers`
  - `fix(DCU-D1-04): align variable OI macros and energy type tags`
- 性质：新增首批变量读取；2000/2001 行为不变。

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `cmd/dcu/dcu_698_var.h`（新） | `dcu_value_t`、类型标签常量、`dcu_698_var_read_value`、`dcu_698_var_read`。 |
| `cmd/dcu/dcu_698_var.c`（新） | 变量类通用读取：OAD → 影子 OAD、按属性选类型标签、旧式标量回写。 |
| `cmd/dcu/dcu_698_object_model.c` | 删除 `_oi_variable_read`；新增 0010、0020、2004、200A、200F 五行；新增 bus ops 访问器。709 → 704 行。 |
| `cmd/dcu/dcu_698_object_model.h` | 声明 bus ops 访问器；删除过期 `dcu_object_t` 注释。 |
| `cmd/dcu/dcu_router.h` | 新增 `IC_ENERGY`、`IC_POWER`、`IC_DATA_VARIABLE`；修正 OAD 宏错位。 |
| `cmd/dcu/CMakeLists.txt` | `dcu_698_var.c` 加入 `dcu_app_lib`。 |
| `tests/test_dcu_698_var.c`（新） | 7 个用例：7 OI 值与 type_tag、影子缺失、属性 mask、旧式标量、电能属性 2/4 标签、200A/200F 分离、重复读一致。 |
| `tests/CMakeLists.txt` | 注册 `test_dcu_698_var` 并链接 `dcu_app_lib`。 |

未改 `modules/`、`foundation/`、`adapters/`。

## OAD 宏对齐（review 修正）

`dcu_router.h` 原先把 2009 标成功率因数、200A 标成频率，与标准/目录错位。现改为：

- `OAD_POWER_FACTOR_TOTAL = 0x200A0200`（200A 功率因数）
- `OAD_GRID_FREQUENCY = 0x200F0200`（200F 电网频率）

`dcu_driver_adapter.c`、`dcu_north_source.c`、`dcu_north_adapter.c` 与测试都通过这两个宏取址，
改动后功率因数与频率的影子写入/读取一起对齐。200F 对象行现在读 200F 影子，
不再读 200A。

## 首批 OI 与类型标签

| OI | Class | 影子 OAD | attr 2 | attr 4 | 属性 |
| --- | --- | --- | --- | --- | --- |
| 0010 正向有功电能 | 1 | `OAD_ACTIVE_ENERGY` | 0x06 double-long-unsigned | 0x15 long64-unsigned | 2、4 |
| 0020 反向有功电能 | 1 | `OAD_REACTIVE_ENERGY` | 0x06 | 0x15 | 2、4 |
| 2000 电压 | 3 | `OAD_VOLTAGE` | 0x17 float32 | — | 2 |
| 2001 电流 | 3 | `OAD_CURRENT` | 0x17 float32 | — | 2 |
| 2004 总有功功率 | 4 | `OAD_ACTIVE_POWER_TOTAL` | 0x17 float32 | — | 2 |
| 200A 功率因数 | 4 | `OAD_POWER_FACTOR_TOTAL` | 0x11 long | — | 2 |
| 200F 电网频率 | 6 | `OAD_GRID_FREQUENCY` | 0x12 long-unsigned | — | 2 |

- 量纲不复制：value 取影子工程值，raw 由 value 截断；scale 仍归 `dcu_driver_adapter`。
- 旧式标量输出按 type_tag：float32 → float、0x06 → uint32、0x15 → uint64、
  0x11 → int16、0x12 → uint16。2000/2001 仍是 float，`test_dcu_router` 的
  220.5/4.95 断言不变。

## 拆分与行数

`dcu_698_object_model.c`：709 → 704 行。变量处理器已迁出，但新增 5 个 OI 行与
bus ops 访问器抵消了大部分收益。后续拆分点：Class 7 事件（约 150 行）→
`dcu_698_event.c`（D3 系列）；Class 9 冻结 → `dcu_698_freeze.c`（D1-15）。

## 完成定义逐项

- [x] 新建 `dcu_698_var.{h,c}`，含变量类通用读取。
- [x] `dcu_value_t` 与 type_tag 按设计 4.2 / 5.1 落地；电能按属性分 0x06/0x15。
- [x] 对象表补齐 class_id、scope、attr mask；无方法行 method mask 保持空。
- [x] OAD 宏 200A/200F 与标准/目录对齐，影子读写跟随。
- [x] 新测试覆盖 7 OI 值/type_tag、200A/200F 分离、电能属性 2/4 标签、影子缺失、mask 拒绝、重复读一致。
- [x] 现有 `test_dcu_router`、`test_dcu_advanced_modules`、`test_dcu_business_engines` 全绿。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `cmake --build build -j8` | exit 0，0 warning |
| `cd build && ctest --output-on-failure -R dcu` | 21/21 通过 |
| `./build/tests/test_dcu_698_var` | 7/7 通过 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0 |
| ASAN：`test_dcu_698_var` | 7/7 通过，无泄漏 |
| `./test.sh --omnis dcu` | dcu-acceptance 2 passed、dcu-northbound 4 passed、退出码 0 |

## 证据文件

- `summary.md`：本文件
- `tests.log`：构建、ctest、新测试、ASAN、守卫、行数
- `runtime.log`：`./test.sh --omnis dcu` 完整输出
- `diffstat.txt`：修正提交的 `git show --stat`
