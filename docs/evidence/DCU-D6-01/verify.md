# DCU-D6-01 Verification Record

Spec: `docs/evidence/DCU-D6-01/spec.md`

## Checks Executed

1. Clean Ninja Release build (`-DCMAKE_BUILD_TYPE=Release -DENABLE_WEB_BUILD=OFF`, target `test_dcu_698_catalog`):
   - `python3 ci/scripts/check_build_warnings.py --log build/build_test_dcu_698_catalog.log --root .`
   - Result: `✅ [BUILD_WARNING_GUARD] No new violations. 0 tracked debt item(s) remain pinned.`
2. `ctest --test-dir build -R '^test_dcu_698_catalog$' --output-on-failure`
   - Result: `100% tests passed, 0 tests failed out of 1` (`10 test(s) run`, `10 test(s)` passed).
   - Covered matrix cases:
     - `test_every_table_oi_is_in_the_catalog` (`dcu_698_object_count() == 135`)
     - `test_masks_stay_within_the_class_definition`
     - `test_methods_stay_within_the_oi_definition`
     - `test_no_duplicate_ois`
     - `test_class_attr_limit_matches_catalog`
     - `test_class_method_limit_matches_catalog`
     - `test_appconn_acl_matches_object_model_and_catalog` (`4400 A2` decoded via A-XDR, 130 objects, 0 dangling OIs)
     - `test_negative_unregistered_oi_returns_not_found` (`EDGE_SLAVE_SRC_NOT_FOUND`, DAR 6)
     - `test_negative_out_of_class_attr_returns_undefined` (`EDGE_SLAVE_SRC_UNDEFINED`, DAR 4)
     - `test_negative_out_of_class_method_returns_undefined` (`EDGE_SLAVE_SRC_UNDEFINED`, DAR 4)
3. `python3 ci/scripts/check_doc_references.py --root .`
   - Result: exit 0 (after final doc edit, re-verified before commit).

## Base Observations (pre-existing in `5815f344`, outside owned files)

- `cmd/dcu/dcu_698_object_model.c`: `k_oi_max_attr` omits `0x4000` (A3/A4) and `0x8001` (A3..A5) listed in `dcu698_catalog_oi_attrs`.
- `cmd/dcu/dcu_698_var.c`: `dcu_698_value_for_oad` returns `EDGE_SLAVE_SRC_NOT_FOUND` (-20) instead of `EDGE_SLAVE_SRC_UNDEFINED` (-23) for out-of-class attributes on variable OIs (`0x0010`, `0x2000`) via `dcu_north_resolve_axdr`.
