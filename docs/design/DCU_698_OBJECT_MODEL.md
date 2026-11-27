# DCU DL/T 698.45 对象模型设计

> 日期：2026-11-24　状态：已实现态 + 目标态分区（D1-02b 修正稿）
> 读者：架构、C 开发、协议与驱动、验证
> 关联：`docs/plans/2026-11-21-dcu-698-production-plan.md`、
> `docs/design/DCU_698_OI_CATALOG.md`、`docs/design/DCU_698_CLASS_RECONCILIATION.md`、
> `docs/698.pdf`（原文）、`docs/698_parsed.md`（检索件）

## 1. 口径与事实来源

- 标准原文以 `docs/698.pdf` 为准：7.3 节给类定义，7.4 节与附录 A 给 OI。
  `docs/698_parsed.md` 只作行号检索（本文引用的行号即以它为准）；OCR 冲突回 PDF。
- `DCU_698_OI_CATALOG.md` 由 `scripts/extract_dcu_698_catalog.py` 生成，当前目录
  514 个唯一 OI（fixture 531 条，含 17 个重复 OI）；生成物不参与运行期。
- 本文把所有小节分成 **已实现态** 与 **目标态**。未标注“目标态”的描述都指当前
  `cmd/dcu` 代码事实；目标态只是计划，不能当作现状引用。
- 有记录的标准—产品偏差统一放 `DCU_698_CLASS_RECONCILIATION.md`，本文只引用不复制：
  6000 的 A.7 归属（标准 IC 11，产品 10，reconciliation:48）、8000 的 A.9 归属
  （标准 IC 8，产品 13，reconciliation:76）。两处都是产品决策，不是笔误。

## 2. 术语与 DAR

| 术语 | 含义 | 代码表达 |
| --- | --- | --- |
| IC | 接口类 | `uint8_t class_id` |
| OI | 对象标识 | `uint16_t oi` |
| OAD / OMD | 属性 / 方法描述符 | `uint32_t` |
| DAR | 数据访问结果 | `uint8_t dar` |
| ACL | Class 20 对象列表给出的访问权限 | `k_objects[]` + 驱动 `dlt698_acl_t` |

DAR 取值（标准 6.3.3.11 表34，`docs/698_parsed.md:1923-1925` 的完整枚举是 0–35 与 255；下表是产品相关取值子集，产品实际使用见 §5）：0 成功、1 硬件故障、2 暂时失效、3 拒绝读写、
4 对象未定义、5 对象接口类不符合、6 对象不存在、7 类型不匹配、8 越界、15 密码错/未授权。

## 3. 已实现的对象表

### 3.1 结构与所有权

`cmd/dcu/dcu_698_object_model.h` 的实际结构：

```c
typedef struct dcu_698_object_s {
    uint16_t          oi;
    uint8_t           class_id;
    dcu_obj_scope_t   scope;      /* 当前无读者，两值无行为差异，见 §3.4 */
    dcu_698_read_fn   read;       /* NULL = 属性不可读 */
    dcu_698_write_fn  write;      /* NULL = 只读 */
    dcu_698_method_fn method;     /* NULL = 无方法 */
    const void       *instance;   /* 聚合根/实体指针，不拥有、不释放 */
    uint8_t           attr_mask[32];
    uint8_t           method_mask[32];
} dcu_698_object_t;
```

处理器 ABI 是 `int fn(const dcu_698_object_t *, uint8_t attr/method, uint8_t elem,
void *out/in)`；**不存在** `dcu_attr_read_fn` / `dcu_axdr_buf_t` 之类类型。标量走
`dcu_698_read/write`；结构属性由 `dcu_north_resolve_axdr`（`cmd/dcu/dcu_698_axdr.c`）
按 OI switch 编码，新增结构属性必须改这个 switch，不能只加对象行。

### 3.2 生命周期

- `g_objects[]` 是 `static` **非 const** 表（`dcu_698_object_model.c:11`）。
- `dcu_698_object_model_init()` 绑定 8 个实例指针：6000/6001→inventory、
  6012→poller、6014→poller、F100→esam、6002→discovery、3000/3005→event 对象。
- `dcu_698_object_model_reset()` 清空实例指针；未 init 时 `dcu_698_find()` 返回 NULL。
- 表不拥有 `instance`；实例由 `dcu_node` 聚合根持有。
- 表当前 131 行（`dcu_698_object_count()`），与目录/OI 列表的差异在 §4.2。

