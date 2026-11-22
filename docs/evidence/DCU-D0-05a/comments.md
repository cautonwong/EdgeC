# DCU-D0-05a comment review

Tool: `pstack:comment-sicko` (no-comments), model opus, scope `main...30200b3c`.

## Result

- Files edited: `foundation/src/protocol/dlt698_codec.c`, `tests/test_dlt698_conformance_suite.c`. Comment-only.
- Deleted 4 comments: change narration about the encoder/decoder sharing a formula, the HCS range sentence duplicated by `hcs_at = 6 + sa_len`, the APDU-start narration, and the test step-by-step recompute narration.
- Kept 4 comments: the 5.1.5 HCS range decomposition (`5 + sa_len`), why HCS is checked before FCS, why the test must recompute FCS after corrupting HCS, and the frame-layout fact that index 7 is the HCS low byte.
- The reviewer correctly deleted the "shared formula" claim: the encoder uses a pointer difference and the decoder uses `_hcs_cover_len`; they are equivalent, not one shared expression.
- `HOLD`: none.

## Verification

- `git diff --check` exit 0.
- `cmake --build build -j$(nproc)` exit 0.
- `ctest -R dlt698` 6/6.
- Conformance suite 21 cases, 20 passed, 1 skipped (GET-Record/RSD -> D1-15).
- `run_guard_selftest.py --root .` all guards and live checks passed.
