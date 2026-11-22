# DCU-D1-07 — 5004 GetRecord storage

- Base: rebased onto D1-19 merge `7687612d` (originally `a9ddfbaf`); record module unchanged by the split
- Branch: `dcu/D1-07-record-storage`
- Date: 2026-11-22

## What landed

`cmd/dcu/dcu_698_record.{h,c}` implements the ADR-0014 record port:

- `begin(oad, selection, len, &rows)` decodes `OAD | RSD | RCSD` with
  `dlt698_decode_get_record_request`, maps the OAD element index to the meter
  slot, and queries `dcu_storage_query_freeze` into a fixed page (no dynamic
  memory). Rows are grouped by `freeze_time` so one record row can carry
  several source columns.
- `row(index)` writes the N columns in RCSD order; a missing cell or an
  unsupported column fails the row, which the driver turns into a whole-page
  DAR (ADR-0014 §3).
- `end` clears the page.
- Wired in `dcu_north_adapter.c`; `main.c` passes the node so the adapter can
  reach storage/inventory.

## Column mapping

| Column OAD | Wire type | Scale |
| --- | --- | --- |
| 20210200 | date_time_s (0x1C + 7) | — |
| 00100200 / 00200200 | double-long-unsigned (0x06) | −2 |
| 20000200 | long-unsigned (0x12) | −1 |
| 20010200 | double-long (0x05) | −3 |
| 20040200 | double-long (0x05) | −1 |
| 200A0200 | long (0x10) | −3 |
| 200F0200 | long-unsigned (0x12) | −2 |

RCSD count 0 means "all columns" (Table 31): the product answers with the
freeze time plus every source OAD present in the page.

## Time base

Storage `freeze_time` is Unix **seconds** UTC: the freeze pipeline writes
`ev->ts`, which the demo heartbeat sets from `time(NULL)`. RSD `date_time_s`
and `date_time` are converted with civil-days arithmetic (UTC, no `mktime`
process state). Selector2 is half-open, so the storage query uses `end - 1`;
selector 1 pins one second; selector 0 is unbounded.

## Limits / ambiguity

- More than 255 distinct freeze times, or more than 255x4 stored cells, returns
  DAR 2: there is no GetResponseNext in this driver (ADR-0014 §2). The query
  asks for capacity+1 so a page that exactly fills is still served.
- Non-NULL interval, RSD Selector3..10, and ROAD CSD columns return DAR 2
  (unsupported), not a wrong value.
- `date_time` (0x19) follows Table 48: month/day, weekday skipped, hour at byte 6;
  milliseconds are ignored because the storage resolution is seconds. A unit case
  uses weekday 7 with hour 0 to prove the old off-by-one parse is gone.
- The timezone is a convention choice: the standard's `date_time_s` has no
  zone field. UTC was chosen to match `time(NULL)`; a master using local time
  would shift range queries by the local offset. Recorded here explicitly.
- A freeze row stores one `source_oad`; columns with no matching source fail
  the whole result. The demo writer produces energy/voltage/current; asking
  for reactive power in the same RCSD fails the page rather than inventing a
  value.

## Acceptance

The bench heartbeat produces `saved=0` in the acceptance run, so it writes no
freeze rows. `dcu_product.py` provisions two 5004 rows for meter slot 0 before
the product starts. The acceptance case sends a real GetRecord with RSD
Selector2 `[2026-11-22, 2026-11-24)` and asserts exactly two rows, column order,
`date_time_s` dates, and scaled values 1234500 / 1234600.

## Gates

Clean build 0 new warnings, 33/33 tests, ASAN 33/33, guards, acceptance
3 + 10, `git diff --check`. See `tests.log`, `runtime.log`.