### 3.3 查表与掩码

`dcu_698_find` 线性扫描；`dcu_698_read/write/call` 先查掩码。掩码是 256 位位图：
属性/METHOD 未广告时，类定义过 → DAR 5，未定义 → DAR 4；OI 不在表 → DAR 6。
门面不得绕过掩码。**Class 8 参数是例外**：GET/SET 在掩码检查前先走
`dcu_698_param_encode_axdr` / `dcu_698_param_accept_axdr`（`dcu_698_axdr.c:844-845,
1071-1074`），对象行只广告 A2（`PARAM_ROW`），ACL 却给 4000 A2/A3/A4 写权限；参数
白名单与形状由 `dcu_698_param.c` 决定，不靠对象掩码。新增参数属性必须同时改
param 白名单与对象/ACL 行。

### 3.4 scope

`scope` 当前没有任何读者，`DCU_OBJ_SHARED/PER_METER/SINGLETON` 两两之间没有行为差异。
它保留为未来“按表寻址”的元数据，**不是现状约束**；在出现第一个读者前，新增 OI
不应依赖 scope 做访问控制。

## 4. 访问路径与 ACL

### 4.1 顺序（已实现）

从站请求的实际判定顺序（`adapters/io_drivers/src/driver_dlt698.c`）：

1. **ACL 优先**：会话建立/首次请求时由产品对象列表
   `dcu_698_appconn_object_list()` 加载到 `dlt698_acl_t` 并缓存；OI/属性/方法不在
   ACL，直接 DAR 3，永远到不了对象表。
2. 对象表：OI 不存在 → DAR 6；掩码未广告（类定义过）→ DAR 5（未定义 → 4）。
3. 结构属性由 resolver switch 编码；失败按 `_dar_for_source_rc()` 映射。
4. SET 的旧回调 `-1` 按 DAR 6；ACTION 的 `-1` 与通用失败按 DAR 2
   （`_dar_for_action_rc`，driver_dlt698.c:263-267）。

`0x4400`（Class 20 应用连接）是**对象表外例外**：它没有对象行，resolver 与驱动各自
特判 `0x44000200`；新增 OI 不走这个例外。

### 4.2 对象数与 ACL 数（当前）

- 对象表：**131** 行（`dcu_698_object_count()`）。
- Class 20 ACL：**118** 个对象（107 个手写行 + 11 个 `DCU_698_PARAM_OIS` 生成行）。
- 每 ACL 对象：12 B（属性 id 1..18，2 bit/属性压 5 B；方法号每对象最多 12 个）。
  方法号放会话级 `method_ids[1024]` 池，累计超池整单拒绝（fail closed）。
- 每会话存储：`DRIVER_PRIV_SIZE 4096`（D1-21，原 1024）；`EDGE_SLAVE_AXDR_MAX 10240`
  （D1-21，原 2048）；`EDGE_SLAVE_ACL_MAX_OBJECTS 224`（D1-21，operator 2026-11-24
  决定移除 60 项上限，目标集合 201，余 23）。证据见 `docs/evidence/DCU-D1-21/`。
- **对象数上限不是设计常量**；上表数值是 D1-21 的当前容量，未来扩容按同一测量方法
  调整。

有对象行但**不进 ACL** 的 13 个 OI（主站访问 DAR 3）：
`0x5008/5009/500A/500B/5011`、`0x8001`、
`0x8100/8101/8102/8103/810E/810F/8110`。
原因：5008–5011 无切换触发源；A.9 控制族里 IC 8 的
8001/8100/8101/8102（resolver DAR 2）、810E/810F/8110（仅对象存在性，resolver DAR 5）
与 IC 13 的 8103（DAR 2）都只有对象行；ACL 当时只复用 8000。D1-21 已解除数量上限，但把
它们加进 ACL 需要产品把对应能力从“仅 DAR”升级为可发布，属后续任务。

## 5. DAR 映射（代码事实）

