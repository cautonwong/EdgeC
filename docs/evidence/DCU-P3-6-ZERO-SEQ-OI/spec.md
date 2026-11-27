# DCU-P3-6-ZERO-SEQ-OI — OAD_ZERO_SEQ_CURRENT 的 OI 归属

基线 `feat/dcu` = `40d8dca5`。本单目标：判定 `cmd/dcu/dcu_router.h:56` 的
`OAD_ZERO_SEQ_CURRENT`（`0x20020200`，OI 2002 属性 2）应取的正确 OI，或给出
“未用且无可判定位，移除”的判断。

## 五栏

| 业务规则 | 不变量 | 可观测输出 | 验收判据 | 非目标 |
| --- | --- | --- | --- | --- |
| R1 `dcu_router.h` 每个 `OAD_*` 常量的 OI 必须在 `docs/design/DCU_698_OI_CATALOG.json` 存在，且目录语义与常量名一致；冲突即失败。 | I1 任何 OAD 常量不得指向不同物理量的 OI。 | 测试失败消息给出宏名、OAD 值、OI 与目录名。 | AC1 `tests/test_dcu_698_router_oad_catalog.py` 中 OI 存在性用例当前为绿；零序语义用例当前为红。 | N1 不引入非标准私有 OI。 |
| R2 标准表 A.3（`docs/698_parsed.md:6652`）与 7.4.4 表 204（`:5752`）定义 OI 2002 = 电压相角（Class 3，long-unsigned，度，换算 -1）；全标准无“零序电流”OI。 | I2 `OAD_ZERO_SEQ_CURRENT` 不得继续指向 `0x2002`。 | 目录 JSON/表中 OI 2002 的 name = `电压相角`。 | AC2 标准与目录逐行对拍（本 spec“标准依据”）。 | N2 不修改标准解析物与生成器输出。 |
| R3 标准里与零序最接近的量是 OI 2001 属性 4 零线电流（`:6651`，换算 -3；IN=Ia+Ib+Ic=3I0），不是零序电流；2027 是电流不平衡率（`:6709`）；3006 是电能表断流事件（Class 24，`:6906`），均不可别名。 | I3 不把零线电流、电流不平衡率或事件 OI 当作零序电流。 | 标准行号与量纲。 | AC3 若后续实现以 2001 A4 或 3006 充当零序，语义用例必须失败。 | N3 不新建私有零序 OI，不实现零序计算。 |
| R4 `OAD_ZERO_SEQ_CURRENT` 无对象行/方案引用，北向名已删（`tests/test_dcu_north_source.c:91`）；唯一生产引用是规则种子表 `cmd/dcu/dcu_698_rules.c:42`。一致处置 = 移除宏与种子行。 | I4 主站 GET/ACTION/Record 行为不变；移除种子行不得改变现存对象访问。 | 引用清单（本 spec 下节）。 | AC4 修复单必须同时删除宏与种子行并更新 `tests/test_dcu_698_rules.c:67`；只删宏会编译失败。 | N4 本单不改 `cmd/dcu/dcu_698_rules.c`（文件边界），因此不实施修复。 |
| R5 标准/解析物中不存在可判定的零序电流 OI，且至少有“移除未用常量”“映射到 2001 A4 需额外 /3 换算”“私有 OI”三种自洽解释，无法唯一裁决。 | I5 不跨界改、不发明 OI、不带着歧义实现。 | `red.log` 的失败断言。 | AC5 本单停在 spec：交红测试与证据；不交 commit 2、变异守卫与验收。 | N5 不改 `dcu_698_rules.c`、`dcu_698_object_model.c`、`dcu_698_appconn.c`、`dcu_698_var.c`、`dcu_soe_event_engine.h`。 |

## 标准依据

- `docs/698_parsed.md:6652`：`2002  3   电压相角  数据类型：long-unsigned，单位：度，换算：-1`（附录 A.3 表 A.3）。
- `docs/698_parsed.md:5752`：7.4.4 表 204，`02H：电压相角`。
- `docs/698_parsed.md:6651`：OI 2001 属性 4 `零线电流∷=double-long, 单位：A 换算：-3`。
- `docs/698_parsed.md:6709`：`2027  6   电流不平衡率`；`:6708` 2026 电压不平衡率。
- `docs/698_parsed.md:6906`：`30 06  24   电能表断流事件`（事件 OI，非零序）。
- `docs/698.pdf`（pdftotext 全文，行 6916）同句：`2002  3  电压相角`；全文 `零序` 出现 0 次。
- 目录：`docs/design/DCU_698_OI_CATALOG.md:283`、`DCU_698_OI_CATALOG.json` OI 2002（`source_line: 6652`）均为电压相角、Class 3。

