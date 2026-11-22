# DCU-D1-18c 采集类属性与方法按标准收敛

基线与分支：`dcu/D1-18c-acq-shapes`，工作树 `/tmp/dcu-d1-18b`，父提交 `7687612d`（D1-19 合并后）。

## 问题

6000/6012/6002/6003 的 mask 把非标准语义当标准位暴露：6000 A2 是配置数组却回档案条数，方法 1/127 只改 RAM、128 被当成 Delete；6012 A2 是任务配置数组却回方案数、方法 1 被当成 AddScheme；6002 A2 是搜表结果数组却回状态、A3 才是条数；6003 是结果结构，不是带属性的对象。

## 位到状态

| OI | 位 | 标准（7.3.10/7.3.11、A.7） | 处理 | 线上 |
| --- | --- | --- | --- | --- |
| 6000 | A2 | 采集档案配置表（array） | 关闭 | DAR 5 |
| 6000 | 方法 127–134 | Add/AddBatch/Update/Delete/Clear | 全部关闭 | DAR 5；1–5、127、128 不再声明 |
| 6012 | A2 | 任务配置表（array） | 关闭 | DAR 5 |
| 6012 | 方法 1 | 复位 | 实现：清空方案表 | 0 |
| 6012 | 方法 127 | Add（任务配置单元） | 实现：既有 AddScheme 挪到 127 | 0/-1 |
| 6012 | 方法 128–130 | Delete/Clear/Update | 关闭 | DAR 5 |
| 6002 | A2 | 所有搜表结果（array） | 关闭 | DAR 5 |
| 6002 | A3 | 当前记录数（long-unsigned） | 实现：搜表结果条数 | 标量 0x12 |
| 6002 | 方法 127 | 实时启动搜表 | 保留，透传启动结果 | 0/-1 |
| 6003 | A2 | 一个搜表结果（structure） | 关闭 | DAR 5 |
| 5004 | — | 冻结记录（D1-07） | 不动 | 不变 |

方法 1 在 6000 上不再声明（A.7 给的是 127–134）；6000 的 Add/Delete/Clear 需要 `meter_inventory` 持久化与标准参数结构，登记为后续依赖。数组编码（A2）依赖 A-XDR 结构/数组通道（P0 第 10 项）。

## DAR 分级

沿用 D1-18b 的规则：类定义有、实例未声明 → DAR 5（`EDGE_SLAVE_SRC_CLASS_MISMATCH`）；类里没有的属性号 → DAR 4；未知 OI → DAR 6。6002 A3 的标量行加在 `dcu_698_var.c`，6000/6012 的旧标量行删除。

## 改动文件

- `cmd/dcu/dcu_698_class_acq.c`：删除 6000/6012-read/6003 处理器；6012 复位与 127 Add；6002 A3 条数与 127 返回码透传
- `cmd/dcu/dcu_698_object_model.c`：6000/6012/6002/6003 表行 mask
- `cmd/dcu/dcu_698_object_internal.h`：处理器原型同步
- `cmd/dcu/dcu_698_var.c`：删 6000/6012 标量行，加 6002 A3
- 测试：`test_dcu_698_object_model.c`、`test_dcu_698_value_table.c`、`test_dcu_698_axdr.c`、`test_dcu_router.c`、`test_dcu_advanced_modules.c`
- 验收：`acceptance/dcu/tests/test_dcu_northbound.py` 两条计数用例改为直接发帧验 DAR 5；`devices_north.yaml` 移除两个数组点

## 门禁

- 干净构建 + `check_build_warnings.py`：0 warning
- `ctest -j`：335/335 通过
- ASAN：8/8 相关目标通过
- `run_guard_selftest.py`：全部通过
- 验收 `test.sh --omnis dcu`：dcu-acceptance 3/3、dcu-northbound 9/9
- `git diff --check`：干净