| 情况 | DAR | 代码 |
| --- | --- | --- |
| 成功 | 0 | `EDGE_SLAVE_SRC_OK` |
| 暂时失效（无来源/未接硬件） | 2 | `EDGE_SLAVE_SRC_TEMPORARY`；ACTION 的 `-1` 也按 2 |
| ACL 未授权 | 3 | `EDGE_SLAVE_SRC_DENIED` |
| 类/实例未定义 | 4 | `EDGE_SLAVE_SRC_UNDEFINED` |
| 类定义但属性/方法未广告 | 5 | `EDGE_SLAVE_SRC_CLASS_MISMATCH` |
| 对象表无此 OI | 6 | `EDGE_SLAVE_SRC_NOT_FOUND`；GET/SET 旧回调 `-1` 也按 6（ACTION 的 `-1` 见 DAR 2） |
| 类型/形状不匹配 | 7 | `EDGE_SLAVE_SRC_TYPE_MISMATCH` |
| 越界 | 8 | 标准有、产品当前未使用：`dlt698_codec.h:41-47` 无 DAR 8 映射；Class 9 容量拒绝按派发裁决用 DAR 3 |

不静默成功：任何失败都必须回上表对应的 DAR，并写入产品日志的失败原因。

## 6. 存储映射（实际表名）

表由 `dcu_storage.c` 与各 store 分别建：`dcu_scheme_store.c:76-118`、
`dcu_task_store.c:63-68`、`dcu_rule_store.c:10-15`、`dcu_698_freeze_cfg.c:49-54`、
`dcu_inventory.c:319`。运行期表名如下（没有 `dcu_events/dcu_plans` 等）：

| 内容 | 表 |
| --- | --- |
| 表档案与影子 | `dcu_meters`、`meter_inventory`、`dcu_telemetry_latest`、`telemetry` |
| 冻结记录与属性 3 | `dcu_freeze_history`、`dcu_freeze_attr3`、`dcu_freeze_attr3_meta` |
| 参数与审计 | `dcu_params`、`dcu_param_audit` |
| 事件记录 | `event_history` |
| 任务/方案 | `dcu_task_config`、`dcu_task_meta`、`dcu_scheme_config`、`dcu_scheme_csd`、`dcu_scheme_meta` |
| 事件/透明/上报方案 | `dcu_event_scheme`、`dcu_event_road`、`dcu_event_road_rel`、`dcu_transparent_scheme`、`dcu_transparent_content`、`dcu_transparent_frame`、`dcu_transparent_result`、`dcu_transparent_result_item`、`dcu_report_scheme`、`dcu_report_channel` |
| 采集规则 | `dcu_rule`、`dcu_rule_meta` |

“设计态”的 `dcu_acq_plan_t`、`dcu_control_op_t`、`edge_event` 端口在仓库中不存在；
后续若引入，必须先立 schema/端口任务，不能当作现状引用。

## 7. 类等级与落点（已实现态）

等级：`full` = 读写/方法已实现；`read` = 只读；`DAR` = 只有诚实 DAR/对象行；
`none` = 无对象行：对象表/resolver 回 DAR 6；这些 OI 也不在 ACL，主站请求先由驱动回 DAR 3（§4.1）。

