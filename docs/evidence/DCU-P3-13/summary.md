# DCU-P3-13 证据包摘要：OAD 误标清理、3006 注释订正、`OAD_REACTIVE_ENERGY` 审计与 `rules` 测试 `snprintf` 替换

- **分支**：`dcu/P3-13-oad-cleanup-and-rules-strcpy`（基于 `feat/dcu` `acef3050`）
- **Worktree**：`/tmp/dcu-P3-13`

## 改动清单

1. `cmd/dcu/dcu_soe_event_engine.h:24`：保留枚举标识符与值 `OAD_EVT_ZERO_SEQ_CURRENT = 0x3006` 不变以维持 ABI 兼容，将注释订正为 `3006: 电能表断流事件 (Meter Current-Loss Event, 698_parsed.md:6906)`。
2. `scripts/build_exhaustive_idls.py:31`、`scripts/build_clean_human_readable_idls.py:26`、`scripts/parse_full_pdf_schemas.py:49`、`scripts/parse_exhaustive_698_dlms_idls.py:76`：依据标准表 A.3（`docs/698_parsed.md:6652`）与表 204（`:5752`），将硬编码 `0x20020200` 从零序电流/C 相电压修正为 `OAD_VOLTAGE_PHASE_ANGLE`（电压相角）。
3. `docs/design/DCU_698_DEVIATIONS.md` & `docs/evidence/DCU-P3-13/spec.md`：完成 `OAD_REACTIVE_ENERGY`（`0x00200200`，`cmd/dcu/dcu_router.h:49`）与标准 A.2（`0020` = 反向有功电能 vs `0030`/`0040` = 组合无功 1/2 电能）的审计登记，保留宏名与数值不变以免破坏既有测试与 ABI。
4. `tests/test_dcu_698_rules.c:88`：将 `strcpy(b->path, "/tmp/dcu_698_rules_XXXXXX")` 替换为 `snprintf(b->path, sizeof(b->path), "/tmp/dcu_698_rules_XXXXXX")`。
5. `tests/test_dcu_scripts_oad.py`：新增确定性测试，校验 4 个脚本不再将 `0x20020200` 误标为零序电流或 C 相电压（均映射为 `OAD_VOLTAGE_PHASE_ANGLE` / 电压相角），且 `tests/test_dcu_698_rules.c` 使用 `snprintf` 无 `strcpy`。
6. `docs/evidence/DCU-P3-13/`：`spec.md`、`summary.md`、`tests.log`、`diffstat.txt`、`comments.md`。

## 验证结果

- Clean build (`cmake -S . -B build -G Ninja && cmake --build build --clean-first --target test_dcu_698_rules`) + `python3 ci/scripts/check_build_warnings.py`：0 新增告警。
- `ctest --test-dir build -R '^test_dcu_698_rules$' --output-on-failure`：1/1 通过。
- `/workspaces/edgeos/EdgeC/.venv/bin/pytest tests/test_dcu_scripts_oad.py`：2/2 通过。
- `python3 ci/scripts/run_guard_selftest.py`：291/291 夹具测试与全部 live 守卫通过。
- `python3 ci/scripts/check_doc_references.py --root .`：通过。
