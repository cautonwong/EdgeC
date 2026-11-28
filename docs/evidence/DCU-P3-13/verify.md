# DCU-P3-13 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `22fdfffc` (verified on combined Batch 2 tip `461a0ee0`)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-P3-13/spec.md)

## Independent Expected Value Derivation (from Spec & Standard)

From `spec.md` R1–R4 and DL/T 698.45 (`docs/698_parsed.md`):
- OI `3006` (table A.8, `698_parsed.md:6906`) is `电能表断流事件`, not zero-sequence current; enum value `0x3006` and identifier `OAD_EVT_ZERO_SEQ_CURRENT` must remain ABI-unchanged while comment reflects `电能表断流事件`.
- OI `2002` (table A.3 / table 204, `698_parsed.md:6652`) is `电压相角`; all 4 IDL/schema scripts must map `0x20020200` to `OAD_VOLTAGE_PHASE_ANGLE` and contain 0 mappings of `0x20020200` to zero-sequence current or C-phase voltage.
- OI `0020` (table A.2, `698_parsed.md:6316`) is `反向有功电能` (kWh, -2), whereas combined reactive energy 1/2 is `0030`/`0040` (kvarh, -2); `OAD_REACTIVE_ENERGY` (`0x00200200`) must be preserved for ABI compatibility and explicitly recorded in `docs/design/DCU_698_DEVIATIONS.md`.
- `tests/test_dcu_698_rules.c:88` must use `snprintf(b->path, sizeof(b->path), ...)` instead of unbounded `strcpy`, with `test_dcu_698_rules` continuing to pass.

## Merged-Tree Verification Evidence (`feat/dcu` @ `461a0ee0`)

1. **Build & Warning Gate**: `python3 ci/scripts/check_build_warnings.py --log /tmp/dcu-b2-build.log` → `0 warnings` (rc=0).
2. **Deterministic Script & `snprintf` Guard**: `/workspaces/edgeos/EdgeC/.venv/bin/pytest -q tests/test_dcu_scripts_oad.py` → `2 passed in 2.40s`.
3. **Full Test Suite**: `ctest --test-dir build --output-on-failure -j$(nproc)` → `380/380 passed` (includes `test_dcu_698_rules`).
4. **Repo Guards & Doc References**: `run_guard_selftest.py` PASS; `check_doc_references.py --root .` PASS.