| 类 | 等级 | 实际落点 / 说明 |
| --- | --- | --- |
| 1 电能量 | read | `dcu_698_var.c`（2000.. 变量族） |
| 2 最大需量 | DAR | 对象行 1010–1013，无掩码 → DAR 5；无来源 |
| 3 分相变量 | read | `dcu_698_var.c` |
| 4 功率 | read | `dcu_698_var.c` |
| 5 谐波 | DAR | 200D/200E 对象行（`object_model.c:183-184`，IC_HARMONIC=5），无 attr_mask，无来源 → DAR 5 |
| 6 数据变量 | read | `dcu_698_var.c` |
| 7 事件对象 | none | 目录 3009.. 无对象行（resolver DAR 6，主站 ACL DAR 3）；产品事件 OI 全按 Class 24 建模（§8.4）；归属任务 D3-05 |
| 8 参数变量 | full（白名单） | `dcu_698_param.c`；4000–4004、401C–4021；`dcu_params`/审计。A.9 控制族里 IC 8 的七个 OI 也在本类（698_parsed 8756/8821/8822/8824/8931/8932/8933，对象行 IC_PARAM，落点 `dcu_698_axdr.c`）：8001/8100/8101/8102 resolver 回 DAR 2；810E/810F/8110 仅对象存在性、无 attr_mask，类定义但实例未声明 → resolver DAR 5（单测 test_dcu_698_axdr.c:1548 断言 810E0200）；七者均不在 ACL，主站先回 DAR 3 |
| 9 冻结数据 | full (5000–5007) | `dcu_698_freeze.{c,h}`、`dcu_698_freeze_cfg.{c,h}`、`dcu_698_record.c`；5008–5011 只有对象行 |
| 10 采集监控 | full | `dcu_698_class_acq.c`、`dcu_698_scheme_axdr.c`、`dcu_698_rules.c` |
| 11 集合类 | full / DAR | `dcu_698_class_acq.c`、`dcu_698_scheme_axdr.c`、`dcu_698_rules.c`：6014/6016/6018/601A/601C/601E/6032/6034 与 6002 均 IC_COLLECTION（6032 A2 仍回 DAR 2）；6000 的 A.7 偏差（标准 11、产品 10，reconciliation:48）；6001 是 IC 8。A.8 的 7000（`OI_FILE_SET`）与 7010（`OI_SCRIPT_SET`）只有对象行与只读 ACL（空掩码）：7010 A2 在 `dcu_north_resolve_axdr` 回 DAR 5，7000 方法 127–129 与 7010 方法 127–130 在 `dcu_698_check_call` 回 DAR 5，类未定义回 DAR 4，主站未在 ACL 先回 DAR 3；无文件/脚本集合存储，不返回空集合或伪造元素；见 `docs/evidence/DCU-D5-03/` |
| 12 脉冲计量 | DAR | 2401 对象行（`dcu_698_object_model.c`，IC_PULSE=12，空掩码）并进 ACL（`dcu_698_appconn.c`，属性 1..15 只读、方法 1..4）；无脉冲量来源 → resolver 对表157 定义的 A1..A19 回 DAR 5、A20 回 DAR 4；16/17 的归属在表157/158 之间冲突（`698_parsed.md:4365-4366` vs `:4423-4425`），按类定义表157 记为已定义但 ACL 不广告（HOLD，主站先 DAR 3）；A18/A19 仍不广告（ACL 只放 1..15，schema 已支持到 18）；方法与 SET 不伪造成功；见 `docs/evidence/DCU-D5-04/` |
| 13 控制 | read/DAR | `dcu_698_class_control.c`、`dcu_698_axdr.c`；8000（A.9 IC 8、产品偏差 13，reconciliation:76）只读 A2/A4/A5 DAR 2（无继电器来源，D4-02a），方法 129–131 无继电器 OAD→表地址映射 → DAR 2；8103（A.9 IC 13）A2 只读 DAR 2，且不在 ACL → 主站 DAR 3 |
| 14 区间统计 | DAR | 2100–2104 对象行（`dcu_698_object_model.c`，IC_INTERVAL_STAT=14，空掩码）并进 ACL（`dcu_698_appconn.c`，属性 1..3 只读、方法 1..4）；无统计生产者，resolver 回 DAR 5、类未定义 DAR 4（主站先被 ACL 拒为 DAR 3）；方法与 SET 不伪造成功；见 `docs/evidence/DCU-D5-05/` |
| 15 累加平均 | DAR | 2110–2114 对象行（IC_AVERAGE=15，空掩码）并进 ACL（属性 1..3 只读、方法 1..4）；无生产者 → resolver DAR 5/4、主站未广告先 DAR 3；不伪造统计值 |
| 16 极值工具 | DAR | 2120–2124 对象行（IC_EXTREMUM=16，空掩码）并进 ACL（属性 1..3 只读、方法 1..4）；无生产者 → resolver DAR 5/4、主站未广告先 DAR 3；`_class_max_method` 按表 171 修正为 4 |
| 17 显示 | DAR | F300/F301 对象行（`dcu_698_object_model.c`，IC_DISPLAY=17，空掩码）并进 ACL（`dcu_698_appconn.c`，属性 2..4 只读、方法 1..6）；无显示配置来源 → resolver DAR 5/4、主站未广告属性先 DAR 3；方法与 SET 不伪造成功；见 `docs/evidence/DCU-D5-06/` |
| 18 文件传输 | DAR | F000/F001/F002 对象行（`dcu_698_object_model.c`，IC_FILE_TRANSFER=18，空掩码；`k_oi_max_attr` 登记 F000 A1..A5、F001/F002 A1..A4）并进 ACL（`dcu_698_appconn.c`：F000 A1..A5、F001/F002 A1..A4 只读，F000/F002 方法 1..8、F001 方法 1..10）；无文件传输/DFU 来源 → resolver 对已定义属性与方法回 DAR 5、类/OI 未定义回 DAR 4；F001 方法 9/10 已广告，主站 ACTION 回 DAR 5；SET 回 DAR 3；不伪造传输结果；见 `docs/evidence/DCU-D4-06/` |
| 19 设备管理 | DAR | 4300/4307–4309 对象行（`dcu_698_object_model.c`，IC_DEVICE_MGMT=19，空掩码）并进 ACL（`dcu_698_appconn.c`，A1..A10 只读、方法 1..6）；无设备描述/版本/复位/初始化端口 → resolver 对表178 定义的 A1..A10 回 DAR 5、A11 回 DAR 4；方法 1..6 回 DAR 5（绝不执行复位/数据初始化/恢复出厂/事件初始化/需量初始化）；4307..4309 的 127..129 已计入定义并进入 ACL，主站 ACTION 回 DAR 5；SET 回 DAR 3；见 `docs/evidence/DCU-D4-04/` |
| 20 应用连接 | A2 only（ACL） | `dcu_698_appconn.c` + 驱动 `_acl_load/_acl_parse`；A3/A4/A5 与方法 1/2 无 0x4400 对象行与 ACL；0x4400 例外见 §4.1 |
| 21 ESAM | DAR | OI `F100`（表184；F101 是 Class 8 安全模式参数，不是本类实例）：对象行空掩码并进 ACL（A1..A15 只读、方法 1..12）。产品没有 ESAM 硬件/会话来源，软件 ESAM 模拟只保留 C API；resolver 对 A1..A15 回 DAR 5、A16 回 DAR 4；方法 1..12 回 DAR 5（698 面不执行复位/读取/密钥/证书/钱包/红外认证）；SET 回 DAR 3；不返回序列号/版本/会话/MAC；见 `docs/evidence/DCU-D4-07/` |
| 22 输入输出 | read/DAR | 目录 F200..F20C 的 13 个 OI 都有对象行（`dcu_698_object_model.c`，IC_PERIPHERAL、SINGLETON）并进 ACL（`dcu_698_appconn.c`）；A.12 只读属性如实广告：A2 全部，F203/F204/F206/F20C 加 A4，F209 加 A5/A6；无端口配置来源 → resolver：已广告属性 DAR 2、类定义未广告 DAR 5、类未定义 DAR 4（主站对未广告属性先被 ACL 拒，回 DAR 3）；SET 回 DAR 3；A.12 的 OI 专属方法 127/128 已计入定义（resolver DAR 5、未实现；主站 ACTION 因 ACL 无方法回 DAR 3） |
| 23 总加组 | DAR | 2301–2308 对象行（`dcu_698_object_model.c`，IC_GROUP=23，空掩码）并进 ACL（`dcu_698_appconn.c`，A1..A18 只读、方法 1..5）；无总加组配置/总加值/滑差/功控来源 → resolver 对表190 定义的 A1..A18 回 DAR 5、A19 回 DAR 4；方法 1..5 回 DAR 5（绝不执行清空/添加/删除/执行）；SET 回 DAR 3；不伪造总加值、滑差或轮次；见 `docs/evidence/DCU-D4-09/` |
| 24 分项事件 | full（3000/3005） | `dcu_698_class_event.c`；3000/3005 实现 A2/A3/A4/A6/A11/A12/A14；其它 OI 无对象行（resolver DAR 6，主站 ACL DAR 3） |
| 25 无线公网通信 | DAR | 4500/4501 对象行（`dcu_698_object_model.c`，`IC_WIRELESS_PUBLIC`=25，空掩码）并进 ACL（`dcu_698_appconn.c`，A1..A11 只读、方法 1）；产品没有 APN/ICCID/IMSI/SIM 号码/信号强度来源 → resolver 对表196 定义的 A1..A11 回 DAR 5、A12 回 DAR 4；主站未在 ACL 先 DAR 3；方法与 SET 不伪造成功；见 `docs/evidence/DCU-D4-10/` |
| 26 以太网通信 | DAR + 只读 MAC | 4510–4517 对象行（`IC_ETHERNET`=26）；4510 A5 是唯一有来源的属性（`dcu_comm_info.c` 读平台第一个物理单播以太网口的 MAC：type=1、无 wireless/ 与 bridge/、有总线 device 链接），A1..A4 与 4511–4517 的 A5 回 DAR 5、A6 回 DAR 4；ACL A1..A5 只读、方法 1；平台没有符合判据的以太网口（含容器 veth）时 A5 也回 DAR 5，不伪造；见 `docs/evidence/DCU-D4-10/` |

