# DCU-D6-01 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `dcu/D6-01-26class-catalog-matrix` (`ec30c4c0`, verified on combined Batch 4 tip)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-D6-01/spec.md)

## Independent Expected Value Derivation (from Spec & Catalog)

From `spec.md` and `cmd/dcu/dcu_698_object_model.h` / `cmd/dcu/dcu_698_appconn.c`:
- Object table count: `dcu_698_object_count() == 135` with 0 duplicate OIs and 100% `class_id` agreement against `dcu_698_catalog_expected.h` (`DCU_698_OI_CATALOG.md`).
- Bit 0 of `attr_mask` and `method_mask` must never be set; every set attribute/method bit across all 135 objects must lie within the standard class/OI definition.
- `dcu_698_appconn_object_list()` (`4400 A2`) must decode via `dlt698_decode_data` into `130` objects, every OI present in `dcu_698_object_model.c` (0 dangling ACL OIs), every advertised attribute/method valid for its `class_id`.
- Unregistered OIs return `EDGE_SLAVE_SRC_NOT_FOUND` (DAR 6); out-of-class attributes/methods return `EDGE_SLAVE_SRC_UNDEFINED` (DAR 4).

## Merged-Tree Verification Evidence

1. **Clean Release Build & Warning Gate**: `check_build_warnings.py --log /tmp/dcu-b4-build.log` → `0 warnings` (rc=0).
2. **Full Test Suite**: `ctest --test-dir build --output-on-failure -j$(nproc)` → `381/381 passed` (includes `test_dcu_698_catalog`, 10/10 cases).
3. **Repo Guards & Doc References**: `run_guard_selftest.py` PASS; `check_doc_references.py --root .` PASS.
