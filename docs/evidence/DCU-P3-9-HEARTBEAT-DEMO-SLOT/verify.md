# DCU-P3-9-HEARTBEAT-DEMO-SLOT verification

Branch `dcu/P3-9-heartbeat-demo-slot`, base `99318550`, final tip `903cd640` (final code `8fcd469c`, test `64b027db`). The first attempt (`67f1f929`, address `10000001`) was rejected: the acceptance bench meter is `000000000001`, so the lookup skipped and no 645 trip frame was sent. This version targets the inventory's first meter.

## Scope

- `dcu_heartbeat_control_tick` resolves `inv->meters[0].slot_id` and logs the resolved address; an empty inventory or missing node logs a warning and skips. Production path unchanged.
- Tests remove slot 0 so the first meter is `000000000002` with `slot_id = 1`, then assert the control sink receives that address with `OI_REMOTE_CONTROL` and trip; a second test asserts an empty inventory makes no call.

## Independent red / green

- Red at `64b027db`: `CMOCKA_TEST_FILTER=test_demo_control_targets_first_meter_slot ctest ...` -> `[ERROR] 0 != 1` at `tests/test_dcu_runtime_health.c:361`. Raw log in `verifier/red.log`.
- Green at `903cd640`: same filtered test 1/1; `ctest -R 'dcu|dlt698'` 45/45 (138.08s under concurrent build load).

## Acceptance

Three consecutive independent rounds: `全部合格:2 个验收任务`; `dcu_northbound` 35 passed in 22.74 / 17.85 / 17.97s; `dcu_acceptance` exit 0 each round.

## Mutation guard

`DCU-P3-9-heartbeat-first-meter-slot`: mutated build rc=0, test rc=8; restored original bytes/hash, build rc=0, test rc=0; runner reports 1/1 sensitive. Raw log in `verifier/mutation-guard.log`.

## Notes

- Target identity is "the inventory's first meter", matching the demo freeze pipeline's archive wording. A product requirement to target a fixed meter would be a design change, out of this unit.
- The branch history keeps the rejected first attempt; the final tree contains only the first-meter behavior and its tests.