## 引用清单

- 生产：`cmd/dcu/dcu_router.h:56`（宏定义）、`cmd/dcu/dcu_698_rules.c:42`（DLT698 种子规则行）。
- 测试：`tests/test_dcu_698_rules.c:67`（期望种子行）、`tests/test_dcu_698_scheme_mgmt.c:860`（借用作删除目标）、`tests/test_dcu_north_source.c:91`（断言北向名为 NULL）。
- 对象表/方案/ACL：无引用（`DCU_698_OAD_STATUS.md:243`、`docs/plans/2026-11-21-dcu-698-production-plan.md:974`）。
- 遗留误标（不在本单边界）：`cmd/dcu/dcu_soe_event_engine.h:24` 把事件 OI 3006 注释成“零序电流超限”；`docs/design/DCU_698_OAD_STATUS.md:306` 把标准 OI 2002 写成“零序电流”。

## 测试映射

| spec 条目 | 测试 |
| --- | --- |
| R1 / AC1 | `test_every_oad_macro_oi_is_in_the_catalog` |
| R2 / I2 / AC1 | `test_zero_seq_current_macro_is_absent_or_zero_sequence_current`（宏缺失视为合规；宏存在则目录名必须含“零序”） |

## 停点（spec 回报）

1. 标准可判定：OI 2002 属性 2 = 电压相角，`OAD_ZERO_SEQ_CURRENT = 0x20020200` 是错误别名。
2. 标准无可判定零序电流 OI：PDF/parsed 全文无“零序”；2001 A4 零线电流、2027 电流不平衡率、3006 断流事件均非零序。
3. 唯一保持主站行为不变的修法是移除宏与 `dcu_698_rules.c:42` 种子行；该文件不在本单文件边界内。
4. 按任务约束“修复必须改 `cmd/dcu/dcu_698_rules.c` 或超出边界 → 停在 spec 回报”，本单不实施 commit 2、变异守卫与验收，等待 verifier 拆跨文件修复单。

## 裁决与实施（verifier 2026-11-27）

裁决：采用最小删除——删除 `OAD_ZERO_SEQ_CURRENT` 宏与唯一生产引用（`cmd/dcu/dcu_698_rules.c`
种子行），不做 2001 A4 派生、不造私有 OI。依据：表 A.3（`docs/698_parsed.md:6652`）与
7.4.4 表 204（`:5752`）的 OI 2002 是电压相角；全标准无零序电流 OI；2001 A4 是零线电流
（`698_parsed.md:6651`，IN=3I0），语义不同；当前无对象/ACL/方案消费。授权边界扩展到
`cmd/dcu/dcu_698_rules.c`、`tests/test_dcu_698_rules.c`、`tests/test_dcu_698_scheme_mgmt.c`
（仅该 OAD 相关断言）、`docs/design/DCU_698_OAD_STATUS.md`、宏所在头文件、本 pytest 与证据。

实施：删宏；删规则种子行；更新规则期望表；`test_rule_dar_cases` 的 NOT_FOUND 场景改为
先成功删除 `OAD_REACTIVE_POWER_TOTAL`、再删一次得 NOT_FOUND（同一语义：映射在、标准行 0）；
`test_dcu_north_source.c` 去掉零序断言（保留同样“无北向名”的 `OAD_REACTIVE_POWER_TOTAL`）；
`DCU_698_OAD_STATUS.md` 删该 OAD 行并把“2002（零序电流）”订正为“2002（电压相角）/ 标准无零序 OI”。
新 pytest 保持为守卫：宏缺失即合规、每个 OAD 宏的 OI 必须在目录中；已注册进
`ci/scripts/run_guard_selftest.py`（全绿，见 `guard-selftest.log`）。

验证：删除前 pytest 红（`red.log`，d9943a37）；删除后 pytest 绿、`ctest -R 'dcu|dlt698'`
45/45（`tests-after-delete.log`，137.24s）、验收 1 轮退出 0、全部合格 2 个验收任务
（`acceptance-1.log`）。相邻误标（3006 注释、scripts 硬编码、`OAD_REACTIVE_ENERGY` 命名）
按裁决留作 follow-up，本单未改。
