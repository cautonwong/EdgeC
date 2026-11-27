# DCU-P3-7-DATE-TIME-TAG verification

Branch `dcu/P3-7-date-time-tag`, base `99318550`, final tip `a7c48c84` (code commits `6ba26a09`, `da7fa138`, `f2845962`; evidence commits after).

## Scope

- Add `DLT698_TAG_DATE_TIME = 25` to `foundation/include/dlt698_codec.h`.
- Replace date_time / date_time_s tag literals with `DLT698_TAG_DATE_TIME` / `DLT698_TAG_DATE_TIME_S` in `foundation/src/protocol/dlt698_codec.c`, `cmd/dcu/dcu_698_axdr.c`, `cmd/dcu/dcu_698_class_event.c`, and update the `dcu_698_axdr.h` comments.
- Behavior-neutral cleanup; no DAR, object-model, or wire-format change.

## Equivalence

`build/foundation/CMakeFiles/edge_codec.dir/src/protocol/dlt698_codec.c.o` sha256 `eaa6b7c4bd1ba9c0f1119b02d0271306d24dfe531ba4938cf9c50dda39894dfc` at base `99318550` and at tip `a7c48c84`. Byte-identical.

## Tests

- `ctest -R 'dlt698'` 7/7 (codec tip `6ba26a09`; codec files unchanged after).
- `ctest -R 'dcu|dlt698'` 45/45 at final tip (50.64s).
- Acceptance three consecutive rounds: `全部合格:2 个验收任务`; `dcu_northbound` 35 passed in 22.66 / 22.69 / 22.58s.

## Literal audit

`rg -n '0x19|0x1C' cmd/dcu -g '*.c' -g '*.h'` -> no hits. The remaining codec hit is the FCS16 table value `0x19c9`, not a tag. Raw output in `verifier/rg.log`.

## Review

Pure literal replacement; no adversarial review per handover §1.2 (pure cleanup path).
