# DCU-D2-04 普通采集方案（6014/6015）

基线与分支：`dcu/D2-04-schemes`，父提交 `1488eb4f`；实现分支 `dcu/D2-04-impl` 合入。

## 目的

方案此前只在 RAM 里、只播种方案 1，且方案字段与标准 6015 不对应。本单把 6014/6015 做成可增删改查、按 CSD 只采选定列、可持久化的普通采集方案。

## OI/方法/类型

| 位 | 类 | 类型 | 行为 |
| --- | --- | --- | --- |
| 6014 A2 | 11 | array<6015 结构> | 按方案编号升序返回全部方案 |
| 6014 M127 Add | | array<6015> | 按方案编号 upsert；CSD 变更对已被任务引用的方案 DAR 3 |
| 6014 M128 Delete | | array<unsigned> | 删除；被引用或不存在 DAR 6（整批） |
| 6014 M129 Clear | | NULL | 仅 NULL；仍有被引用方案时 DAR 3 |
| 6014 M130 Set_CSD | | {unsigned id, array CSD} | 重置列选择；被引用 DAR 3 |
| 6015 A2 | 8 | 6 字段结构 | 元素索引 = 方案编号（0 = 最小）；缺失 DAR 6 |
| 6020 其它 | | | 类定义未广告 5 / 未定义 4 |

6015 结构：{方案编号 unsigned, 存储深度 LU, 采集方式 {采集类型 unsigned, 采集内容 Data}, 记录列选择 array CSD, 电能表集合 MS, 存储时标选择 enum}。CSD 线格式 `5B 00 + 4 字节 OAD`；MS `5C + choice`。

## 已支持子集与拒绝

支持：采集类型 0（当前数据）+ 内容 NULL、存储深度 0、存储时标 0、MS = 全部用户地址、CSD 只含 OAD。
其余标准合法值一律 DAR 2 并逐字段记录：深度非 0、其它采集类型、非零时标、其它 MS、ROAD、未知 CSD OAD。H.5.4 的示例帧（深度 1、时标 1）因此在当前 profile 下回 DAR 2，验收使用等价简化帧。

## 持久化

`dcu_scheme_config` + `dcu_scheme_csd` + `dcu_scheme_meta` 建在既有 `storage->db`；DB 先写、RAM 后改、失败回滚；首次启动从轮询 tag 播种方案 1 一次。

## 门禁

（见 tests.log / runtime.log）