## 8. 类要点（与实现对齐）

### 8.1 Class 8 参数白名单

`cmd/dcu/dcu_698_param.c` 的实际白名单：4000 A2 date_time_s、A3 enum、A4 structure；
4001–4003 A2 octet-string；4004 A2 地理位置结构；401C/401D A2 double-long-unsigned；
401E/4020 A2 2×double-long-unsigned；401F/4021 A2 3×double-long-unsigned（A.5 顺序）。
4000 A2 GET/SET 走注入的 `dcu_clock_port_t`；SET 失败代码回 DAR 2（时钟端口）
或 7（类型，`dcu_698_param.c:432-444`），没有 DAR 3；验收条款
（`test_dcu_northbound.py:927`）写 2/3，属文档/代码/合同三方不一致，验收条款待修
（另立缺陷评估）。其余参数写
`dcu_params` 并记 `dcu_param_audit`；产品目前没有功能消费这些配置，SET 成功不等于
设备行为改变。

### 8.2 Class 9 冻结

- 对象表与 ACL：5000–5007 进 ACL；5008/5009/500A/500B/5011 只有对象行（ACL DAR 3，
  GetRecord begin 回 DAR 2）。
- 7.3.9 方法集合为 1/2/3/4/5/7/8（无 6）：`dcu_698_class_defines_method` 按真实集合
  判定，对象表未掩码的方法 1/2 回 DAR 5、方法 6 回 DAR 4；ACL 只广告 3/4/5/7/8，
  主站对未广告方法先回 DAR 3。
