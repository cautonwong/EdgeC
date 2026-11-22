# DCU-D1-18a — comment audit

`comment-sicko` reviewed the diff and ran the unit suites. No `HOLD`s.

## Deleted comments (8)

- `dcu_698_var.c` struct-field restatement.
- `dcu_698_var.h` false header lines ("scale not copied here", stale tag table
  reference) — rewritten to the real split and standard tag values.
- `dcu_698_var.h` read_value contract said it supports Class 1/3/4 — it now
  serves only Class 6; rewritten.
- `dcu_698_axdr.c` dispatch narration.
- `test_dcu_router.c` migration narration.
- Acceptance module docstring "walk the device model" and the "voltage is
  float32" clause — both false after ADR-0016; rewritten.

## Code findings fixed

- **Southbound DLT698 phase keys**: the map key is the wire OAD; phase rows still
  used 0x20000300/0x20010300/0x20040300-0500 (attributes 3/4/5). Keys are now
  attr-2 element OADs, tag ids stay `TAG_*`.
- **ACL vs object masks**: 0020 was missing attr4/5; variable masks lacked
  attr3/5. Both aligned.
- **`dcu_698_var_read` ignored `elem_idx`**: it now puts the element index into
  the OAD, with a slot-1 test.
- **Dead duplicate read** in `dcu_698_value_for_oad` removed.
- **Poll list duplicated the shape table**: `dcu_698_var_poll_tags()` derives it;
  `dcu_node.c` uses it (and includes the header, fixing the warning-gate hit).
- **Event-monitor tag literals** replaced with `TAG_*`/`OAD_*` macros.
- **Stale tag table** in `docs/design/DCU_698_OBJECT_MODEL.md` corrected
  (long 0x10, unsigned 0x11).

## Left open

- The ACL (`k_attrs_*`) and the object-model `attr_mask` still encode the same
  attribute truth in two places; they now agree, but generating one from the
  other is a follow-up.
- The protocol scaler in `dcu_driver_adapter.c` and the standard scaler in
  `k_vars` are still separate; a mismatch would change the northbound value
  silently. A test linking them is a follow-up.
- 200A exposes one element (total) because the product has no phase
  power-factor shadow; the array is legal but shorter than Table 134's
  total/A/B/C for a three-phase meter.
