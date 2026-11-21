# DCU-D1-05 — comment audit

`pstack:comment-sicko` reviewed the working-tree diff (11 scoped files). No `HOLD`s.

## Deleted (2)

- `tests/test_dcu_698_axdr.c:128` — `/* 5004 属性 2 现在是标量计数…… */`; restated the
  assertion two lines below.
- `tests/test_dcu_698_value_table.c` file header — ticket-id narration; the test names and
  assertions carry the intent.

## Kept (all accepted)

- `dcu_698_var.c` — the TYPE_MISMATCH-vs-NOT_FOUND rule for attributes without a scalar
  representation.
- `dcu_698_var.h` — exported-function contract, including the complex-attribute error.
- `dcu_north_adapter.h` — why the scalar callback is non-static (tests call it directly).
- `test_dcu_northbound.py` docstrings for 2001/2004, 6012, 5004 — A-XDR tag rules and the
  DAR-failure vs fake-0 trap.

## Code findings fixed (not comments)

- **Tag→width had a second source.** `_width_for_tag` in the adapter duplicated the type
  table and advertised widths for 0x03/0x13 that `_encode_axdr_value` could not encode, so
  `resolve` and `resolve_axdr` disagreed for event attrs 8/9. Moved width into
  `dcu_value_t`/`dcu_698_tag_width()` in `dcu_698_var.{h,c}`, added 0x03/0x13 to the
  encoder, and added a test that both callbacks agree.
- **64-bit raw truncation.** The port's `raw` is `int32_t`; the adapter silently truncated
  `long64-unsigned`. It now returns `TYPE_MISMATCH` for widths above 4 and leaves 64-bit
  values to `resolve_axdr`; covered by `test_scalar_rejects_64bit_raw`.

Skipped: `ci/baseline/composition_root_budget.txt` is outside the scoped list; it was
handled separately (adapter shrank 203 → 190).
