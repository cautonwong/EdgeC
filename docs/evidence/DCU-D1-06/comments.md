# DCU-D1-06 — comment audit

`pstack:comment-sicko` reviewed the diff. No `HOLD`s.

## Deleted comments (3)

- `adapters/io_drivers/src/driver_dlt698.c` — DAR mapping restated below in
  `_dar_for_action_rc`; kept the "never fake success" line.
- `cmd/dcu/dcu_north_adapter.c` — `@brief` duplicated the public header contract; kept
  the A.9 / no-mapping paragraph.
- `acceptance/dcu/tests/test_dcu_northbound.py` — "参数解出来了" change narration removed
  from the 129 docstring.

## Falsified contract updated (1)

`foundation/include/edge_slave_plane.h` said non-zero ACTION results become DAR 2. They
now map through `EDGE_SLAVE_SRC_*` (7/3/6/2); the port text was updated so it does not lie.

## Code findings fixed

- `_dar_for_action_rc` duplicated `_dar_for_source_rc`; it now delegates and only overrides
  the generic failure to DAR 2.
- The 127…131 range was spelled in three places; `DCU_ACTION_METHOD_FIRST/LAST` now drive
  the mask, the handler, and the decoder.
- The decoder accepted 1…8 items but kept the last; it now keeps the first and records
  `count` so the array is not silently reshaped.
- `dcu_698_call(8000, 127/128, NULL)` reported TYPE_MISMATCH for a legal NULL param; the
  handler now routes 127/128 before the `!param` check.
- Adapter log claimed DAR 7 for every decode failure; it logs the actual `rc`.
- Object-model log printed a relay OAD for 127/128, which have none; message is branched.
- `DLT698_TAG_OI` had no consumer; dropped with its decoder case, keeping only OAD (81).
- Codec comment citation corrected to table 24 (tag) + 6.3.3.5 table 28 (layout).

## Docs synced

`docs/design/NORTHBOUND_ARCHITECTURE.md` rows for ACTION, E4b/E5b, the acceptance table,
and the E5b limitation paragraph; `cmd/dcu/dcu_north_source.c` downlink justification.
The plan's D-03 row still says the product returns DAR 2 for all OMDs; it is the
coordinator's file and is left for the merge record.

## Open

- Relay OAD → meter/DO mapping is the execution blocker; reported in `summary.md`.
- Legacy OI 8000 method 1 (slot int + action byte) remains while the demo heartbeat uses
  it; delete together with that demo path when it goes.
