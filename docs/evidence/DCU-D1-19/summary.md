# DCU-D1-19 对象模型按类族拆分

基线与分支：`dcu/D1-19-split`，工作树 `/tmp/dcu-d1-18b`，父提交 `e5906b65`（D1-18b 合并后）。

## 目的

把 `dcu_698_object_model.c` 从单文件处理器集合拆成按接口类族分布的文件，三个开发可以分别改 3000/3005、采集类、控制/安全类而不冲突。纯重构：处理器逐字搬移，只改符号名与共享状态访问方式。

## 文件与职责

| 文件 | 行数 | 职责 |
| --- | ---: | --- |
| `dcu_698_object_model.c` | 279 | `g_objects` 表、init/reset/find/count/at、read/write/call 门面、快照分派、bus_ops 访问器、控制下沉注入、`dcu_698_class_defines_attr`、内部访问器 |
| `dcu_698_class_acq.c` | 184 | 6000/6012/6002/6003 与 5004 处理器 |
| `dcu_698_class_event.c` | 63 | 3000/3005 读写与复位 |
| `dcu_698_class_control.c` | 50 | 8000 方法 |
| `dcu_698_class_security.c` | 53 | F100 鉴权、F200 端口配置读写 |
| `dcu_698_object_internal.h` | 49 | 类族处理器原型与共享状态访问器；不对外安装 |

拆分前 `dcu_698_object_model.c` 634 行，拆分后同文件 279 行。

5004（Class 9 冻结记录）不在边界清单的四个类文件里，但它属于采集侧记录类，随 6000/6012 放进 `dcu_698_class_acq.c`，避免门面文件重新长回处理器集合。

## 共享状态

`dcu_698_object_internal.h` 只声明访问器，不新增全局：

- `dcu_698_internal_sink_ops()`：5004 读冻结条数；
- `dcu_698_internal_inventory()`：8000 按 slot 查表；
- `dcu_698_internal_control_sink(&fn, &ctx)`：8000 取控制下沉端口。

`g_router_bus_ops`、`g_router_sink_ops`、`s_router_inv`、`s_control_sink/ctx` 仍只定义在门面文件，`g_f200_channel_cfg` 随 F200 处理器移入 `dcu_698_class_security.c`。

快照分派仍用处理器符号识别事件类：由 `dcu_698_event_read` 经内部头可见；删除了永远不会进入的空分支 `else if (obj->class_id == IC_RECORD) {}`，行为不变。

## 公共 API

`git diff e5906b65 -- '*.h'` 为空：没有修改任何被安装的头文件，新增的 `dcu_698_object_internal.h` 不是公共 API。`dcu_698_object_model.h` 的公开函数与类型未动。

## 门禁

- 干净构建 + `check_build_warnings.py`：0 warning
- `ctest -j`：335/335 通过（首次并行运行时 `test_edge_app_mgr` 因 1.3 s 级时序失败一次，单跑与全量重跑均通过，与本改动无关）
- ASAN：8/8 相关目标通过
- `run_guard_selftest.py`：全部通过
- 验收 `test.sh --omnis dcu`：dcu-acceptance 3/3、dcu-northbound 9/9
- `git diff --check`：干净
