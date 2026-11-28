# DCU-D4-11 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `dcu/D4-11-p4-matrix-tests` (`215adad2`, verified on combined Batch 4 tip)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-D4-11/spec.md)

## Independent Expected Value Derivation (from Spec & Standard)

From `spec.md` and `docs/design/DCU_698_OBJECT_MODEL.md`:
- Every P4 class (`13, 18, 19, 20, 21, 22, 23, 25, 26`) must have at least 4 independent assertions covering advertised GET, unauthorized/read-only SET (`DAR 3`), ACTION methods (or `DAR 2/5` honest unbacked status), and undefined attribute/method (`DAR 4`).
- For Class 20 (`0x4400 A2`), with `OBJECT_COUNT = 130 >= 0x80`, the A-XDR array header must encode long-form length `0x01 0x81 0x82` and total `GET-Response` frame length `6435` bytes (`<= BUS_SLAVE_POOL_BUF_SIZE 10496`).
- For Class 26 (`0x4510 A5`), when MAC is installed, `resolve_axdr` returns a valid 6-byte `octet-string`, whereas `dcu_north_resolve_scalar` returns `DAR 5` (no scalar representation for 6-byte MAC).

## Merged-Tree Verification Evidence

1. **Clean Release Build & Warning Gate**: `check_build_warnings.py --log /tmp/dcu-b4-build.log` → `0 warnings` (rc=0).
2. **Focused & Full Test Suite**: `ctest --test-dir build --output-on-failure -j$(nproc)` → `381/381 passed` (includes `test_dcu_698_p4_matrix`, 9/9 test groups).
3. **Repo Guards & Doc References**: `run_guard_selftest.py` PASS; `check_doc_references.py --root .` PASS.
