# DCU-D3-02 范围评估：Class 9 冻结调度器

状态：评估稿，未写实现。基线 `feat/dcu 1f065248`，分支 `dcu/D3-02-freeze`。

## 1. 标准与目录事实

- Class 9 共 13 个 OI：5000 瞬时、5001 秒、5002 分钟、5003 小时、5004 日、5005 结算日、5006 月、5007 年、5008 时区表切换、5009 日时段表切换、500A 费率电价切换、500B 阶梯切换、5011 阶梯结算。
- 属性：1 逻辑名、2 冻结数据表、3 关联对象属性表。方法：1 复位、2 执行、3 触发一次冻结、4/5/7/8 关联对象属性的增删/批增/清空。
- **标准属性 3 含周期与深度**（`docs/698_parsed.md` 4190–4215）：属性 3 是
  `array 一个关联的对象属性∷=structure { 冻结周期 long-unsigned, 关联对象属性描述符 OAD, 存储深度 long-unsigned }`。
  周期单位由 OI 决定（秒/分/时/日/月/年/结算日）；周期 >0 自动冻结、=0 非自动（方法 3 等外部触发）；
  基准 2000-01-01 00:00；结算日冻结在每次月结算动作时发生，切换冻结在切换动作时发生。`prd.md` 的
  周期/深度描述与标准一致，早先评估把它误读成“标准没有周期/深度、属性 3 只是 array OAD”，此处更正。
- `docs/698_parsed.md` 表 A.6 是唯一形状依据；设计文档 `DCU_698_OBJECT_MODEL.md` §Class 9 只承诺“冻结记录落 `dcu_freeze_record_t`、查询走 `dcu_storage_query_freeze`、方法 3 触发一次冻结”，**没有**调度周期、零点对齐、结算日、切换类触发、质量标记的设计。

## 2. 现有可复用实现

- `dcu_storage`：`dcu_freeze_history(meter_address, freeze_oad, freeze_time, source_oad, frozen_value)`，PK `(meter, freeze_oad, freeze_time, source_oad)`；`dcu_storage_write_freeze` / `dcu_storage_query_freeze` / `dcu_storage_count_freezes` 已有。
- `dcu_698_record.c`：5004 的 GET.Record（RSD 时间范围 + RCSD 列）已实现，但硬编码 `OI_DAILY_FREEZE (0x5004)`；5005 等其他 OI 未接。
- `dcu_event_monitor.c`：演示版“日冻结”管线——收到 tag==`OAD_DAILY_FREEZE` 的触发后，从 DataHub 影子抓 `OAD_ACTIVE_ENERGY`/`OAD_VOLTAGE`/`OAD_CURRENT` 三个固定 OAD 写 freeze 表；触发来自 `dcu_heartbeat.c` 演示模式（每 5 秒、exit 前自杀）。生产模式不冻结。
- 对象模型/ACL：只有 5004 一行（`attr 2` 只读），其余 12 个 OI 没有对象行；对象数 54、ACL 53、cap 53。
- 时钟：`dcu_clock_port_system`（`time(NULL)`/localtime）；D3-01 的 `edge_time` 未合并，时间质量分级缺口按夜间授权记录，不伪造。

## 3. 硬缺口（按 §9.5 / 夜间授权停）

1. **ACTION 8010 没有标准依据**：`docs/698_parsed.md` 与 `DCU_698_OI_CATALOG.md` 中 “8010” 各出现 0 次；Class 13 目录块的 OI 是 8000、8103–8108，另有 8100–8110 的行归在 Class 8/13 混淆区，但都没有 8010。计划 D1-06/D3-02 的“8010 强制冻结”在仓库内找不到表 A.9 依据 → §9.5 #2，停止该子项。替代路径只能是 Class 9 方法 3（标准触发一次冻结，归 D3-03），或由 operator/ADR 给出产品的 8010 定义。
2. **属性 3 的主站可配周期/深度尚未实现**：标准字段存在（见 §1），但产品还没有属性 3 的持久化与方法 4/5/7/8（D3-03 范围），`DCU_698_OBJECT_MODEL.md` 也未覆盖配置语义。
   本单以标准默认节奏（周期=1）占位并写清边界：**D3-03 实现属性 3 后，调度器必须改为消费属性 3 的配置，
   硬编码默认节奏不是最终语义**。
3. **切换类 OI（5008/5009/500A/500B/5011）没有触发源**：产品没有时区表/日时段/费率/阶梯切换事件；可挂对象行，但不能假装有触发。
4. D3-01 未合并：没有时间质量分级；UNTRUSTED 质量标记无法实现，只记录缺口。

## 4. 数据形状提议（待拍板）

