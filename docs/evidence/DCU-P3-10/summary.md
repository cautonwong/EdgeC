# DCU-P3-10 Summary: Fix A6 Event Record Index Wraparound/Duplication

## Goal
Ensure every appended Class 24/Class 7 A6 event record receives a strictly monotonically increasing per-OI sequence number (`1, 2, 3, ...`) even after the 10-entry (`MAX_EVENT_RECORDS`) reverse-chronological window evicts older records, while skipped recoveries do not advance the sequence.

## Root Cause
In `cmd/dcu/dcu_event_monitor.c` (`_logger_proc`), `new_rec->index` was assigned `(uint16_t)(evt->record_count + 1)`. Because `evt->record_count` saturates at `MAX_EVENT_RECORDS` (10), the 11th and 12th appends both received `index = 11`, producing `[11, 11, 10, ..., 3]` in the window.

## Fix
- `cmd/dcu/dcu_event_monitor.c`: After shifting `evt->records[0..limit-1]` to `evt->records[1..limit]`, assign `new_rec->index = (evt->record_count > 0) ? (uint16_t)(evt->records[1].index + 1u) : 1u;`. Skipped recoveries return `STAGE_DROP` before shifting or assigning `index`, leaving the sequence unchanged.
- `tests/test_dcu_advanced_modules.c`: Added `test_event_record_index_monotonic_after_eviction` verifying 12 appends + skipped recovery yield `[12, 11, 10, 9, 8, 7, 6, 5, 4, 3]` in both `evt_3000.records[]` and A6 GET-Record A-XDR output (`dcu_north_resolve_axdr`).
- `ci/mutation_guards.tsv`: Registered `DCU-P3-10-event-record-index`.

## Verification
- **Red baseline**: `test_event_record_index_monotonic_after_eviction` failed at line 1036 (`11 != 12`) before the fix.
- **Clean build & warnings**: `cmake --build build --clean-first --target dcu_app_lib test_dcu_advanced_modules test_dcu_698_axdr` + `python3 ci/scripts/check_build_warnings.py --log /tmp/dcu-P3-10-build.log` -> `0` new violations.
- **Unit tests**: `ctest --test-dir build -R '^(test_dcu_advanced_modules|test_dcu_698_axdr)$' --output-on-failure` -> 2/2 passed (6/6 tests in `test_dcu_advanced_modules`).
- **ASAN**: `cmake -B /tmp/dcu-P3-10-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON` + `ctest --test-dir /tmp/dcu-P3-10-asan -R '^test_dcu_advanced_modules$'` -> 1/1 passed.
- **Guard selftest**: `python3 ci/scripts/run_guard_selftest.py` -> 291/291 refutation tests passed, all live guards passed.
- **Mutation guard**: `python3 ci/scripts/run_mutation_guards.py --build build --only DCU-P3-10-event-record-index` -> red on mutation (`rc=8`), green on restore (`rc=0`).
- **Comment review**: `/no-comments` via `comment-sicko` -> 2 narration comments in `tests/test_dcu_advanced_modules.c` deleted; 0 new comments remain.
