# DCU-D1-07 — comment audit

`comment-sicko` reviewed the diff and the new record module. No `HOLD`s.

## Deleted comments (5)

- UTC helper: claimed `timegm` has process state; only `mktime` does. Replaced
  with the portability reason (no standard C `timegm`).
- NULL-boundary branch comment: unreachable — the codec rejects tag 0x00 before
  the product sees it; branch removed with it.
- Header "4 个源 OAD" claim: unenforced assumption; comment now states only the
  255-row limit and the +1 truncation probe.
- Bench fixture narration and the test docstring claiming the heartbeat writes
  the rows: both false (rows are pre-seeded). Rewritten/removed.

## Code findings fixed

- **RCSD count 0** now means all columns (Table 31): time + each source OAD in
  the page. Added a unit case (time + 3 sources -> 21-byte row).
- **Page truncation** now queries 255*4+1 cells and rejects only when that
  buffer is full, so a legitimate 1020-cell page is not misread as truncated.
- **Integer range checks** in the column encoder: values that do not fit the
  wire type fail the row (whole-page DAR) instead of wrapping.
- **ROAD CSD** returns DAR 2 (unsupported) instead of "object not found".
- **`date_time` (0x19)** ranges accepted as the codec allows; ms ignored
  (storage resolution is seconds).
- Deleted the unused `dcu_698_record_attach`, switched column OADs to the
  `OAD_*` macros, and dropped the duplicate daily-freeze literal.
- Acceptance now uses a real Selector2 range with exact row/value assertions;
  the DAR path fails instead of `pytest.skip`, and the 12 s retry loop is gone.

## Follow-up fixes after handback

- `date_time` (0x19) was parsed with the date_time_s layout; Table 48 puts the
  weekday before the hour, so hour/minute/second were shifted. Fixed and
  covered by `test_record_selector1_date_time_layout` (weekday 7, hour 0).
- Exact page boundary: the query now asks for capacity+1 and rejects only when
  it exceeds 255x4 cells, so an exactly full page is served.

## Left open

- Column metadata (`k_cols`) and `_scale_to_raw` duplicate `dcu_698_var.c`;
  sharing needs a header change, and D1-18b is editing that file. Follow-up.
- The live `dcu_driver_adapter.c` scalers disagree with the standard for current
  and active power; the record path uses the standard scalers, so the real-time
  and record paths can differ for the same engineering value. Pre-existing.
- The bench repeats the `dcu_freeze_history` DDL; if the storage schema changes,
  the fixture must follow.
- One global record context: safe while the port is synchronous; would need
  per-session state if begin/row ever interleave.
