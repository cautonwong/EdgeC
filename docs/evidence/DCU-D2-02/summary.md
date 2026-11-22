# DCU-D2-02 Class 10 采集档案（6000/6001）

基线与分支：`dcu/D2-02-class10`，工作树 `/tmp/dcu-d2-02`，父提交 `f0ec96fe`。

## 目的

D1-18c 把 6000 方法整体下架，因为旧实现只改 RAM、重启即丢。本单按标准 A.7 在 `cmd/dcu` 内实现采集档案的增删改查与持久化，只重新广告已实现的方法。

## 位 → 状态

| 位 | 广告 | 行为 |
| --- | --- | --- |
| 6000 A2 | 是 | `array 采集档案配置单元`；超过 255 只表或整组放不进 2048 字节 A-XDR 缓冲 → DAR 2 |
| 6000 方法 127 Add | 是 | 解码配置单元 → `dcu_inventory_add_unit` → 写 `meter_inventory`；重复/满 → DAR 3；落库失败 → DAR 2 且 RAM 回滚 |
| 6000 方法 128 AddBatch | 是 | 1–8 个单元，全部成功或全部回滚（RAM 与 SQLite 都回滚） |
| 6000 方法 129 Update(Basic) | 是 | 按配置序号改基本信息；地址变更同时改主键（先写新行再删旧行） |
| 6000 方法 130 Update(Ext/Annex) | 是 | 改扩展信息；附属信息非空 → DAR 2（没有存储路径） |
| 6000 方法 131 Delete(序号) | 是 | 先删库再删 RAM |
| 6000 方法 132 Delete(Basic) | 是 | 按地址删除 |
| 6000 方法 133 Delete(地址+端口) | 是 | 按地址删除（端口用于定位，产品档案以地址为主键） |
| 6000 方法 134 Clear | 是 | 清空库与 RAM |
| 6000 方法 1–5 | 否 | Class 10 定义了 1–3：DAR 5；4/5 未定义：DAR 4 |
| 6001 A2 | 是 | 单个配置单元结构；元素索引 = 配置序号，0 = 第一只；不存在 → DAR 6 |
| 6001 其它属性 | 否 | 类定义过的回 DAR 5，未定义回 DAR 4 |

配置序号是设备分配的身份：加载时从 1 起补号（0 保留给“未分配”），持久化在 `meter_inventory.config_seq`。

## 持久化路径

- `router_sink_ops_t` 增加 `archive_save/delete/clear` 三个端口；`dcu_node` 用 `dcu_storage_t.db` 实现，复用既有单连接，不另开 SQLite。
- `dcu_inventory` 负责 SQL（与既有 `load_from_db` 同源）；`meter_inventory` 通过 `ALTER TABLE ... ADD COLUMN` 增量补列，旧库可直接升级。
- 写入顺序：先落库、后改 RAM；失败不改 RAM。Update 的地址变更先写新主键再删旧行。

## 已知限制

- 6000 A2 在 40 只表的台架上超过 A-XDR 缓冲（2048B），按 DAR 2 明确失败；验收用 6001 A2 验证加表/删表。
- 130 的附属信息（Annex 属性值数组）没有存储模型，非空回 DAR 2。
- 133 按地址删除；端口只参与定位语义，档案主键仍是地址。

## 测试

- `tests/test_dcu_698_archive.c`：加表→落库→重装载字段；删表/清空；128 批量的全成或全不成；129/132/133；无持久化端口时 127 回 DAR 2；127 参数 A-XDR 解码；加载补号；6000 A2 编码含新增单元。
- `tests/test_dcu_698_object_model.c`：6000/6001 mask 与 DAR 分类。
- `acceptance/dcu/tests/test_dcu_northbound.py`：6000 A2 超缓冲 DAR 2；`D_00_00_02_000018` 加表→6001 A2 读回→删表→DAR 6。

## 门禁

（见 tests.log / runtime.log）
