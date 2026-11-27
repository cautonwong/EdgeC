# P5-03d c1：策略分类表与键优先级

状态：2026-11-27，切片 c1。单一事实源是 `strategy-classification.tsv`；本文解释列与规则，`ci/scripts/check_iems_strategy_classification.py` 逐行校验。本切片不改变任何生产行为：所有行 `effective=module`，不切 wasm、不接 coordinator、不改 loader/converter/schema。

## 1. 来源

- ADR-001 §2.1/§2.2（`/home/edge/migration/adr-001-ems-strategy-app-vs-module.md:155-199`）：app 六条件、8 个 app 家族、6 类模块行。
- P5-03a CONTRACT §1.2（13 种 v3 策略形态）、§4.4（6 个策略类）、§4.7（仲裁顺序）（`/home/edge/migration/p5-03a-strategy-contract-2026-11-23.md`）。
- P5-03d ticket（`/home/edge/migration/p5-03d-classification-2026-11-27.md`）：分类字段、安全家族、app 配额、键优先级。
- P1-07a ticket（`/home/edge/migration/p1-07a-lc-user-cfg-2026-11-27.md`）：per-LC 文件三值（OK/ABSENT/ERROR）与 `lc_user_cfg` 覆盖 `lc_cfg` 的优先级来源。

## 2. 列

| 列 | 含义 |
| --- | --- |
| form | v3 形态 id，对应 CONTRACT §1.2 的一行；共 13 行 |
| kind | 该形态的唯一决策键，固定为 CONTRACT §1.2 的入口符号；门禁与 EXPECTED 逐行比对。同一 kind（trim 后）出现两次即判定“同一决策双源”；kind 含空白或不是 C 标识符 token 即失败 |
| class | 统一策略端口类，取自 CONTRACT §4.4 的 6 类 |
| adr_family | ADR §2.2 的家族；app 家族 8 个、模块家族 6 个 |
| target | 目标归属，`app`（条件性，等 P1-06/P1-07/安全签字/ARM p99）或 `module` |
| effective | 当前归属；c1 全部为 `module` |
| role | `DECIDE`（产生目标）/`LIMIT`（对 desired 做包络）/`OVERRIDE`（优先级覆盖） |
| period_ms | 决策周期；`0` 表示事件驱动。门禁按 CONTRACT §4.3 的上下限逐行审计（§4.3 无对应行时用 §1.2，如 EV 分配 2s、PCS 低 SOC 事件驱动） |
| decision_owner | 产生决策的实现单元；已实现者用现有符号，未实现者用 CONTRACT §4.4/§4.7 的计划符号。门禁按 EXPECTED 的固定符号审计 |
| execution_owner | 执行/限幅/设备命令的归属；门禁按 EXPECTED 的固定符号审计 |
| safety | `none` 或安全家族标签；非 `none` 的行只能 `target=module` |
| points_per_cycle | 每周期读+写点数上限估计，用于 app 配额检查 |
| events_per_s | 每应用事件率上限估计 |
| subscriptions | 订阅数上限估计 |
| notes | 来源与理由 |

`points_per_cycle`、`events_per_s`、`subscriptions` 是 ADR §2.1 第 2、6 条配额的机器可查形式。它们是上限估计，不是测量值；门禁只用它们做 ≤100 / ≤1000 / ≤32 的上界判定。

## 3. 13 种形态与 6 类

CONTRACT §1.2 的 13 行各自成一行，`form` 与 `kind` 均唯一。类映射沿用 CONTRACT §4.4 的 6 类：

| class | forms |
| --- | --- |
| TOU | tou_current_ac、tou_legacy_loop、pv_storage_self_use、pv_priority |
| DEMAND | demand_fixed、demand_dynamic |
| LIMIT | anti_reflux_limit |
| PF | pf_loop |
| SOC | soc_keep、pcs_low_soc_protect、standby_backup |
| EV | ev_capacity、ev_allocation |

CONTRACT §4.4 没有独立 PV 类，所以自发自用与光伏优先归经济（TOU）类；应急备电按备电 SOC 处理。这是本切片的分类决定，依据是 CONTRACT §1.2 中三者与 TOU 同执行路径或同属备电语义，不是从实现反推。

## 4. ADR §2.2 家族

app 家族 8 个：TOU、DEMAND_FIXED、DEMAND_DYNAMIC、ANTI_REFLUX、PV_PRIORITY、SOC_KEEP、PF、EV_CAPACITY。
模块家族 6 个：FIRE、PROTECTION、BMS_CLUSTER、PCS_COMMAND、LOOP_10MS、OVER_CAPACITY。

本表出现 10 个：8 个 app 家族各至少一行，加 PROTECTION（PCS 低 SOC）与 OVER_CAPACITY（EV 分配）。

未出现的 4 个模块家族（FIRE、BMS_CLUSTER、PCS_COMMAND、LOOP_10MS）在 EdgeC 是安全模块，不是 v3 策略形态，因此不占 13 行；它们仍受“安全家族只能 module”约束，门禁把它们列为已知但本表不要求的家族。

防逆流是一个边界：ADR 允许 app 做决策、硬限幅留模块。本表只有 `anti_reflux_limit` 一行，表示硬限幅形态；app 决策部分不是独立的 v3 形态，留在调用方策略内，c3 拆仲裁时再显式化。

## 5. 规则

