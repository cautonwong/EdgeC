# DCU-D4-04b 五栏规格：Class 19 `4300` 设备管理软件面领域引擎

## 1. 业务规则
1. 新建 `cmd/dcu/dcu_698_devmgr.h` 与 `cmd/dcu/dcu_698_devmgr.c`，实现纯软件 Class 19（`0x4300` 电气设备）设备管理领域引擎（独立模块，暂不修改 `dcu_698_object_model.c`）：
   - 维护固定静态设备信息结构 `dcu_devmgr_info_t`（设备描述符、厂商代码 4B、软件版本号 4B、软件版本日期 6B、硬件版本号 4B、硬件版本日期 6B、厂家扩展信息 8B、生产日期 `date_time_s` 7B、支持规约列表）。
   - 提供属性 A-XDR 编码器 `dcu_698_devmgr_encode_attr(attr_id, buf, cap, out_len)`：
     - `A2`：设备描述符（`visible-string`）
     - `A3`：版本信息（`structure`：6 项 `visible-string`）
     - `A4`：生产日期（`date_time_s` 7 字节）
     - `A5`：设备列表 / 规约列表（`array` of `visible-string`）
   - 提供方法分派器 `dcu_698_devmgr_call(method_id, ops, ctx)`（注入软件初始化端口 `dcu_devmgr_ops_t`：`data_init`、`event_init`、`demand_init`）：
     - `M3`（数据初始化）：调用 `ops->data_init(ctx)`（未注入则返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH` DAR 5）。
     - `M5`（事件初始化）：调用 `ops->event_init(ctx)`（未注入则返回 DAR 5）。
     - `M6`（需量初始化）：调用 `ops->demand_init(ctx)`（未注入则返回 DAR 5）。
     - `M1`（设备复位）与 `M4`（恢复出厂）：无硬件/平台复位端口时诚实返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5），绝不伪造成功。

## 2. 不变量
1. **零动态内存**：全部使用固定静态存储（通过 `check_no_dynamic_memory.py`）。
2. **诚实 DAR 契约**：`M1`/`M4` 绝不返回 0 假装重启或恢复出厂；`M3`/`M5`/`M6` 未注入回调时返回 DAR 5。
3. **缓冲边界**：`cap` 不足时编码器返回 `-1`，不越界写。

## 3. 可观测输出
- `dcu_698_devmgr_encode_attr` 对 `A2`/`A3`/`A4`/`A5` 输出合法 A-XDR 字节流，对 `cap == 0` 或未知属性返回 `-1`/`EDGE_SLAVE_SRC_UNDEFINED`。
- 注入 mock `ops` 时 `M3`/`M5`/`M6` 准确触发对应回调且不误触发其他回调；`M1`/`M4` 始终返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`。

## 4. 验收判据
1. `tests/test_dcu_698_devmgr.c` 覆盖 `A2..A5` 编码、边界 `cap`、`M3/M5/M6` 注入回调、`M1/M4` 诚实 DAR 5，并接入 `tests/CMakeLists.txt`。
2. `cmd/dcu/CMakeLists.txt` 编译 `dcu_698_devmgr.c`。
3. Clean Release build 0 warning，`ctest -R 'dcu_698_devmgr'` 全绿，ASAN 下 0 错误，`check_no_dynamic_memory.py` 通过。

## 5. 非目标
- 不在本单修改 `cmd/dcu/dcu_698_object_model.c`（留待下一批接线）。
- 不实现硬件看门狗复位或出厂镜像回滚。
