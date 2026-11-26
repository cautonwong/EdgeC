# DCU-D3-03：Class 9 属性 3 配置与方法

状态：切片 1 落地，交 `dcu-verifier` 复核。基线 `feat/dcu ce106278`，分支
`dcu/D3-03-methods`。

## 1. 标准依据

`docs/698_parsed.md` 4190–4215（表 149/150）：

- 属性 3 = `array 一个关联的对象属性∷=structure { 冻结周期 long-unsigned, 关联对象属性描述符 OAD,
  存储深度 long-unsigned }`。周期单位随 OI（秒/分/时/日/月/年/结算日），以 2000-01-01 00:00
  为基准；周期 >0 自动冻结，=0 非自动、由方法 3 或外部事件触发；结算日冻结在月结算动作时发生。
- 方法 3 参数 = long-unsigned 延时秒（0 = 立即）；方法 4 参数 = 冻结对象 structure；
  方法 5 参数 = OAD；方法 7 参数 = array 冻结对象；方法 8 参数 = NULL。
- 属性 2 的冻结记录列 = 属性 3 的 OAD 列表；本单让调度器写记录时直接消费该列表。

## 2. 数据形状与归属

- 新增 `dcu_freeze_cfg_t`（每 OI 至多 8 项，调用方分配，静态缓存无动态内存）：
  `{ oad, period, depth }`；`dcu_freeze_cfgs_t` 持有 5000–5007 八个 OI，嵌在 `dcu_node` 组合根。
- 持久化复用现有 SQLite 连接（不新造存储层）：
  - `dcu_freeze_attr3(oi, seq, oad, period, depth)`，PK `(oi, oad)`；`seq` 保属性 3 数组顺序，
    同 OAD 再添加 = 按 PK upsert（周期/深度更新，顺序不变）。
  - `dcu_freeze_attr3_meta(oi, configured)`：区分“主站没配过”与“主站清空过”。没有 meta
    或未配置 → 默认集（周期 1、三源 `{有功电能, 电压, 电流}`、深度 1）；configured 但零行 →
    属性 3 就是空数组，不再回落默认。
- 默认集只存在于未配置分支；主站第一次方法 4/5/7 时先把默认集物化成显式行，再施加变更
  （`_materialize_defaults`），所以“删掉一个默认 OAD”后重开不会又被默认集补回。
- 内容在每次变更后从 SQLite 重载该 OI，缓存与库单一来源；写操作在 `dcu_storage_begin_trans`
  事务内完成。

## 3. 方法解码与 DAR

`dcu_698_freeze_decode_op` 用 `dlt698_decode_data` 严格解 A-XDR，并只接受“末尾无字节或单个
TimeTag(0)”；形状错回 DAR 7。`dcu_698_freeze_execute` 按对象方法分派：

- 方法 3：延时 0 立即用当前时钟调 `dcu_freeze_trigger` 写该 OI；延时 >0 未实现 → DAR 5。
- 方法 4/7：structure/array<structure> 逐项 upsert；方法 5：按 OAD 删除（不存在 → DAR 6）；
  方法 8：清空并置 configured。
- ACL：5000–5007 属性 `{2 只读, 3 只读}`，方法 `{3,4,5,7,8}`；5008–5011 仍不进 ACL。
- 方法 3 延时 >0 是标准合法参数但产品未实现 → DAR 5；未绑定存储/调度器是初始化故障 →
  DAR 2；两者都有单测断言具体返回码，不靠注释。
- 写路径事务：任一失败（默认集物化、upsert、删除、清空）都回滚，不留下部分默认行/修改；
  成功路径先在事务内把新快照载到临时对象，提交成功后才替换缓存，重载失败回滚且缓存不动。

## 4. 调度器消费属性 3

- `dcu_freeze_scheduler_set_configs` 注入配置快照；`_fire` 按该 OI 的 OAD 列表逐项读 DataHub
  影子并写 `dcu_freeze_history`；清空后的 OI 不写记录。
- 周期：`_oi_boundary` 以 2000-01-01 00:00 本地基准 + OI 单位计算；5001/2/3 按
  `base + (now-base)/span*span`，5004 每 N 天、5006 每 N 月、5007 每 N 年、5005 每月 1 日；
  周期 0 → 无自动边界，只由方法 3/外部触发。未配置时沿用周期 1 的 D3-02 缺省。
- 简化（证据缺口）：一个 OI 只取属性 3 第一项的周期作为实例节奏；标准允许每个 OAD 不同周期/
  深度（不同冻结方案）。深度只持久化/广告，未做保留条数裁剪。

## 5. 覆盖与证据

- 单测 `test_freeze_attr3_rollback_and_dar`：删除不存在的 OAD 时默认集物化整体回滚（表无部分行、
  cfg 仍 `configured=0`）；方法 3 延时 >0 断言 `CLASS_MISMATCH`（DAR 5）；未绑定断言
  `TEMPORARY`（DAR 2）。
- 单测 `tests/test_dcu_698_freeze.c::test_freeze_attr3_config_drives_records`：
  默认集 3 项且 `configured=0`；A3 编码 2+3×13 字节；方法 5 删除电能后记录只剩 2 列且
  重载仍 2 项；方法 8 清空后重载 0 项；方法 7 装回三源 period=0 后自动冻结不写、方法 3
  立即写 3 行。
- 对象/ACL：5004 行 `attrs{3}`、`methods{3,4,5,7,8}`；ACL 60 项不变；`test_dcu_698_object_model`
  与 `test_dcu_698_axdr` 同步（方法 1 仍类定义未暴露 → DAR 5 改由类检查覆盖）。
- 验收 `D_00_00_02_000033`：测试时钟让 5001 每秒冻结；方法 5 删除 OAD 0x00100200 后，
  GetRecord `[时间,电压,电流]` 回行（列标签 0x1C/0x12/0x05），`[时间,电能]` 回 DAR 分支；
  最后方法 7 装回默认三源。现有 31 条 + D3-02 的 000032 保持绿，共 33 passed。

## 6. 门禁

- clean Release build：0 warning；单测 5/5；ctest `-R dcu` 35/35；ASAN `detect_leaks=1` 5/5，0 泄漏；
  fanalyzer 3 known / 0 new；SRP 干净；guard 105/105；验收 2/2（3 + 33）。
- 顺手修正 D3-02 summary §4 的旧计数为对象 66 / ACL 60 / cap 60。

## 7. 未决项（切片 2 / D3-03b）

1. 深度：`depth` 已持久化并可按 A3 读出，但冻结表未按深度保留/裁剪；需要定义“每 OAD 至少保留
   N 条”的删除时机与事务边界。
2. 每 OAD 独立周期/方案：当前一个 OI 用第一项周期；标准允许同一 OI 内不同 OAD 带不同周期与
   深度，需要把 `_fire` 拆成按方案分组。
3. 方法 3 延时 >0：需要一次性定时器（仍是同一调度器）或明确回 DAR 5；当前 DAR 5。
4. RSD：属性 2 的时间范围 RSD 已由 `dcu_698_record` 支持；属性 3 是普通 GET array，标准只对
   属性 2 记录定义 RSD，未新增假选择路径（D3-03b 不含此项）。
5. 5001 每秒写放大的 P6 缺口继承 D3-02，不在本单长跑。
