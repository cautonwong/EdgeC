# DCU-P3-13 五栏规格：OAD 误标清理、3006 注释订正、`OAD_REACTIVE_ENERGY` 审计与 `rules` 测试 `snprintf` 替换

基线：`feat/dcu` (`acef3050`)，工作分支 `dcu/P3-13-oad-cleanup-and-rules-strcpy`。

## 五栏规格

| 业务规则 | 不变量 | 可观测输出 | 验收判据 | 非目标 |
| --- | --- | --- | --- | --- |
| R1 `cmd/dcu/dcu_soe_event_engine.h:24` 中 `0x3006` 依据 `docs/698_parsed.md:6906`（及 `:5876`）为 `电能表断流事件`（Class 24），不得注释为“零序电流超限”。 | I1 保留枚举标识符 `OAD_EVT_ZERO_SEQ_CURRENT = 0x3006` 不变，确保既有调用方与 ABI 零破坏。 | `dcu_soe_event_engine.h:24` 注释准确标明标准 OI `3006` 为电能表断流事件，并标注旧枚举名为兼容保留。 | AC1 静态检查与编译（0 新增 warning）通过；既有 C 测试全绿。 | N1 不重命名 `OAD_EVT_ZERO_SEQ_CURRENT` 或修改事件引擎逻辑。 |
| R2 依据 `docs/698_parsed.md:6652`（表 A.3）与 `:5752`（表 204），OI `2002` 为 `电压相角`（Class 3，`long-unsigned`，单位度，换算 -1），`scripts/build_exhaustive_idls.py`、`scripts/build_clean_human_readable_idls.py`、`scripts/parse_full_pdf_schemas.py`、`scripts/parse_exhaustive_698_dlms_idls.py` 不得将 `0x20020200` 映射为零序电流或 C 相电压。 | I2 上述 4 个脚本中 `0x20020200` 统一映射为 `OAD_VOLTAGE_PHASE_ANGLE`（电压相角）。 | `pytest tests/test_dcu_scripts_oad.py` 断言 4 个脚本均不含 `0x20020200` 到零序电流/C 相电压的映射，且均正确映射为电压相角。 | AC2 `pytest tests/test_dcu_scripts_oad.py` 全部通过。 | N2 不重写 IDL 生成器其他无关 OAD，也不重新生成覆盖 `contracts/idl/`。 |
| R3 审计 `cmd/dcu/dcu_router.h:49` `OAD_REACTIVE_ENERGY`（`0x00200200`）：依据表 A.2（`docs/698_parsed.md:6316` vs `:6324`/`:6332`），`0020` 为 `反向有功电能`（`double-long-unsigned`，kWh，-2），组合无功 1/2 电能为 `0030`/`0040`（`double-long`，kvarh，-2）。 | I3 不修改 `OAD_REACTIVE_ENERGY` 宏名与数值 `0x00200200`，不破坏既有对象表、北向名 `"REACTIVE_ENERGY"` 及全部测试/ABI。 | `spec.md` 与 `docs/design/DCU_698_DEVIATIONS.md` 精确记录该命名偏差、引用范围与复查条件。 | AC3 `docs/design/DCU_698_DEVIATIONS.md` 新增对应偏差行，`check_doc_references.py` 通过。 | N3 本单不更改 `OAD_REACTIVE_ENERGY` 定义、`dcu_698_var.c` 表项及既有测试断言。 |
| R4 `tests/test_dcu_698_rules.c:88` 使用 `snprintf(b->path, sizeof(b->path), "/tmp/dcu_698_rules_XXXXXX")` 替代不安全的 `strcpy`。 | I4 `mkstemp(b->path)` 临时路径模板与测试行为完全一致。 | `ctest --test-dir build -R '^test_dcu_698_rules$'` 通过；`tests/test_dcu_scripts_oad.py` 断言 `tests/test_dcu_698_rules.c` 不含 `strcpy` 且使用 `snprintf`。 | AC4 `test_dcu_698_rules` 全绿且 pytest 守卫通过。 | N4 不重构 `test_dcu_698_rules.c` 其他无关用例。 |

## `OAD_REACTIVE_ENERGY` (`0x00200200`) 审计结论

- **标准依据**：`docs/698_parsed.md:6316`（表 A.2）定义 OI `0020` = `反向有功电能`（`double-long-unsigned`，单位 `kWh`，换算 `-2`）；组合无功 1/2 电能分别为 `:6324` `0030`（`double-long`，单位 `kvarh`，换算 `-2`）与 `:6332` `0040`（`double-long`，单位 `kvarh`，换算 `-2`）。
- **产品现状**：
  - `cmd/dcu/dcu_router.h:49`：`#define OAD_REACTIVE_ENERGY 0x00200200`
  - `cmd/dcu/dcu_698_var.c:41`：类型 `DCU_698_TAG_DOUBLE_LONG_UNSIGNED`、换算 `-2`、单位 `33`（kWh，符合 `0020` 物理定义），但北向名称字符串为 `"REACTIVE_ENERGY"`。
  - `cmd/dcu/dcu_698_object_model.c:149`、`cmd/dcu/dcu_698_rules.c:19,34,56`（其中 DLT645-2007 DI `0x00020000` 为反向有功总电能）、`cmd/dcu/dcu_698_record.c:27` 及 6 个 C 测试文件均依赖该宏名与值 `0x00200200`。
- **处置**：记录于 `docs/design/DCU_698_DEVIATIONS.md`，保留宏名与值不变以维持 ABI 与既有测试兼容；待后续产品点位表/北向命名统一迁移时另立单按标准拆分 `0020`（反向有功）与 `0030`/`0040`（组合无功 1/2）。
