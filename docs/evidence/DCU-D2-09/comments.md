# DCU-D2-09 /no-comments

Reviewer: `pstack:comment-sicko` (opus), scope `a3f8d40f..HEAD` plus worktree changes, 2026-11-22.

## Applied

17 of 18 comment findings applied (26 lines):

- `cmd/dcu/dcu_discovery.h`: removed enum member restatements (`// 空闲`, `// 搜表中`), the mutex restatement, and the `init`/`snapshot`/`destroy` restating doc blocks. Corrected `dcu_discovery_clear` to name method 128 only (129 clears the cross-area set, which this function does not touch). Corrected the enum contract from "结果集保留" to "计数保留", because no result records are stored.
- `cmd/dcu/dcu_discovery.c`: removed the RAM-only comment; the struct doc already carries the limitation.
- `cmd/dcu/dcu_node.c`: removed the duplicate RAM-only comment.
- `cmd/dcu/dcu_698_axdr.c`: removed the "check_call already returned 5/4" narration.
- `cmd/dcu/dcu_698_object_model.c`: removed the false sentence "只有对象表里出现的 OI 需要登记"; kept the Appendix A vs 7.3.11 note.
- `tests/test_dcu_698_axdr.c`: removed the "搜表完成" narration, corrected "A5..A9" to "A5/A7/A8/A9", removed the duplicate-start narration.
- `tests/test_dcu_698_object_model.c`: removed two assertion restatements.
- `tests/test_dcu_advanced_modules.c`: removed the A2 scalar assertion restatement.
- `docs/design/DCU_698_OAD_STATUS.md`: heading changed to "部分可达（D2-09 更新）".

## Code fixes accepted from the workaround flags

- `cmd/dcu/dcu_698_axdr.c`: method 128/129 now checks `axdr_len == 0` before reading `axdr[0]`. Tests cover both NULL/0 and non-NULL/0 payloads.
- `cmd/dcu/dcu_698_class_acq.c`: 6002 encode now reports an undersized output buffer as `EDGE_SLAVE_SRC_TEMPORARY` (DAR 2), matching the task/archive encoders, not DAR 7. Tests cover `cap=1` for A2 and A3.

## Rejected

- `k_oi_max_attr` / `dcu_698_oi_defines_attr` (reviewer MUST KILL: hand-copied from the catalog, max-only, missing 0x2001). Kept. This helper classifies DAR 4 vs 5 for unadvertised attributes on object-table OIs. Variable OIs such as 0x2001 are handled by `dcu_698_var_*`, and parameter OIs by `dcu_698_param_*`, before this classifier. 6002 is the only object-table OI whose Appendix A attributes exceed its class table, and its A5..A10 have no gaps, so `max_attr = 10` is exact. Generating a product-side table from `docs/design/DCU_698_OI_CATALOG.json` is a cross-cutting build change; recorded open instead.
- `tests/test_dcu_advanced_modules.c:234` numbered section banner. Kept because sections 1-5 in that test all use the same `// ===` banner; removing one leaves a numbering gap. The inline A.7 restatement in the same section was removed.

## Open

- The catalog parser's `6298..9450` source window is duplicated in `parse_ois`, `parse_oi_methods`, and the new `parse_oi_attributes`. Pre-existing; anchoring on section headings is a separate generator refactor.
- `tests/test_dcu_698_catalog.c` checks advertised attributes against `max(OI Appendix A max, class max)`. For 6002 every id 1..10 is standard-defined, so the bound is exact today. A set-based check (attribute is in the class table or the OI's Appendix A set) would be stronger.
