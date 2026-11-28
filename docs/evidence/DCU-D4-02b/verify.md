# DCU-D4-02b Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `63bfc8a5` (verified on combined Batch 2 tip `461a0ee0`)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-D4-02b/spec.md)

## Independent Expected Value Derivation (from Spec)

From `spec.md` and `cmd/dcu/dcu_698_appconn.c` A-XDR encoding (`_put_object`):
- Each object entry in `4400 A2` occupies `5 (head) + 4 (attr array header) + 6 * n_attrs + 2 (method array header) + 6 * n_methods = 11 + 6 * (n_attrs + n_methods)` bytes.
- Adding the 8 read-only A.9 OIs (`0x8001` with 4 attrs, `0x8100..0x8103` with 1 attr each, `0x810E..0x8110` with 0 attrs, all with 0 methods) adds:
  - Object count: `118 + 8 = 126 <= 224` (`EDGE_SLAVE_ACL_MAX_OBJECTS`).
  - Byte delta: `(11 + 24) + 4 * (11 + 6) + 3 * 11 = 35 + 68 + 33 = 136` bytes.
  - Expected total `4400 A2` length: `6014 + 136 = 6150` bytes (`<= 10240`).
- Access rights:
  - `GET 8001 A2..A5` & `GET 8100..8103 A2`: pass ACL (rights=1) and reach resolver → honest `DAR 2` (`DLT698_DAR_TEMPORARY_FAILURE`).
  - `SET 8001 A2` / `8100 A2` / `8103 A2`: rejected by ACL → `DAR 3` (`DLT698_DAR_DENIED`).
  - `GET/SET 810E..8110 A2`: rejected by ACL (0 attrs advertised) → `DAR 3`.

## Merged-Tree Verification Evidence (`feat/dcu` @ `461a0ee0`)

1. **Build & Warning Gate**: `check_build_warnings.py --log /tmp/dcu-b2-build.log` → `0 warnings` (rc=0).
2. **Full Test Suite**: `ctest --test-dir build --output-on-failure -j$(nproc)` → `380/380 passed` (includes `test_dcu_698_appconn`, `test_dcu_698_object_model`, `test_driver_dlt698`, `test_dcu_698_scheme_mgmt`, `test_dcu_698_axdr`).
3. **Mutation Guard**: `python3 ci/scripts/run_mutation_guards.py --build build --only DCU-D4-02b-class13-acl`:
   - Mutated (`cmd/dcu/dcu_698_appconn.c`): `build rc=0, test rc=8` (RED).
   - Restored: `build rc=0, test rc=0` (GREEN).
4. **Repo Guards & Doc References**: `run_guard_selftest.py` PASS; `check_doc_references.py --root .` PASS.
