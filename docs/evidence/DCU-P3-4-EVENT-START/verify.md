# DCU-P3-4-EVENT-START verification

Branch `dcu/P3-4-event-start-time`, base `99318550`. Scope: event recovery records must take `start_time` from the same slot's occurrence record instead of the per-OI `evt->last_start_time`.

## Independent red / green

- Red at `4a7e94e5` (test-only commit): `cmake --build build -j2 --target test_dcu_advanced_modules` then `ctest --test-dir build -R '^test_dcu_advanced_modules$' --output-on-failure` -> 1 failed, `[ERROR] 2001000000000 != 1001000000000` at `tests/test_dcu_advanced_modules.c:757` (`test_event_recovery_start_time_is_per_slot`).
- Green at `ff700dd2`: same test 1/1 pass; `ctest --test-dir build -R 'dcu|dlt698'` 45/45 pass (45.17s). After the guard follow-up, the final tip `3299111d` passes `ctest -R 'dcu|dlt698'` 45/45 (116.88s under concurrent build load).

## Acceptance (serial, quiet window)

Three consecutive rounds, each `全部合格:2 个验收任务`; `dcu_northbound` 35 passed in 22.93 / 22.92 / 22.75s; `dcu_acceptance` exit 0 each round.

## Mutation guard

`DCU-P3-4-event-start-slot`: mutated build rc=0, test rc=8; restored original bytes/hash, build rc=0, test rc=0; runner 1/1 sensitive. Raw log in `verifier/mutation-guard.log`.

`DCU-P3-4-event-archive-start` (added after the adversarial review): same red rc=8 / green rc=0 outcome; runner 1/1 sensitive. Raw log in `verifier/mutation-archive.log`.

## Review notes

- Recovery searches `records[0..record_count)` for the newest `is_active` record with the same `slot_id` and uses its `start_time`. If the occurrence record has been evicted from the 10-record window, the recovery record is not appended; only `last_end_time` is updated. This is an explicit product-visible choice (no fabricated start time), documented in the code and the evidence summary.
- `dcu_event_record_t` gains `uint32_t slot_id`; it lands in existing padding (`slot_id` offset 4, `start_time` offset 8), so `sizeof(dcu_event_record_t)` stays 568 at base and tip and the RAM delta is 0. Independently measured; an earlier verifier note of +4 bytes per record was wrong.
- A14 `last_start_time` / `last_end_time` stay per event OI, unchanged.

## Adversarial review

Two independent reviewers attacked the event-lifecycle change. No functional findings against the intent. Both flagged the RAM evidence number (fixed above). One found that the `archive_event` start argument and the eviction path had no executable guard, proved by reverting the archive start and observing a green suite. That gap is now covered by `test_event_archive_start_is_per_slot` (red `1700000001 != 1700000000` at `tests/test_dcu_advanced_modules.c:921` on the reverted line) and `test_event_recovery_evicted_occurrence_does_not_archive`, plus the `DCU-P3-4-event-archive-start` mutation guard.

Dropped by the reviewers: A6 record index reuse after wrap-around (`new_rec->index = record_count + 1`) is real and pre-existing, out of this unit's scope; logged as a follow-up. The duplicate-recovery pairing requires injecting a second recovery message, which the production composition root cannot do.
