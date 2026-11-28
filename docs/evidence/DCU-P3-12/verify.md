# DCU-P3-12 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `56c3ca51` (combined Batch 1 tip)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-P3-12/spec.md)

## Independent Expected Value Derivation (from Spec)

From `spec.md` §1 (Business Rules) and DL/T 698.45 `SET-Response` APDU format (`0x86 0x01 piid 0x40 0x00 0x02 0x00 DAR 0x00`):
- Byte `res[7]` is the DAR for `SET 4000 A2` (`date_time_s`, tag `0x1C` + 7 calendar bytes):
  - Authorized + valid `date_time_s` + `clock->set == 0` → `res[7] == 0x00` (DAR 0), subsequent `GET 4000 A2` returns the new timestamp.
  - Authorized + valid `date_time_s` + missing clock or `clock->set != 0` → `res[7] == 0x02` (DAR 2, `EDGE_SLAVE_SRC_TEMPORARY`), subsequent `GET 4000 A2` returns the original timestamp (no RAM shadow fallback).
  - Authorized + malformed payload (wrong tag, truncated, trailing bytes, or out-of-range calendar field) → `res[7] == 0x07` (DAR 7, `EDGE_SLAVE_SRC_TYPE_MISMATCH`), clock unchanged.
  - Unauthorized session (ACL denies write to `4000 A2`) → `res[7] == 0x03` (DAR 3, `DLT698_DAR_DENIED`), clock unchanged.
- All four surfaces (`cmd/dcu/dcu_698_param.c`, unit tests `test_dcu_698_param.c` + `test_driver_dlt698.c`, acceptance `test_dcu_northbound.py::D_00_00_02_000022`, and `docs/design/DCU_698_OBJECT_MODEL.md` §8.1) must state this exact `0 / 2 / 7 / 3` contract.

## Merged-Tree Verification Evidence (`feat/dcu` @ `56c3ca51`)

1. **Clean Release Build & Warning Gate**:
   - `python3 ci/scripts/check_build_warnings.py --log /tmp/dcu-b1-build.log` → `0 warnings` (rc=0)
2. **Full Test Suite**:
   - `ctest --test-dir build --output-on-failure -j$(nproc)` → `380/380 passed` (rc=0)
3. **Mutation Guard (Red under mutation / Green after restore)**:
   - `python3 ci/scripts/run_mutation_guards.py --build build --only DCU-P3-12-4000-set-dar`
   - Mutated (`cmd/dcu/dcu_698_param.c`): `build rc=0, test rc=8` (RED)
   - Restored: `build rc=0, test rc=0` (GREEN)
4. **ASAN + LSAN + UBSAN**:
   - `ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build-asan -R "^(test_dcu_698_param|test_driver_dlt698)$"` → `2/2 Passed` (0 leaks, 0 ASAN/UBSAN errors)
5. **Repo Guards & Doc References**:
   - `python3 ci/scripts/run_guard_selftest.py` → `PASS`
   - `python3 ci/scripts/check_doc_references.py --root .` → `PASS`
