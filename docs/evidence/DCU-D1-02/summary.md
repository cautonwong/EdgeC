# DCU-D1-02b：对象模型文档修正（13 项评审逐条）

状态：交 `dcu-verifier` 复核。基线 `feat/dcu 0a401712`（已 merge），分支
`dcu/D1-02b-design-doc`。只改 `docs/design/DCU_698_OBJECT_MODEL.md`，未改代码/测试/标准抽取。

## 1. 逐条：finding → 复核 → 改动

| # | 评审 finding | 复核结论 | 文档改动 |
| --- | --- | --- | --- |
| 1 | DAR 映射与实现相反、缺 ACL 优先 | 成立。驱动 `_dar_for_source_rc`（driver_dlt698.c:245-261）与 ACL 先于对象表（`_acl_check`）是代码事实 | §4.1 访问顺序、§5 DAR 表按代码重写；删除“OI 不在表=4”的旧说法（实为 6） |
| 2 | 8000 接口类（A.9 IC8，产品 13）、6000（A.7 IC11，产品 10） | 成立；reconciliation:48/76 已有记录 | §1 引用 reconciliation；§7 Class 13/11 行标注偏差；§8.7 列出 8000 A2/A4/A5 与 M127–131 |
| 3 | 计数/上限过时（74/60，D1-21） | 成立。`dcu_698_object_count()`=74、appconn OBJECT_COUNT=60；D1-21 后 ACL_MAX 224、AXDR 10240、DRIVER_PRIV_SIZE 4096 | §4.2 改为 74/60 与新容量；列出 14 个“有对象行不进 ACL”OI 及原因；注明上限非设计常量、引用 D1-21 证据 |
| 4 | §8 落点表虚构文件/函数 | 成立。逐文件 `ls`/grep 验证；实际落点见 §7 | §7 表全部改为实际文件；§8.2 用 `dcu_698_record_begin`；§8.8 列出不存在的旧文件名 |
| 5 | Class 10/11 与 A.7/A.8 不符 | 成立（6000 偏差已记录）；A.8 7000/7010 非只读 OAD | §8.3 说明 A.7 归属；§8.8 写 A.8 实义与无 Class 18 行 |
| 6 | 处理器 ABI/单一来源不实 | 成立。实际是 `dcu_698_read_fn/write_fn/method_fn` + `void*`；结构属性走 `dcu_north_resolve_axdr` switch | §3.1 改实际结构/ABI，明确“新增结构属性必须改 resolver switch” |
| 7 | Class 9 “full, 13 OI” 过度声明 | 成立。5000–5007 进 ACL/GetRecord；5008–5011 仅对象行 | §8.2 按实际写（ACL DAR3 / record begin DAR2） |
| 8 | Class 21 OI 写错 | 成立。ESAM 是 F100，F101 属 Class 8；产品只接方法 1、F100 不在 ACL | §8.6 改正；§7 Class 21 行 |
| 9 | Class 12 16/17 顺序、Class 24 名单、Class 22 范围 | 12 与 24 成立（300C–3010 属 Class 7）；Class 22 范围成立；原 finding 关于 F200 的部分**成立**：F200=`OI_CHANNEL_CONFIG`（router.h:104），对象行在 object_model.c:285，无 attr_mask；ACL A4 可写，SET 经 ACL 后回 DAR 5（验收 test_set_masked_f200_attr_returns_class_mismatch，northbound.py:976-980） | §7 Class 12/24/22 行、§8.4、§8.6 已按代码与验收修正 |
| 10 | Class 20/ACL 模型不实 | 成立。0x4400 不在对象表、resolver/驱动特判；ACL 是手写 + 参数宏生成、会话建立时缓存 | §4.1 0x4400 例外、§8.5 加载/缓存、§9 检查单 |
| 11 | 存储表名/类型/端口不存在 | 成立。实际表见 `dcu_storage.c`；`dcu_acq_plan_t`/`edge_event` 不存在 | §6 全部改为实际表名；设计态名称明确“不存在，需另立 schema/端口任务” |
| 12 | 生命周期/所有权/scope | 成立。`g_objects` 非 const；init 绑 8 实例；reset 清空；find 未 init 为 NULL；scope 无读者 | §3.2 生命周期、§3.4 scope 明说“当前无行为差异” |
| 13 | 过期现状描述 | 成立。旧稿的 SET/GetRecord 缺口、ADR 未合并、夹具路径等已过时 | 全文重写，只保留当前代码事实；目录 514 唯一 OI；fixture 531 条 = 514 OI 行 + 17 条实例属性上限；夹具路径未再引用旧值 |

