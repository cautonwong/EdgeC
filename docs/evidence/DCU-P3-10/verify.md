# DCU-P3-10 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `b25ff9af` (verified on combined Batch 1 tip `56c3ca51`)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-P3-10/spec.md)

## Independent Expected Value Derivation (from Spec)

From `spec.md` §1 (Business Rules) and §2 (Invariants):
- Window capacity is `MAX_EVENT_RECORDS = 10` in reverse-chronological order (`records[0]` is newest).
- Record `k` appended (`k = 1, 2, ..., 12`) must receive `index = k`.
- After 12 appends, the oldest 2 records (`index = 1, 2`) are evicted, leaving `record_count = 10` with:
  - `records[0..9].index == [12, 11, 10, 9, 8, 7, 6, 5, 4, 3]`.
- In A-XDR `GET-Record` (`attr_id = 6`), each record's `double-long-unsigned` (`0x06`) field encodes a big-endian 4-byte unsigned integer, so the 10 encoded indices must be `0x0000000C` down to `0x00000003` with strict monotonicity `records[i].index > records[j].index` for all `0 <= i < j < 10`.
- Baseline bug (`new_rec->index = evt->record_count + 1`) saturates at `10 + 1 = 11` once the window is full, producing duplicate `11, 11, 10, ..., 3` (violating strict monotonicity).

## Merged-Tree Verification Evidence (`feat/dcu` @ `56c3ca51`)

1. **Clean Release Build & Warning Gate**:
   - `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_WEB_BUILD=OFF && cmake --build build -j$(nproc)`
   - `python3 ci/scripts/check_build_warnings.py --log /tmp/dcu-b1-build.log` → `0 warnings` (rc=0)
2. **Full Test Suite**:
   - `ctest --test-dir build --output-on-failure -j$(nproc)` → `380/380 passed` (rc=0)
3. **Mutation Guard (Red under mutation / Green after restore)**:
   - `python3 ci/scripts/run_mutation_guards.py --build build --only DCU-P3-10-event-record-index`
   - Mutated (`cmd/dcu/dcu_event_monitor.c`): `build rc=0, test rc=8` (RED)
   - Restored: `build rc=0, test rc=0` (GREEN)
4. **ASAN + LSAN + UBSAN**:
   - `ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build-asan -R "^test_dcu_advanced_modules$"` → `Passed` (0 leaks, 0 ASAN/UBSAN errors)
5. **Repo Guards & Doc References**:
   - `python3 ci/scripts/run_guard_selftest.py` → `PASS`
   - `python3 ci/scripts/check_doc_references.py --root .` → `PASS`
