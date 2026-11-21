# DCU-D1-03 证据包：698 对象字典改为静态对象表

- 分支：`dcu/D1-03-object-model`
- 起点：`f8e49709`（main）
- 提交：`refactor(DCU-D1-03): move 698 object dictionary to a static table`
- 性质：重构，行为不变。

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `cmd/dcu/dcu_698_object_model.h`（新） | `dcu_obj_scope_t`、`dcu_698_object_t`（字段按设计 4.1）、处理器类型、`init/reset/find/read/write/call/snapshot` 声明。 |
| `cmd/dcu/dcu_698_object_model.c`（新） | 12 个 OI 的静态表、16 个类处理器（原样迁移）、查表与分发、快照引擎、遥控下沉端口。 |
| `cmd/dcu/dcu_router.c` | 由 664 行缩到 96 行：只保留 `dcu_tag_map_fn` 与查表门面；删除 `g_obj_dictionary` 与逐对象注册。 |
| `cmd/dcu/dcu_router.h` | 删除只在本文件使用的 `dcu_object_t`，指向新模块。 |
| `cmd/dcu/CMakeLists.txt` | `dcu_698_object_model.c` 加入 `dcu_app_lib`。 |
| `tests/test_dcu_698_object_model.c`（新） | 4 个用例：12 OI 查表/class/scope/mask、缺失 OI、mask 拒绝、读写调用分发、reset。 |
| `tests/CMakeLists.txt` | `add_edge_test(test_dcu_698_object_model ...)` + `dcu_app_lib`。 |

未改 `modules/`、`foundation/`、`adapters/`。

## 为什么

计划 D1-03：对象字典原来在 `dcu_router_init_ex` 里逐条 `obj->oi = ...` 手写注册。
迁到静态表后，新增同类 OI 只加表项；查表集中在一处，为 D1-04 起的表驱动读取铺路。

## 行为保持的关键点

1. **公开 API 与返回码不变。** `dcu_router_get_attr/set_attr/action` 查表后**直接调处理器**，
   不使用 `dcu_698_read/write/call` 的 mask 预检：处理器自己的 `-2`（未定义属性/方法）、
   `-3`、`-4` 原样返回。现有 `test_dcu_router` 对 6000 方法 2/3 的 `-4`、
   8000 的 `-4` 断言全部保持。
2. **条件注册的实例缺失时结果不变。** 表始终含 12 个 OI；`esam`/`discovery`/事件实例为 NULL 时，
   处理器本身返回 -1，与迁移前“对象未注册 → 查表 -1”同码。
3. **`dcu_router_destroy` 语义不变。** 调用后 `dcu_698_find` 返回 NULL，读写方法返回 -1。
4. **快照引擎**迁到 `dcu_698_snapshot`，事件实例为 NULL 时返回 0（迁移前未注册同样返回 0）。
5. **mask 语义**：`dcu_698_call/read/write` 缺属性/方法按 mask 返回 -1。遗留测试方法
   127/128 超出 16 位 mask，未置位；门面直达处理器，所以现有测试路径不受影响。
   这是新 API 与旧门面之间唯一有意的差别，已在头文件与代码注释写明。

## 完成定义逐项

- [x] 对象表使用 `dcu_698_object_t`，字段按设计 4.1；静态存储，init 只填 `instance`。
- [x] `dcu_698_find/read/write/call` 提供；缺 OI / 缺 mask → -1。
- [x] 现有静态处理器全部迁到新文件，没有两处注册。
- [x] `dcu_router_init_ex` 只转交实例；`dcu_router.c` 664 → 96 行。
- [x] 新测试覆盖 12 OI、缺失 OI、mask、分发；`test_dcu_router`、`test_dcu_advanced_modules`、
      `test_dcu_business_engines` 全绿。
- [x] 未改行为、日志语义、启动流程。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `cmake --build build -j8` | exit 0，0 warning |
| `cd build && ctest --output-on-failure -R dcu` | 20/20 通过（新增 `test_dcu_698_object_model`） |
| `./build/tests/test_dcu_698_object_model` | 4/4 通过 |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0 |
| ASAN：新测试 + `test_dcu_router` + `test_dcu_business_engines` | 通过，无泄漏 |
| ASAN：`test_dcu_advanced_modules` | 功能通过；LSan 报告 3 处泄漏，均在 `tests/test_dcu_advanced_modules.c` 自己创建、未释放的 pipeline stage，属既有问题，与本次改动无关 |
| `./test.sh --omnis dcu` | dcu-acceptance 2 passed、dcu-northbound 4 passed、退出码 0 |

## 说明

- 新文件 688 行，超过设计“单文件 600 行”的目标。本单按任务要求把 12 个 OI 的处理器
  集中在 `dcu_698_object_model.c`；D1-04 起按类组拆分（`dcu_698_var.c` 等）时回落。
- `dcu_router.h` 的 `dcu_object_t` 已无外部使用者，本次删除，避免两套对象类型并存。

## 证据文件

- `summary.md`：本文件
- `tests.log`：构建、ctest、新测试、ASAN、守卫、行数
- `runtime.log`：`./test.sh --omnis dcu` 完整输出
- `diffstat.txt`：代码提交的 `git show --stat`