## 2. 文档结构

- §1 口径（标准/目录/偏差来源）
- §2 术语与 DAR 枚举
- §3 实际对象表结构、生命周期、scope
- §4 访问顺序（ACL 优先）、0x4400 例外、74/60 与 D1-21 容量、14 个 ACL 外 OI
- §5 DAR 映射（代码来源）
- §6 实际存储表
- §7 26 类等级/落点表（区分 full/read/DAR/none）
- §8 类要点（Class 8/9/10-11/24-7/20/21-22/8000/A.8）
- §9 新增 OI 检查单（对象行、掩码、handler/resolver、ACL、计数、测试、偏差）
- §10 目标态（明确不是现状）

## 3. 复核证据

- 文件存在性：`ls cmd/dcu` 对照 §7；`grep` 确认 0xF200/F100/8001 等行位置。
- 计数：`dcu_698_object_count()`=74；`OBJECT_COUNT`=60；D1-21 合并后
  `EDGE_SLAVE_ACL_MAX_OBJECTS=224`、`EDGE_SLAVE_AXDR_MAX=10240`、
  `DRIVER_PRIV_SIZE=4096`。
- 标准行号：A.9 8000 在 `docs/698_parsed.md` 8716–8790；Class 9 属性 3 在 4190–4218；
  6000 A.7 偏差引用 reconciliation:48，8000 引用 reconciliation:76。
- F200 复核教训：对象行用宏 `OI_CHANNEL_CONFIG`（=0xF200），直接 grep 字面量会漏；
  已按 router.h:104、object_model.c:285、appconn.c:95 与验收断言改为 DAR 5 事实。

## 4. /no-comments

文档任务，按派发说明跳过 Comment Sicko；comments.md 说明理由。


## 5. 第二次复审 F1–F14 修正（2026-11-24）

| # | finding | 复核/修正 |
| --- | --- | --- |
| F1 | Class 5/23 写反 | 成立：200D/200E 是 IC_HARMONIC=5（object_model.c:183-184）、无掩码；Class 5 行改正，Class 23 改为无对象行（目录 2301..2308）→ DAR 6 |
| F2 | Class 11 写成 DAR | 成立：6014/6016/6018/601A/601C/601E/6032/6034 与 6002 是 IC_COLLECTION=11 且有 handler；Class 11 改 full，落点 class_acq/scheme_axdr/rules；6001 归 IC 8 |
| F3 | ACTION/SET 的 -1 映射混写 | 成立：ACTION `-1` → DAR 2（driver_dlt698.c:263-267）；GET/SET 旧 `-1` → 6（:258-260）。§5 行分开写 |
| F4 | Class 20 写 full | 成立：只 A2（对象列表）有产品路径；A3/A4/A5 与方法 1/2 无 0x4400 对象行/ACL。等级改 A2 only |
| F5 | 531 OI | 成立：目录 514 唯一 OI；fixture 531 条 = 514 OI 行 + 17 条实例属性上限。§1 改正 |
| F6 | Class 12 引错 A.6 且标准表冲突 | 成立：Class 12 在 7.3.12；表157/158 的 16/17 归属冲突（698_parsed 4365-4366 vs 4423-4425）；产品无对象行，记录冲突并待 PDF 所有者裁决 |
| F7 | Class 8 绕过对象掩码 | 成立：param encode/accept 在掩码前（axdr.c:844-845,1071-1074），PARAM_ROW 只广告 A2，ACL 给 4000 A2/A3/A4；§3.3/§8.1 补例外说明 |
| F8 | DAR 8 写成事实 | 成立：dlt698_codec.h:41-47 无 DAR 8；标“标准有、产品未用” |
| F9 | F200 指向不存在的 reconciliation 行 | 成立：改为文档内直接记录产品 attr2/attr3 口径，另立缺陷 |
| F10 | 检查单写 instance | 成立：instance 由 init 统一绑定；检查单改为行只写 oi/class/scope/掩码/handler |
| F11 | DAR 表引用错误 | 成立：标准 6.3.3.11 表34（698_parsed:1923-1925）；§2 改正 |
| F12 | 存储表全归 dcu_storage.c | 成立：scheme/task/rule/attr3/inventory 表由各自 store 建；§6 列出各文件行号 |
| F13 | 4000 A2 SET 失败 DAR | 成立：代码 2（时钟）/7（类型），无 3；验收条款写 2/3，三方不一致，§8.1 按代码写并注明“验收条款待修” |
| F14 | Class 9 full 未限定 | 成立：5008–5011 只有对象行；改为 full (5000–5007) |

