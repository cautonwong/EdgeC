# DCU-D1-18a — 变量类标准形状

- Base: main `fb0f16b9`
- Branch: `dcu/D1-18a-variable-shapes`
- Date: 2026-11-22

## ADR

`docs/adr/0016-variable-value-shapes.md` (Proposed) records:

- Class 1 attr2 = array of total (+ rates), double-long-unsigned [6]; attr4 =
  array long64-unsigned [21]; attr3/5 = Scaler_Unit.
- Class 3 attr2 = array A/B/C, long-unsigned [18] for voltage, double-long [5]
  for current (Table 131 instance-specific); attr3 = Scaler_Unit.
- Class 4 attr2 = array total/A/B/C, double-long [5]; attr3 = Scaler_Unit.
- Class 6 attr2 = single value (long [16] for power factor, long-unsigned [18]
  for frequency); attr3 = Scaler_Unit.
- OAD element index selects the table slot; the phase is the array element.
  `TAG_*` phase constants are internal shadow tags, not 698 OADs (renamed).
- `raw = round(value × 10^-scaler)`; no float32 on standard attributes.

## OAD / type table (wire)

| OAD | Shape | Element tag | scaler | unit |
| --- | --- | --- | --- | --- |
| 00100200 | array(1) | 0x06 double-long-unsigned | −2 | 33 kWh |
| 00100300 | Scaler_Unit | 0x02{0x0F,0x16} | −2 | 33 |
| 00100400 | array(1) | 0x15 long64-unsigned | −4 | 33 |
| 00100500 | Scaler_Unit | — | −4 | 33 |
| 00200200 / 03 / 04 / 05 | same as 0010 | | | |
| 20000200 | array(3) A/B/C | 0x12 long-unsigned | −1 | 38 V |
| 20000300 | Scaler_Unit | — | −1 | 38 |
| 20010200 | array(3) A/B/C | 0x05 double-long | −3 | 36 A |
| 20010300 | Scaler_Unit | — | −3 | 36 |
| 20040200 | array(4) total/A/B/C | 0x05 double-long | −1 | 27 W |
| 20040300 | Scaler_Unit | — | −1 | 27 |
| 200A0200 | array(1) total | 0x10 long | −3 | 0 |
| 200A0300 | Scaler_Unit | — | −3 | 0 |
| 200F0200 | single | 0x12 long-unsigned | −2 | 47 Hz |
| 200F0300 | Scaler_Unit | — | −2 | 47 |

## Implementation

- `dcu_698_var.c`: per-OI shape table (channel count, element tag, scaler, unit,
  shadow tag); `_scale_to_raw`; `dcu_698_var_encode_axdr` writes arrays and
  Scaler_Unit into the caller buffer (no dynamic memory); `dcu_698_var_poll_tags`
  derives the poll list from the same table.
- `dcu_north_resolve_axdr` dispatches variable OIs to the shape encoder;
  `dcu_698_value_for_oad` serves only Class 6 scalars and rejects arrays/structures
  with DAR 7.
- `dcu_698_var_read` now honours the element index (slot).
- Poll scheme (`dcu_node.c`) is derived from the shape table (14 shadow tags).
- ACL (`dcu_698_appconn.c`) and object masks allow attr3/5 for the variable OIs.
- Southbound DLT698 point keys for phases are attr-2 element OADs
  (`20000201/02`, `20010201/02`, `20040201..03`), while northbound OAD index
  remains the table slot.
- `dcu_698_var.h` tag constants corrected to the standard: long 0x10, unsigned
  0x11 (the old 0x11/0x13 mapping was wrong); design doc table updated.
- Acceptance reads arrays directly from APDU (platform point table cannot
  express them); the point table keeps only scalar counts.

## Audit

`comment-sicko` found the southbound phase keys, ACL/mask drift, `dcu_698_var_read`
ignoring the slot, a dead duplicate read, and the hardcoded poll list; all fixed.
Comment/doc text updated (8 comment deletions).

## Gates

Clean build 0 new warnings, 32/32 tests, ASAN 32/32, guards, acceptance 3 + 9,
`git diff --check`. See `tests.log`, `runtime.log`.
