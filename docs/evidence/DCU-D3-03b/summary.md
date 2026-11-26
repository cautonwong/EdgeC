# DCU-D3-03b：Class 9 属性 3 完整语义

状态：交 `dcu-verifier` 复核。基线 `main 7f227146`，分支 `dcu/D3-03b-attr3`。

## 1. 标准依据（docs/698_parsed.md 4190–4218、4224–4236）

属性 3 每项 `structure {冻结周期 long-unsigned, 关联对象属性描述符 OAD, 存储深度 long-unsigned}`。
周期单位随 OI，基准 2000-01-01 00:00；周期 >0 自动冻结，=0 只由外部条件或“触发一次冻结”触发；
OAD 在表内唯一；每种方案可有不同周期与深度；存储深度 = 最少保留记录条数。方法 3 参数为
long-unsigned 延时秒（0 = 立即）。

## 2. 数据形状与实现

- 调度按 (OI, 属性 3 条目) 分开：`last_fire[8][8]`；每个 tick 对每个 `period>0` 条目用
  `_oi_boundary(oi, now, local, period)` 算自己的边界，边界推进才冻结该条目的 OAD。
  同一 OI 的两个 OAD 可各自有周期，不再共用 `items[0].period`。
- 延时触发：`dcu_freeze_pending_t pending[DCU_FREEZE_PENDING_MAX=8]`（无动态内存，容量 8 写进
  本证据；池满回 DAR 2）。方法 3 延时 >0 只入池；`dcu_freeze_tick` 先处理到期项再走自动边界；
  `dcu_node` 的下一次 uv 定时器取「下一整秒」与「最早 pending 到期」的较小者，不新开线程。
- 深度：`dcu_storage_trim_freeze(storage, addr, freeze_oad, src_oad, keep)` 在同一写入事务内
  按 `(meter_address, freeze_oad, source_oad)` 只保留最新 keep 条（`ORDER BY freeze_time DESC`）。
  `depth=0` 按 1 处理：标准只说“最少保留条数”，0 没有下限，产品选 1 防无界增长（本证据记录）。
- 上限：`DCU_FREEZE_ENTRY_MAX=8`。方法 4/7 在事务内对“现有条目 + 新增唯一 OAD”计数，超过 8
  整单 ROLLBACK 并回 `EDGE_SLAVE_SRC_DENIED`（DAR 3）。DAR 8（越界）在 foundation 头无映射且
  `edge_slave_plane.h` 不在 core_readonly_whitelist，按派发要求不扩权。方法 7 一帧 >8 项在解码
  即回 DAR 3（形状合法、容量拒绝），不进入事务。
- OAD 唯一性：方法 4/7 对已存在 OAD 走 upsert（幂等，周期/深度更新、顺序不变），有测试钉住。
- 加载：`_load_rows` 保留前 8 条，只要实际丢行就把 `cfg->truncated` 置位并 `LOG_WARN`
  （恰 9 行也告警；8 行不告警）。测试断言 9 行 → truncated=true/count=8，8 行 → false。

## 3. 完成定义逐项

| 缺口 | 实现 | 证据 |
| --- | --- | --- |
| 每项独立周期 | 每 (OI, 条目) 边界与 last_fire | `test_freeze_per_entry_periods`：00:03 只写周期 1 的电能，00:04 才写周期 2 的电压 |
| 方法 3 延时 | pending 池 + 定时器最短到期 | `test_freeze_delayed_trigger`：+1s 不写、+2s 写一次、重复 tick 不重复；delay=0 立即 |
| 深度裁剪 | 同事务按 (表, OI, 源) 保留最新 N | `test_freeze_depth_trim`：depth=2 连写 3 次只剩最新 2 条；depth=1 源只剩 1 条；第二块表互不影响 |
| 第 9 项拒绝 | 事务内计数（本帧重复 OAD 去重），回滚 + DAR 3 | `test_freeze_attr3_capacity_dar`：第 9 项 DENIED，重载仍 8；方法 7 九项 DENIED；历史第 9 行 → truncated=true 且 count=8（8 行时 false）；现有 6 项 + 同帧 3 项含重复 OAD → 成功到 8 |
| 延时写失败重试 | pending 失败保留并限速 WARN | `test_freeze_pending_retries_after_failure`：storage=NULL 时 tick 保留 pending 不写；恢复 storage 后到期写出并清 used |
| OAD 唯一 | upsert 幂等 | 同测试：重复添加现有 OAD 后 count 仍 8，period 更新为 5 |

## 4. 测试原文

见 `tests.log`，含 `tests/test_dcu_698_freeze.c` 九个用例的全量输出；新增四个行为级用例：

- `test_freeze_per_entry_periods`：同一 OI 5002 上电能周期 1、电压周期 2；假时钟 prime 于
  00:02:59；00:03 tick 只写电能，00:04 tick 写电能+电压；逐源计数断言。
- `test_freeze_delayed_trigger`：方法 3 延时 2 秒；prime 后 +1 秒无记录、+2 秒三条记录、
  同 tick 重入不重复；delay=0 立即写一条。
- `test_freeze_depth_trim`：两块表、电能 depth 2、电压 depth 1；连续 3 个秒边界后每表电能
  2 条（最新两个时间戳）、电压 1 条；两表互不影响。
- `test_freeze_attr3_capacity_dar`：默认 3 项 + 5 个新 OAD = 8；第 9 项返回 DENIED 且 DB
  重载仍 8；9 项批量解码 DENIED；历史第 9 行 → truncated=true、count=8（8 行时 false）；
  5007 上方法 4 加到 6 项后，方法 7 发 3 项且两项同 OAD → 返回 0、count=8、
  重载仍 8 且 truncated=false；upsert 幂等。
- `test_freeze_pending_retries_after_failure`：storage=NULL 时到期触发失败，pending 保留、
  不写记录；恢复 storage 后下一次 tick 写出并清 used。

## 5. 门禁与命令

- clean Release build：`check_build_warnings.py` 0 新告警。
- targeted：`ctest -R 'dcu|freeze'` 35/35；单测 10/10；ASAN `detect_leaks=1` 3/3 零泄漏。
- `run_fanalyzer.py --build-dir build`：3 known / 0 new。
- `check_single_responsibility.py` 干净；`run_guard_selftest.py` 108/108（main 基线已从 105 升到 108）。
- 验收：`acceptance/run.sh dcu` 2/2，3 + 33 passed（现有 000032/000033 保持绿）。

## 6. 取舍与拒绝的替代

- 每 OAD 一份调度器：拒绝，会新造第二套调度；改为同一调度器内按条目记录边界。
- pending 用动态队列/线程：拒绝，违反无动态内存与“不新开线程”；固定 8 槽，池满 DAR 2。
- 深度用应用层查询再删：拒绝，会跨事务且与插入竞争；放进 storage 的同一事务。
- 第 9 项回 DAR 8：拒绝，foundation 头无映射且保护头不在白名单；按派发用 DAR 3。
- 深度 0 回 0 条：拒绝，标准只定义“最少保留”，0 条会丢刚写入的记录；按 1 处理并记录。

## 7. 未决项

- pending 池容量 8 为产品常量；更大并发延时触发需产品拍板（池满是 DAR 2，不静默丢）。
- 深度裁剪按 (表, OI, 源) 保留；标准“每方案不同深度”已按条目生效，但同一 OAD 多方案
  （属性特征 0..7）仍未建模。
- 属性 3 仍无 RSD（标准只对属性 2 记录定义 RSD）。
- 历史超限行只诊断不迁移；需要时另立数据修复任务。