- 属性 3 `{冻结周期, OAD, 存储深度}`（698_parsed 4190–4218）持久化在
  `dcu_freeze_attr3(_meta)`；调度器按 (OI, 条目) 独立周期，方法 3/4/5/7/8 已实现。
- GetRecord 由 `dcu_698_record.c::dcu_698_record_begin/row/end` 实现，RSD Selector1/2
  选时间，RCSD 选列；不存在 `dcu_698_freeze_read`。
- 深度裁剪在同一写入事务；容量上限 8 项/OI，第 9 项 DAR 3（D3-03b 裁决）。

### 8.3 Class 10/11

A.7 的 6000 是 IC 11、6012 是 IC 10，6000–6042 区域分属 8/10/11；产品把 6000 记为
IC 10（reconciliation:48）。档案/任务/方案/规则的管理面在
`dcu_698_class_acq.c`、`dcu_698_scheme_axdr.c`、`dcu_698_rules.c`，action 解码入口
`dcu_698_accept_archive_action` / `dcu_698_accept_action_axdr`；记录查询走
`dcu_698_record_begin` 与各方案 encode。

### 8.4 Class 24 与 Class 7

Class 24 标准名单是 3000–3008（3006 断流、3007 功率反向、3008 过载）与 300B；
300C–3010 属于 Class 7，不在 Class 24。产品只实现 3000/3005（失压/过流），属性
A2 关联 OAD 数组、A3 四表计数、A4、A6、A11/A12/A14；A5 因结构随 OI 变化且未存储
回 DAR 5。Class 7 的目录 OI（3009..）在产品无对象行：resolver 回 DAR 6（D3-08 单测），主站请求因不在 ACL 先回 DAR 3（§4.1）。

7.3.7 与 7.3.24 的方法集合都是 1/2/4/5（无 3）：3000/3005 的方法 3 在
`dcu_698_check_call` 回 DAR 4，方法 2/4/5 类定义但无处理器回 DAR 5，方法 1 仍执行
复位；Class 7 无实例，同一判定由 `dcu_698_class_defines_method` 表达。

### 8.5 Class 20 与 ACL 加载

`dcu_698_appconn_object_list()` 编码 `array { structure { OI, { 属性[], 方法[] } } }`；
118 项列表当前编码 5806 字节（D1-21 测量方法；115 项时为 5551 字节），>127 项时使用
A-XDR 长格式长度（D1-21）。
驱动在会话建立时加载并缓存 ACL，不是每请求重查；`0x4400` 由 resolver 与驱动特判，
不在对象表。对象列表是**手写表 + 参数宏生成行**，不是“对象表单一来源”；`OBJECT_COUNT
<= ACL_MAX` 由静态断言守住，对象表与目录的一致性由目录测试守住。**两者不做 ACL 与
对象表的逐项交叉校验**（没有脚本比对 k_objects 与 g_objects）。