- 调度器：`dcu_node` 组合根内嵌 `dcu_freeze_scheduler_t`（调用方分配），一个 `uv_timer` 对齐到下一个边界；触发时用边界墙钟（不是定时器实际回调时刻）作 `freeze_time`，并记录实际回调与边界的差值供 <50ms 证据。不新造线程/存储。
- 记录：沿用 `dcu_freeze_history`；`freeze_oad = 0x500X0200`；`source_oad` 取该 OI 的关联对象集合；值在触发瞬间从 DataHub 影子按表槽位读取（`datahub_read_shadow_device`，与现有事件监视器同路径）。
- 关联 OAD 集合：当前产品只有演示版的三个 OAD；标准允许主站用方法 4/5/7/8 配置，但那是 D3-03 的范围。D3-02 需要一个决定：(a) 产品默认集合（固定，证据写明）；(b) 新建配置表（属性 3 的持久化，D3-03 再开方法）；(c) 等 D3-03。
- 周期/深度：标准给出默认节奏（周期=1）：5004 每日 00:00、5006 每月 1 日 00:00、5002 每分钟、5003 每小时、5005 每月 1 日 00:00 结算上月、5007 每年 1 月 1 日 00:00、5000 仅手动/补冻；深度固定。属性 3 的主站可配周期/深度与方法 4/5/7/8 归 D3-03，本单不实现配置，也不把默认节奏当最终语义。
- 5005 结算日：标准语义是“每次月结算动作时冻结”；产品默认月结算在每月 1 日 00:00，即结算上月。
- GET.Record：把 `dcu_698_record.c` 的硬编码 5004 改为按 OI 映射 `freeze_oad`（5004/5005/5006 等），其余查询逻辑复用。
- 对象/ACL：13 个 OI 全进对象与 ACL（对象 54→66，ACL 53→60，cap 53→60），attr 2/3 只读；未实现的 attr/方法按 DAR 2/4/5 诚实返回，不广告不可用方法。

## 5. §9.5 核对

- #2 标准依据不足：ACTION 8010 无标准/目录来源 → 停。
- 夜间授权外的设计决策：属性 3 的周期/深度配置与关联 OAD 默认当时无已提交设计 → 停；后续裁决为“标准默认节奏占位、attr 3 归 D3-03”。
- 不改受保护头/公共组件/ADR；不新造调度器/存储；D3-01 缺口按授权记录。
- D3-05b 不碰。

## 6. 建议切片（拍板后）

1. 切片 A（无新决策）：13 个对象/ACL 行 + 5004 GET.Record 改为按 OI 取 `freeze_oad` + 未实现属性 DAR；对象/ACL 计数与测试。
2. 切片 B：调度器 + 默认周期/深度/关联 OAD（按 §4 的选择），日/月/曲线 + 零对齐 + 可注入假时钟测试 + 验收跨零点读 5004/5005。
3. 8010：仅当 verifier/operator 给出标准/产品定义后另立；否则关闭并写进计划风险。

结论：本单在 8010、周期/深度/结算日、关联 OAD 默认四处触发停止条件，未写实现。请就 §4 的四项给出裁决，我再开切片。

## 7. 切片 A 阻塞：ACL 65 项超出驱动会话固定大小（停下报告）

裁决要求 13 个 Class 9 OI 全进对象/ACL（54→67、53→66、cap 53→66）。实际实现时 cap 升到 65 后，`adapters/io_drivers/src/driver_dlt698.c:316` 的 `_Static_assert(sizeof(dlt698_session_t) <= 1024, "DLT698 session exceeds DRIVER_PRIV_SIZE")` 失败：

- `dlt698_acl_object_t` = `{u16 oi, u8 n_attrs, u8 n_methods, u32 attr_rights, u8 method_ids[8]}` = 16 字节；会话其余字段约 40 字节；1024 字节上限对应 ACL 对象上限约 61。当前 cap 53 可过，65 必炸。
- `DRIVER_PRIV_SIZE` 在 `adapters/io_drivers/src/driver_manager.c` 定义为 1024，会话缓冲被驱动引擎按此分配；这不是 cmd/dcu 内可改的常量。

可选修复（都需要 cmd/dcu 之外的改动，按夜间授权/§9.5 停下报告）：
1. 把 `DRIVER_PRIV_SIZE` 与驱动会话断言从 1024 提到 2048（adapters/io_drivers，影响每驱动实例的会话内存；需要 verifier/架构批准）。
2. 压缩 ACL 对象表示（如方法改位图）以在 1024 内放下 65 项（adapters 改动 + 兼容性核对）。
3. 只把时钟驱动的 5000–5007（8 个，含已有 5004）放进 ACL，切换类 5008/5009/500A/500B/5011 不进 ACL 但在对象模型保留（偏离“13 全进 ACL”的裁决）。

