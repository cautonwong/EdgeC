# DCU-D3-08b Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `4fe8beba` (verified on combined Batch 3 tip `797f03a6`)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-D3-08b/spec.md)

## Independent Expected Value Derivation (from Spec & Standard)

From `spec.md`, DL/T 698.45 Table A.4 & Table 142 (Class 7 `IC_EVENT`):
- `0x3009`, `0x300C`, `0x3011`, `0x3013` are Class 7 (`IC_EVENT`, attributes A1..A10, methods 1/2/4/5). `0x300B` is Class 24 (`IC_SUB_EVENT`) per `698_parsed.md:6940` and `DCU_698_OI_CATALOG.md:1095`, so it is excluded from this Class 7 unit and remains DAR 6.
- Object table count: `131 + 4 = 135`.
- Class 20 ACL (`4400 A2`) count: `126 + 4 = 130 <= 224` (`EDGE_SLAVE_ACL_MAX_OBJECTS`).
- Since `130 >= 0x80`, the A-XDR array length switches from 1 byte (`0x7E`) to 2-byte long form (`0x81 0x82`), adding `+1` header byte.
- Each Class 7 ACL entry has 10 read-only attributes and 0 methods: `5 + 4 + 10 * 6 + 2 = 71` bytes.
- Total byte delta: `1 + 4 * 71 = 285` bytes → `4400 A2` full frame length `6150 + 285 = 6435` bytes (`<= BUS_SLAVE_POOL_BUF_SIZE 10496`).
- Access semantics:
  - `GET` on A1..A10 of `0x3009`/`0x300C`/`0x3011`/`0x3013` passes ACL and reaches resolver with empty `attr_mask` → honest **DAR 5** (`DLT698_DAR_OBJECT_CLASS_MISMATCH`).
  - `SET` on any of them → **DAR 3** (`DLT698_DAR_DENIED`).
  - `dcu_698_call` methods 1/2/4/5 → DAR 5; methods 3/6 → DAR 4 (`EDGE_SLAVE_SRC_UNDEFINED`).

## Merged-Tree Verification Evidence (`feat/dcu` @ `797f03a6`)

1. **Build & Warning Gate**: `check_build_warnings.py --log /tmp/dcu-b3-build.log` → `0 warnings` (rc=0).
2. **Full Test Suite**: `ctest --test-dir build --output-on-failure -j$(nproc)` → `380/380 passed` (includes `test_dcu_698_catalog`, `test_dcu_698_object_model`, `test_dcu_698_appconn`, `test_dcu_698_axdr`, `test_dcu_698_scheme_mgmt`, `test_driver_dlt698`).
3. **Mutation Guard**: `run_mutation_guards.py --build build --only DCU-D3-08b-class7-event-skeleton`:
   - Mutated (`cmd/dcu/dcu_698_object_model.c`): `build rc=0, test rc=8` (RED).
   - Restored: `build rc=0, test rc=0` (GREEN).
4. **Repo Guards & Doc References**: `run_guard_selftest.py` PASS; `check_doc_references.py --root .` PASS.
