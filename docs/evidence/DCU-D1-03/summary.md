# DCU-D1-03 证据包：698 对象字典改为静态对象表

- 分支：`dcu/D1-03-object-model`
- 起点：`0fa39a89`（main，含 256 位 mask 设计）
- 提交：
  - `refactor(DCU-D1-03): move 698 object dictionary to a static table`
  - `refactor(DCU-D1-03): cover full 698 attribute and method id range`
- 性质：重构，行为不变（mask 的 256 位修正是根因修复，见下）。

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `cmd/dcu/dcu_698_object_model.h`（新） | `dcu_obj_scope_t`、`dcu_698_object_t`（字段按设计 4.1）、`dcu_mask_test/set`（256 位位图）、处理器类型、`init/reset/find/read/write/call/snapshot`。 |
| `cmd/dcu/dcu_698_object_model.c`（新） | 12 个 OI 静态表、16 个类处理器（原样迁移）、256 位 mask、查表与分发、快照、遥控下沉端口。 |
| `cmd/dcu/dcu_router.c` | 664 → 76 行：只保留 `dcu_tag_map_fn` 与查表门面；删除 `g_obj_dictionary` 与逐对象注册。 |
| `cmd/dcu/dcu_router.h` | 196 → 184 行，删除只在本文件使用的 `dcu_object_t`。 |
| `cmd/dcu/CMakeLists.txt` | `dcu_698_object_model.c` 加入 `dcu_app_lib`。 |
| `tests/test_dcu_698_object_model.c`（新） | 7 个用例：12 OI 查表/class/scope/mask 精确比对、宽 id 位图、mask 拒绝、127/128 分发、读写调用、门面与表 API 一致、reset。 |
| `tests/CMakeLists.txt` | `add_edge_test(test_dcu_698_object_model ...)` + `dcu_app_lib`。 |

未改 `modules/`、`foundation/`、`adapters/`。

## 256 位 mask 与覆盖清单

`attr_mask` / `method_mask` 改为 `uint8_t[32]`，`dcu_mask_test/set` 支持 0..255。
静态初始化用 `BYTE(id)` / `BIT(id)` 写，避免手算十六进制。按处理器实际 case：

| OI | attr_mask | method_mask |
| --- | --- | --- |
| 6000 | 2 | 1,2,3,4,5,127,128 |
| 6012 | 2 | 1 |
| 2000 | 2 | — |
| 2001 | 2 | — |
| 5004 | 2 | 3,4,5,7,8（实现随 D1-15，现回 -4） |
| 8000 | — | 1 |
| F100 | — | 1 |
| F200 | 2,3 | — |
| 6002 | 2,3 | 127 |
| 6003 | 2 | — |
| 3000 | 2,3,4,5,6,8,9,10 | 1 |
| 3005 | 2,3,4,5,6,8,9,10 | 1 |

- 6000 处理器只实现 127/128，129/131/134 未置位；`dcu_698_call(129)` 返回 -1。
- 5004 没有真实方法实现，但 mask 登记了 3/4/5/7/8，并补最小 `_oi_5004_execute`
  对这几个方法返回 -4（存在但拒绝），保持“mask 与处理器一致”，不假成功。
- 属性 18 未在任何 12 个 OI 中置位；`dcu_mask_set/test` 的宽 id 语义由独立用例覆盖。

## 门面不再绕过 mask

`dcu_router_get_attr/set_attr/action` 现在直接调用 `dcu_698_read/write/call`，
mask 预检生效；处理器自己的 `-2/-3/-4` 仍原样上抛。现有 `test_dcu_router` 的
`-4` 断言（6000 方法 2/3、8000 动作）继续通过。缺失属性从处理器的 -2 变为 -1，
这是设计要求的统一语义，当前没有测试依赖旧 -2。

## 完成定义逐项

- [x] 对象表使用 `dcu_698_object_t`，字段按设计 4.1；静态存储，init 只填 `instance`。
- [x] `attr_mask` / `method_mask` 为 256 位位图，`dcu_mask_test/set` 提供。
- [x] mask 按处理器实际属性/方法 case 填写，覆盖 127/128 等宽 id。
- [x] `dcu_698_find/read/write/call` 提供；缺 OI / 缺 mask → -1。
- [x] 门面走同一套 mask 预检，不再绕过。
- [x] 现有静态处理器全部迁到新文件，没有两处注册。
- [x] `dcu_router_init_ex` 只转交实例；`dcu_router.c` 664 → 76 行。
- [x] 新测试覆盖 12 OI、宽 id、缺失 OI、mask、127/128、门面一致性。
- [x] `test_dcu_router`、`test_dcu_advanced_modules`、`test_dcu_business_engines` 全绿。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `cmake --build build -j8` | exit 0，0 warning |
| `cd build && ctest --output-on-failure -R dcu` | 20/20 通过 |
| `./build/tests/test_dcu_698_object_model` | 7/7 通过 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0 |
| ASAN：新测试 + `test_dcu_router` | 7/7、3/3 通过，无泄漏 |
| `./test.sh --omnis dcu` | dcu-acceptance 2 passed、dcu-northbound 4 passed、退出码 0 |

## 说明

- `dcu_698_object_model.c` 709 行，低于 `check_single_responsibility.py` 的 1000/1500 行门限；
  设计“单文件 600 行”的目标随 D1-04 按类组拆分实现。
- `dcu_router.h` 的 `dcu_object_t` 已无外部使用者，本次删除，避免两套对象类型并存。

## 证据文件

- `summary.md`：本文件
- `tests.log`：构建、ctest、新测试、ASAN、守卫、行数
- `runtime.log`：`./test.sh --omnis dcu` 完整输出
- `diffstat.txt`：mask 修正提交的 `git show --stat`