从站响应不能复用采集/上报尺寸的共享池：118 项对象列表 5806 B 加 22 B 帧是 5828 B，
超过共享池 4096 B。`bus_orchestrator_setup_slave` 因此给从站单独建
`BUS_SLAVE_POOL_BUF_SIZE = EDGE_SLAVE_AXDR_MAX + 256` 的池（32 项，约 328 KB），
而不是把 8192 × 4096 的共享池抬到约 85 MB；编码器（`dlt698_codec.c`）在放不下时回
错误码，GET 路径回 DAR 2，绝不越过 `b.size` 写。

### 8.6 Class 21/22

Class 21 的 OI 是 F100（不是 F101）；表184 定义 A1..A15 与方法 1..12。产品没有 ESAM
硬件/会话来源，软件 ESAM 模拟（`dcu_security_*` C API）不再从 698 面暴露：F100 对象行
空掩码并进 ACL（A1..A15 只读；方法 1..12 全部广告），GET 与
ACTION 按类定义回 DAR 5，绝不伪造序列号/版本/会话/MAC/密钥。Class 22 标准范围
F200..F20C（F210 属 Class 8）：13 个 OI 各有一行
（`OI_CHANNEL_CONFIG` 起，`dcu_router.h` 的 `0xF200`–`0xF20C`，IC_PERIPHERAL、
SINGLETON），属性掩码按 A.12 广告（A2 全部；F203/F204/F206/F20C 加 A4；F209 加
A5/A6）。A2 是端口对象列表（端口描述符、COMDCB、功能）数组，不是标量：F200 的
A2/A3 标量描述（VK_U32/VK_U8，此前被掩码挡住、不可达）已删除，标量端口对已广告
属性回 TYPE_MISMATCH，不截断。`dcu_698_axdr.c` 的 Class 22 分支对已广告属性回
**DAR 2**（产品没有端口配置的持久化来源），类定义未广告回 DAR 5、类未定义回 DAR 4；
主站对未广告属性先被 ACL 拒，回 **DAR 3**，5/4 只在 resolver 层可见。A.12 的 OI 专属
方法（F200/F201/F202/F205/F207/F20B 的 127、F209 的 127/128）已计入
`_class_defines_method` 的定义：resolver 回 DAR 5（未实现），主站 ACTION 因 ACL 无
方法回 DAR 3。ACL 行在 `dcu_698_appconn.c`，只读（rights 1），SET 因无写处理器回
**DAR 3**。F200 的既有 ACL 行 `k_attrs_f200 = {{2,1},{3,1},{4,3}}` 不变，因此 A4 SET
仍在“类定义但实例未声明属性”→ **DAR 5**；验收
`test_set_masked_f200_attr_returns_class_mismatch`
（`acceptance/dcu/tests/test_dcu_northbound.py`）断言该值。reconciliation 文件暂无
F200 行；待产品点位表拍板，另立缺陷。不得把 ACL 只读当成 A2 已实现。



### 8.7 8000 的只读属性与方法

A.9（`docs/698_parsed.md` 8716–8790）的 8000：A2 structure{拉闸电流门限 double-long-unsigned
换算 -4, 超电流保护延时 long-unsigned 分钟}、A4 告警状态 bit-string(8)、A5 命令状态
bit-string(8)；方法 127/128 参数 NULL，129 跳闸 array<structure{OAD,告警延时,限电时间,
自动合闸}>，130 合闸 array<structure{OAD,enum}>，131 明文合闸 array<structure{OAD,enum,
visible-string}>。产品当前：A2/A4/A5 只读，无继电器输出/状态来源 → DAR 2（D4-02a）；
方法 127–131 参数可解，但没有继电器 OAD→电表地址映射 → DAR 2，不假装执行。

### 8.8 A.8 集合对象与不存在的文件存储

A.8 的 7000（方法 127 写文件、128 执行、129 删除；无属性）与 7010（A2 array 脚本，方法
127–130）已按 Class 11 登记为对象行（`OI_FILE_SET` / `OI_SCRIPT_SET`，
`dcu_698_object_model.c`，空属性掩码）并进 ACL（`dcu_698_appconn.c`：7000 方法 127–129、
7010 A2 只读与方法 127–130）。`k_oi_max_method` 为两个 OI 记录 127–129 / 127–130 的实例
方法定义，类 11 的类上限仍为 2，不把方法 3..126 误广告给其他 IC_COLLECTION OI。