复审确认通过的项（原 13 项中 2/3/6/7/8/10/11/12/13、Class 9 属性 3、Class 20 对象列表、
Class 8 A.5、Class 21 F100、Class 22 范围、Class 24 名单、8000/A.8 形状）保持不动。


## 6. 第三次复审 4 项修正

1. `none` 口径：改为“对象表/resolver 回 DAR 6；这些 OI 不在 ACL，主站请求先由驱动回
   DAR 3（§4.1）”；§7 图例、Class 23/24 行、§8.4 同步。
2. §4.1 第 4 步拆开：SET 的 `-1` → 6；ACTION 的 `-1`/通用失败 → 2（driver:263-267）。
3. Class 13 只留 8000（A.9 IC8、产品偏差 13）与 8103（IC13）；8001/8100/8101/8102/
   810E/810F/8110 移入 Class 8 行并注明 A.9 控制族但 IC8、对象行 IC_PARAM、落点
   `dcu_698_axdr.c`；§4.2 按 IC 拆分重写。
4. `none` 行补归属任务：7=D3-05、12=D5-04、14/15/16=D5-05、17=D5-06、18=D4-06、
   19=D4-04、23=D4-09、25/26=D4-10；§10 目标列表补 Class 23。
5. 顺手：summary 第 13 行已改精确口径“目录 514 唯一 OI；fixture 531 条 = 514 OI 行
   + 17 条实例属性上限”；§2 DAR 标“产品使用子集”；Class 11
   加“6032 A2 仍回 DAR 2”；F200 语义改为“当前无线上映射，待点位表拍板”；§8.5 明确
   静态断言只查数量、目录测试只查对象表，不做 ACL 与对象表交叉校验。


## 7. 第四次复审修正

- F1：Class 8 行拆开：8001/8100/8101/8102 resolver DAR 2；810E/810F/8110 仅对象存在性、
  无掩码 → resolver DAR 5（单测 test_dcu_698_axdr.c:1548 断言 810E0200）；七者均不在 ACL，
  主站先回 DAR 3；落点 `dcu_698_axdr.c`。
- F2：§2 DAR 改为“产品相关取值子集；标准完整枚举 0–35 与 255，产品使用见 §5”。
- F3：Class 24 行补 A4（object_model.c:302 广告、class_event.c:15 处理）。
- F4：证据第 13 行改为“目录 514 唯一 OI；fixture 531 条 = 514 OI 行 + 17 条实例属性上限”，
  并更正 §6.5 的旧表述。
- 顺手：Class 13 的 8103 与 Class 8 的七个 IC8 OI 补“不在 ACL → 主站 DAR 3”。