当前工作树已回滚到评估 commit 6974d0d1，未提交任何实现。请裁决修复路径，我再继续切片 A/B。

## 8. 切片 A 落地（裁决 c）

- 对象模型：13 个 Class 9 OI 全进（对象 54→66）；5004 已存在，实际 +12。
- ACL：只进时钟驱动的 5000–5007（8 个，5004 已存在，实际 +7）：ACL 53→60、`EDGE_SLAVE_ACL_MAX_OBJECTS` 53u→60u。5008/5009/500A/500B/5011 留对象模型、不进 Class 20 列表；主站直接访问按类/属性定义回 DAR 5/4，不假装支持。原因双写进证据：无切换触发源；65 项超 `DRIVER_PRIV_SIZE` 1024（16 字节/对象，上限约 61）。后续若产品批准 1024→2048 且有触发源，再进 ACL。
- GET.Record：`dcu_698_record.c` 从硬编码 5004 改为 `OI→freeze_oad = (OI<<16)|0x0200`；5000–5007 走真实查询，切换类回 DAR 2，非 Class 9 回 DAR 6。
- 测试：对象 66 / ACL 60 / driver 列表 60、over-cap 61；对象列表含 5000/5005、不含 5011；新 `test_record_oi_maps_freeze_oad`（5004/5005 各一行、5008 DAR 2、6000 DAR 6）。
- 验证：受影响 ctest 20/20；ASAN detect_leaks=1：scheme_mgmt 21 / rules 10 / schemes 12 / node 3 / poller 9 / polling_loop 2，0 泄漏；验收 2/2（3+31）；SRP 干净；guard 105/105；warning 0；fanalyzer 3 known/0 new。

## 9. 切片 B 落地：Class 9 冻结调度器

- `cmd/dcu/dcu_698_freeze.{h,c}`：单 uv_timer（1000ms 节拍），按标准 OI 节奏计算边界——5001 秒、5002 分、5003 时、5004 本地零点、5005 每月 1 日 00:00 结算上月、5006 每月 1 日 00:00、5007 每年 1 月 1 日 00:00；5000 无时钟边界（方法 3 归 D3-03），5008–5011 不触发。首次 tick 只对齐不回补历史。
- 记录写现有 `dcu_freeze_history`：`freeze_oad=(OI<<16)|0x0200`，`freeze_time` = **边界墙钟**（不是回调时刻），源 OAD 取产品默认集 `{OAD_ACTIVE_ENERGY, OAD_VOLTAGE, OAD_CURRENT}`，值从 DataHub 影子按 `slot_id` 读取；整次冻结在一个事务内。
- 绑定在 `dcu_node` 组合根（内嵌调度器 + 定时器），停止/销毁关闭定时器；生产模式（未设 `DCU_DEMO_MODE`）启用。演示台架保留心跳冻结管线：真实秒/分/时冻结会把 60 秒台架生命周期压垮（实测验收超时），故 demo 下不启用标准调度器。
- 时间质量：D3-01 `edge_time` 未合并，未做 UNTRUSTED 质量标记（缺口记录）。
- 测试 `tests/test_dcu_698_freeze.c`：假时钟 2026-11-29 23:59:59 首次 tick 不回补；11-30 00:00 触发 5001/2/3/4，12-01 00:00 触发 5001/2/3/4/5（结算 11 月）/6，01-01 触发 5001–5007（含 5005 结算 12 月）；每次 `freeze_time` 精确等于边界（边界+1 查不到）；同一秒重入不重复；切换类永不产生记录。6 个 OI × 1 表 × 2 源的行数逐项断言。
- 实测 DAR 修正：切换类 5008/5011 的 GET A2 走 resolve 实际返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（-21 → DAR 5），不是 DAR 6；证据改为实测值。

验证（最终树）：受影响 ctest 21/21（含 freeze 1）；ASAN detect_leaks=1：freeze 1 / scheme_mgmt 21 / rules 10 / schemes 12 / node 3 / poller 9 / polling_loop 2，0 泄漏；验收 2/2（3+31）；SRP 干净；guard 105/105；warning 0（clean Ninja Release）；fanalyzer 3 known / 0 new。

端到端缺口（按授权记录）：产品级“模拟时钟跨零点”需要时钟注入 seam（测试模式设计，未在本单新增）；当前跨零点由假时钟单测证明，验收因 demo 关闭调度器而不覆盖调度器写记录。若 verifier 要求产品级证明，需要批准一个 env/测试时钟 seam 或独立的时钟注入设计。

## 10. D3-02 B2：零点对齐、本地时区边界、产品级跨零点验收