- 安全家族（消防、保护、BMS 簇、PCS 命令、防逆流硬限幅、10ms 闭环）只能 `target=module`、`effective=module`，且 `safety` 必须是对应标签。
- app 目标行必须满足 `period_ms >= 100`、`points_per_cycle <= 100`、`events_per_s <= 1000`、`subscriptions <= 32`（ADR §2.1 第 1、2、6 条）。
- 每周期超过 100 点的形态（EV 分配）与事件驱动保护留模块，不因非安全而进 app。
- c1 不切换任何 `effective`；`target=app` 只是条件性目标。
- TOU/需量的唯一决策源是 `iems_td_evaluate`；coordinator 只保留输入快照、仲裁、包络、执行，不得成为第二个决策源（P5B 结论 + ticket）。门禁按 EXPECTED 的 pinned owner 覆盖全部 5 行（4 个 TOU/需量形态 + `pv_storage_self_use`），并禁止任何 `class=TOU/DEMAND` 的行把 coordinator 记为 decision_owner。
- role 顺序固定为 OVERRIDE → DECIDE（经济单选）→ LIMIT → 执行；与 CONTRACT §4.7 一致。

## 6. 键优先级（resolver 规则，本切片只实现纯函数）

`cmd/iems/iems_strategy_priority.{h,c}`：

- EMS：当前键 > 旧 deprecated 键 > ABSENT。
- per-LC：`lc_user_cfg` override > `lc_cfg` base > ABSENT。
- NULL 指针表示 ABSENT；`absent` 不等于 0，调用方必须看返回码，不能看 `value`。
- 两个键同时存在且值不同：`conflict=true`，高优先级键的值为结果，绝不平均、绝不合并。
- 旧键只要被提供就置 `deprecated_used=true`（即使当前键胜出），用于迁移告警；`source` 说明哪个键供值。
- 返回码只有 `OK`/`ABSENT`/`BAD_INPUT`；非有限值或 NULL 输出指针为 `BAD_INPUT`，输出被重置为安全零。
- `Pmlmax/Pset`（EMS）与 LC `PowerLimit` 永不合并：两个 resolver 参数不相通，代码层面无法把它们放进同一次解析。
- `price_class` 必须由计划提供方赋值；未赋值时 `price_valid=false`，不得猜价格。c1 不接 loader，没有可执行路径，门禁不检查；执行覆盖明确记为 c2 残余（接 P1-07a typed loader 时补用例）。

per-LC 文件三值沿用 P1-07a：文件缺失 = ABSENT；文件存在但坏 = ERROR（显式目录下拒启），不按 0 处理。

### 6.1 旧→新键映射

EMS（`config/schema/ems.schema.json` 的 `x-deprecated` 与 current 属性；converter 对旧键记录 `deprecated` 后丢弃，见 `tools/migration/convert_config.py:143-160`）：

| 旧键（deprecated） | 新键（current） |
| --- | --- |
| `pv_mode` | `PvMode` |
| `cab_power` | `CabPower` |
| `EnPcsRpComp` | `ENRPComp` |
| `SetPcsPowerFactor` | `SetPowerFactor` |
| `EnSocKeepMode` | `SocKeepModeEnable` |

resolver 按 (current, deprecated) 键对通用处理，上表是 P1-07a/converter 迁移留下的具体 EMS 键名；旧键只要被提供，`deprecated_used=true`。

per-LC（P1-07a；golden `tools/migration/golden/lc/`）：

| 覆盖键（`lc_devs/LCx/lc_user_cfg.json`） | 基值键（`lc/lc_cfg.json`） | 示例 |
| --- | --- | --- |
| `config[]` 条目 `{name, type, int_value|double_value}` | 同名扁平键 | `PowerLimit`（type 1 / 数值 80）、`Installedcab_coeff`（type 1）、`EnProtectDOD`（type 0） |

同名条目 override > base > ABSENT；EMS 与 LC 键永不同次解析，`Pmlmax/Pset` 不会与 LC `PowerLimit` 合并。v3 对缺失 `PowerLimit`/`Installedcab_coeff` 自动补 125 的行为不在此切片生效（P1-07a 残余）。

## 7. 计数

计数（脚本校验）：forms=13 classes=6 adr_families=10

脚本读这一行并与 TSV 实际计数比对；文档与表不一致即门禁失败。

## 8. 运行

```sh
python3 ci/scripts/check_iems_strategy_classification.py
python3 ci/scripts/check_iems_strategy_classification.py --selftest
```

`--selftest` 在内存中依次注入缺行、安全家族进 app、10ms 进 app、>100 点进 app、>1000 事件/s 进 app、>32 订阅进 app、同一决策双源、TOU 决策源改成 coordinator、effective 变 app、类/家族/角色错位、13 个 kind 全部伪造、kind 尾随空格（trim 后双源）、kind 内嵌空白、非法 kind token、伪造 decision_owner/execution_owner、`pv_storage_self_use` 改 coordinator、全部 TOU/需量行改 coordinator、PF period 50000（超 CONTRACT §4.3 上限 10000）等负例，要求每条都被拒绝（rc=1）；未注入的基线必须 rc=0。

`ci/scripts/run_guard_selftest.py` 的 live 段会调用本门禁的 `--selftest`；门禁自身负例回归时 CI 直接变红，不需要人记得手动跑。

## 9. 非目标

不切 wasm，不接 coordinator，不改 app runtime、loader、converter、schema、生产策略、AGENTS.md；不做 v3 轨迹对拍（P0-04a）；不做安全签字；不改 PCS 命令与防逆流硬限幅实现；不接 UI/北向。
