# DCU-D1-18e 5004 冻结类的假值清理

基线 `32a89a54`（D1-18c/D1-18d/D0-11/D1-11 已合入），分支 `dcu/D1-18e-freeze`。

目标：Class 9（OI 5004 日冻结）不再对外宣称做不到的属性和方法。

## 依据

- 7.3.9 表148/149：属性 1 逻辑名、2 冻结数据表（array，每条含序号/时间/各关联数据）、3 关联对象表；方法 1 复位、2 执行、3 触发冻结、4/5 增删、7 批量添加、8 清除。
- 表150：方法 3 参数是 `long-unsigned 延时秒数`，0 立即执行。
- 7.3.10 表151/7.3.13 表160/7.3.21 表184/7.3.24 表195：接口类级方法集（用于 DAR 判定）。
- D1-07：记录数据走 GetRecord（存储查询），不依赖对象模型属性 2。

## 决策：属性 2 不暴露，GetRecord 是唯一记录路径

`dcu_698_acq_5004_read` 原来把 `count_freezes` 的条数当属性 2 返回，等于用条数冒充冻结数据数组。
本期不实现数组（需要把存储行编码成标准 `structure`，且 D1-07 已经提供记录读取），改为**不暴露 A2**：

- 对象表 5004 行删掉 `read` / `method` / `attr_mask` / `method_mask`。
- `k_scalars` 删掉 5004 A2 的标量条目。
- GET 5004 A2 现在走 `value_for_oad` 的 `_unadvertised_attr_rc`：Class 9 定义了 A2 → DAR 5。
- 记录数据仍由 D1-07 的 GetRecord 从冻结存储读出，回归测试 `test_dcu_698_record` 通过。

## DAR 规则

- 属性：D1-18c 已有 `dcu_698_class_defines_attr` + `_unadvertised_attr_rc`，本期沿用。
- 方法：rebase 到 D1-18d 后沿用其 `_class_defines_method`（接口类方法上限 + OI 8000/A.9 与
  OI 6000/A.7 的实例方法特判），只补了一个缺口：Class 9 表148 没有方法 6，原上限表会把它当已定义，
  已加 `class_id==9 && method_id==6` 的排除。`dcu_698_call` 的映射：
  - OI 不存在 → `EDGE_SLAVE_SRC_NOT_FOUND`（DAR 6，D1-18d 既有行为）；
  - 类/实例定义了该方法但对象没暴露 → `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）；
  - 类里没有 → `EDGE_SLAVE_SRC_UNDEFINED`（DAR 4）。
  5004 方法 3 属于类定义 → 5；方法 6 类里没有 → 4（见下）。

## 5004 位 -> 状态表

| 位 | 名称 | 状态 | 未暴露时 DAR |
| --- | --- | --- | --- |
| A1 | 逻辑名 | 未暴露（类定义） | 5 |
| A2 | 冻结数据表 | 未暴露（原为条数，已删） | 5；记录走 GetRecord |
| A3 | 关联对象表 | 未暴露（类定义） | 5 |
| A4… | 类里没有 | 未暴露 | 4 |
| M1 | 复位 | 未暴露（类定义） | 5 |
| M2 | 执行 | 未暴露（类定义） | 5 |
| M3 | 触发一次冻结 | 未暴露（类定义） | 5；见下"未实现" |
| M4/M5 | 增/删冻结对象 | 未暴露（类定义） | 5 |
| M6 | 类里没有 | 未暴露 | 4 |
| M7/M8 | 批量添加/清除关联表 | 未暴露（类定义） | 5 |

## 未实现：方法 3（触发一次冻结）及其它方法

方法 3 无法在本任务的文件范围内实现，原因是 ACTION 分派在**禁止修改**的
`cmd/dcu/dcu_north_adapter.c`：

```c
if (oi != OI_REMOTE_CONTROL) return EDGE_SLAVE_SRC_NOT_FOUND;   // 5004 方法到不了 dcu_698_call
```

它还要求把表150 的 `long-unsigned 延时秒数` 解出来传给对象模型，而 `dcu_698_call` 只有
`param` 指针没有长度；现有分派只解 OI 8000 的 A.9 参数。冻结引擎本身存在
（`OAD_DAILY_FREEZE` -> `stage_periodic_freeze`），但对象模型/聚合根不持有 stream engine，
要接也得新增触发注入。

因此：方法 3 与其余方法一律不暴露（不返回占位值），对象模型回 DAR 5。**生产 ACTION 目前
仍会在 `dcu_north_accept_action` 被拦成 DAR 6**，与属性路径的 DAR 5 不一致；要真正实现
方法 3 需要 north adapter 把非 8000 的 ACTION 转给 `dcu_698_call` 并传入延时参数。

## 测试

- `test_dcu_698_object_model`：5004 无属性/无方法；M3 → CLASS_MISMATCH，M6 → UNDEFINED；
  6000 M1/M2/M127 → 类不匹配（127 是 A.7 实例方法），M6/M200 → 未定义；6012 M128 → 未定义；
  事件 M2 → 类不匹配；未知 OI → NOT_FOUND。
- `test_dcu_698_axdr` / `test_dcu_698_value_table` / `test_dcu_advanced_modules`：
  5004 A2 不再解析成条数，回 CLASS_MISMATCH；ACTION 测试现在先装配对象模型（此前靠
  `dcu_698_find==NULL` 的 -1→DAR2 假通过）。
- `test_driver_dlt698`：GET 端口回 CLASS_MISMATCH/UNDEFINED 时，驱动映射成 DAR 5/4。
- `test_dcu_router`：6000 M127 → 未定义，M2/M3 → 类不匹配。
- `test_dcu_698_record`（D1-07 回归）：通过。

## 验收（`./test.sh --omnis dcu`，连跑两次）

- 平台要求型号必须有 `capabilities` 与 `point_table`。点表保留一个**标量** 3000 属性 4
  （最大事件记录数），给任务设备级读数（devices 1/1）。5004 A2 不进点表；电压/电流/功率是
  数组，平台标量点表驱动取不了，仍由直接发帧验证。
- `test_master_reads_freeze_count`（旧断言读回条数）替换为
  `test_master_gets_class_mismatch_for_freeze_array`：直接发 GET 5004 A2，断言 DAR=5。
- `test_master_reads_freeze_records`（D1-07 GetRecord 时间范围两行）保持不变。
- 两次运行都是 **2 个验收任务**（dcu-acceptance、dcu-northbound），各自退出码 0。
  rebase 到 D1-11 后 northbound **17 passed**（D1-11 的 SET/Class20/空记录用例 + 本任务的
  DAR 5、GetRecord、设备级读数三条共存），dcu-acceptance 3 passed。

## 门禁

见 `tests.log` / `comments.md` / `diffstat.txt`。