### 10.1 定时器改为按边界排程

- `dcu_node.c` 不再用固定 1000ms 周期：回调里取当前 ms 时间，`dcu_freeze_tick` 后由
  `dcu_freeze_next_boundary` 返回下一个整秒边界，`uv_timer_start` 用
  `boundary*1000 - now_ms` 的 ms 延迟单次排程（clamp [1, 3600000] ms）。5001 的秒边界
  因此对齐到整秒，而不是漂移的 1s 节拍。
- 每次实际触发记录 `last_align_delta_ms = now_ms - boundary*1000`，`LOG_INFO` 输出
  `DCU freeze boundary=<epoch> align_delta_ms=<n>`；测试时钟下同时写
  `EDGEC_DATA_DIR/freeze_align.txt`。跨零点验收实测 `delta_ms=0`（<50ms）。

### 10.2 测试时钟 seam（仅 demo/测试）

- demo 模式启动时只跑 500ms 探针；`EDGEC_DATA_DIR/freeze_clock.txt` 出现后，组合根启用
  虚拟时钟：`base_ms + (uv_hrtime - t0)/1e6`，文件删除即退出并清 `primed`，台架不会被
  1 秒冻结持续压垮。生产未设控制文件时始终 `dcu_clock_port_system`/CLOCK_REALTIME。
- 现有 31 条北向用例不写控制文件，调度器保持关闭；验收新增第 32 条
  `D_00_00_02_000032`：控制时钟 2026-11-30 23:59:59.800 起动（首 tick 不回补），
  推到 2026-12-01 00:00:00；GetRecord 5004/5005 各回 1 行，`freeze_time` 等于零点，
  列集 = 冻结时间 + 默认源 `{0x00100200 有功电能, 0x20000200 电压, 0x20010200 电流}`；
  5008 返回 DAR 且 0 行；`freeze_align.txt` 断言 `delta_ms < 50`。

### 10.3 本地时区边界与结算日语义修正

- 5002/5003 原来按 UTC 取模；改为本地 tm：`now - tm_sec`、`now - tm_min*60 - tm_sec`。
  非整小时时区（IST +5:30）下 UTC 取模会把 00:00 错成 23:30，新增
  `test_freeze_hour_boundary_uses_local_time`（`TZ=Asia/Kolkata`）钉住本地零点。
- 5005 语义修正为“每月 1 日 00:00 结算上月”（边界在结算点，而不是上月最后一天的
  00:00），与验收“23:59:59→00:00:01 后 5004/5005 都有零点记录”一致；单测改为
  12-01、01-01 各写 5005，11-30 不写。

### 10.4 验证（最终树）

- 单测 `test_dcu_698_freeze` 3/3（本地边界、下一整秒、跨日月年结算）。
- 受影响 ctest（`-R dcu`，build-warn）：35/35；ASAN `detect_leaks=1`：
  freeze/scheme_mgmt/node 3/3，0 泄漏。
- 验收 `acceptance/run.sh dcu`：2/2 任务，northbound 32 passed（31 旧 + 1 新）；
  台架需先清残留产品壳与 `/tmp/rs485_*.sock`，否则会撞已有的设备级读数/6035 时窗抖动
  （与本单无关，干净重跑 32/32）。
- `check_build_warnings.py`：0；`run_fanalyzer.py`：3 known / 0 new；
  SRP 干净；`run_guard_selftest.py` 105/105。

### 10.5 已知性能缺口（P6，不在本单长跑）

5001 秒冻结按标准每秒写 `40 表 × 3 源 = 120` 行，长时间运行会造成持续写放大；
本单只用虚拟时钟跨 1 个零点、并只验证 2–3 秒，未做秒级长跑。生产启用 5001 前需要
批量/降采样或存储层合并策略，登记为 §12.5/P6 性能缺口。

### 10.6 与 D3-03 的边界（属性 3）

标准属性 3 的 `{冻结周期, OAD, 存储深度}` 是最终配置语义：周期 >0 按周期自动冻结（基准
2000-01-01 00:00，周期单位随 OI），=0 只由方法 3 等外部事件触发。本单调度器用“周期=1 的标准
默认节奏”占位，**不是最终语义**；D3-03 落地属性 3 的持久化与方法 4/5/7/8 后，调度器必须改为
读取属性 3 的周期/深度并据此排程，硬编码默认只能作为未配置时的缺省。

5008 GetRecord 实测（OAD `0x50080200`，4 列 RCSD）：应答 `85 03 01 50080200 04 <4×CSD>
00 03 00 00` —— `00` 是记录结果 DAR 分支，DAR=3（拒绝：5008 未进 ACL），无行数据；验收按该
真实布局断言，不把 DAR/行数位置弄反。