Class 18 文件传输由 D4-06 另行登记（见 §7 与 `docs/evidence/DCU-D4-06/`）。产品没有
文件/脚本集合存储与集合元素 producer，因此 `dcu_north_resolve_axdr` 对 7010 A2、`dcu_698_check_call` 对 7000 方法
127–129 与 7010 方法 127–130 如实回 DAR 5，类未定义回 DAR 4，主站未在 ACL 的请求先回
DAR 3；不返回空集合、空数组或伪造元素。计划 §8 D5-03 的出口写
“GET 7000 返回集合元素”，与 A.8（7000 只有方法、7010 才有 A2）冲突，本单按标准改为
7010 A2 DAR 5 + 7000 方法 DAR 5 的诚实骨架；偏差与证据见
`docs/evidence/DCU-D5-03/summary.md`。

旧稿列出的 `dcu_698_event.c`、`dcu_698_acq.c`、`dcu_698_set.c`、`dcu_698_pulse.c`、
`dcu_698_control.c`、`dcu_698_stat.c`、`dcu_698_display.c`、`dcu_698_file.c`、
`dcu_698_devmgr.c`、`dcu_698_esam.c`、`dcu_698_io.c`、`dcu_698_group.c`、
`dcu_698_comm.c` 均不存在；Class 25/26 的只读来源是 `dcu_comm_info.c`，不是旧稿的
`dcu_698_comm.c`。本单没有新建 `dcu_698_set.c`，因为没有可落的处理器。
实际落点以上表为准。

### 8.9 Class 25/26 通信参数

表196/199 定义 Class 25 A1..A11 与 Class 26 A1..A5，两者都只有方法 1（复位）。产品
没有 APN/ICCID/IMSI/SIM 号码/信号强度/端口配置的存储或生产者；平台能给的只有本机
第一个物理单播以太网口的 MAC（`/sys/class/net`，`dcu_comm_info.c`；判据是 type=1、
无 `wireless/` 与 `bridge/`、有总线 `device` 链接，选择逻辑在 `dcu_comm_info_scan`，
测试注入夹具根）。对象模型 init 时读取快照、
reset 时清空；4510 A5 编码成 octet-string，其余类定义属性在 resolver 层回 DAR 5
（类未定义 DAR 4），主站未在 ACL 的属性/方法先回 DAR 3。没有符合判据的以太网口
（含容器 veth）时 A5 也回 DAR 5，不返回全零或编造的 MAC；SET 因 ACL 只读回 DAR 3，
方法 1 无处理器回 DAR 5。
`k_class_max_method` 的 25/26 从 2 改为 1，与表198/201 和目录一致。证据见
`docs/evidence/DCU-D4-10/`。

## 9. 新增一个 OI 的检查单

1. 目录：确认 OI 在 `DCU_698_OI_CATALOG.md`，记录标准行号与 IC。
2. 对象行：`dcu_698_object_model.c` 只写 oi/class/scope/掩码/handler；`instance`
   由 `dcu_698_object_model_init()` 统一清空并绑定 8 个实例，不在行里写。
3. 处理器：标量接 `read/write/method`；结构属性改 `dcu_698_axdr.c` 的 resolver
   switch；记录型接 GetRecord。
4. ACL：`dcu_698_appconn.c` 加属性/方法权限（或确认“只对象行不进 ACL”的理由）。
5. 计数与容量：`dcu_698_object_count()`、`OBJECT_COUNT <= ACL_MAX`、目录测试同步；
   容量测量见 D1-21 方法。
6. 测试：GET/SET/ACTION/Record 行为断言 + 无来源时具体 DAR；验收按 `docs/design/app-platform/10-product-points.md`。
7. 偏差：与标准 IC/属性不一致时先写进 `DCU_698_CLASS_RECONCILIATION.md`。

## 10. 目标态（未实现，不是现状）

- 对象表 26 类全部落点、ACL 覆盖所有已发布 OI、scope 产生真实寻址行为。
- Class 7 事件对象、Class 12/14/15/16/17/18/19/25/26 的处理器。
- 控制执行 129/130/131 与继电器/功控真实来源（D4-02 后续）。
- Class 23 总加组（D4-09）的对象/处理器；Class 25/26 已落只读 DAR 骨架与 4510 的
  平台 MAC，通信参数的写、持久化与 APN/ICCID/IMSI/信号来源仍是目标态。
- 上述目标的进度以 `docs/plans/2026-11-21-dcu-698-production-plan.md` 为准；本文不
  把它们写成已实现。
