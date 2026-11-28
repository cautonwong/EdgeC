# DCU-P3-10 Five-Column Spec: A6 Event Record Index Monotonicity

## 1. 业务规则 (Business Rules)

- Every appended event record (occurrence or recovery) in `dcu_event_obj_t.records[]` for a given OI (`3000` / `3005`) receives a strictly monotonically increasing per-OI event sequence number (`1, 2, 3, ...`), even after the 10-entry (`MAX_EVENT_RECORDS`) reverse-chronological window evicts older records.
- Specifically, when `record_count == 0`, the first appended record gets `index = 1`; when `record_count > 0`, the newly appended record gets `index = records[0].index + 1` (since `records[0]` holds the most recently appended record).
- Skipped recoveries (when the matching active occurrence for `slot_id` was already evicted from the 10-entry window) only update `last_end_time` (A14) and return `STAGE_DROP` without appending a record or advancing the sequence number.

## 2. 不变量 (Invariants)

- For any `0 <= i < j < record_count` in `evt->records`: `evt->records[i].index > evt->records[j].index` (strictly decreasing in the reverse-chronological window; no duplicates, no wraparound at `MAX_EVENT_RECORDS`).
- `evt->record_count <= MAX_EVENT_RECORDS` (10).

## 3. 可观测输出 (Observable Outputs)

- `dcu_event_record_t.index` on each entry of `evt->records[0 .. record_count - 1]`.
- A6 GET-Record (`dcu_698_event_encode_axdr` with `attr_id == 6`) A-XDR encoded `double-long-unsigned` (`0x06`) event index field for each record in the response array.

## 4. 验收判据 (Acceptance Criteria)

1. Appending 12 records (occurrences and/or recoveries) to `evt_3000` yields `record_count == 10` and `records[0..9].index == [12, 11, 10, 9, 8, 7, 6, 5, 4, 3]` (strictly decreasing, unique).
2. A6 GET-Record (`attr_id == 6`) A-XDR encoding emits the exact `double-long-unsigned` indices `[12, 11, 10, 9, 8, 7, 6, 5, 4, 3]` in order.
3. A skipped recovery (occurrence evicted from the 10-entry window) leaves `record_count` and `records[0].index` unchanged, and a subsequent occurrence receives `records[0].index + 1` (no gap caused by the skipped recovery).
4. Mutation guard in `ci/mutation_guards.tsv` turns red when `new_rec->index` reverts to `(uint16_t)(evt->record_count + 1)` and green when restored.

## 5. 非目标 (Non-Goals)

- Do NOT change `MAX_EVENT_RECORDS` (10).
- Do NOT change A14 (`last_start_time` / `last_end_time`) semantics.
- Do NOT change SQLite schema or `modules/edge_core` / protected foundation headers.
