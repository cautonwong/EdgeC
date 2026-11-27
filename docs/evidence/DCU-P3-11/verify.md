# DCU-P3-11 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `53a7b96b` (verified on combined Batch 1 tip `56c3ca51`)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-P3-11/spec.md)

## Independent Expected Value Derivation (from Spec)

From `spec.md` §1 (Business Rules) and §2 (Invariants):
- `bus_orchestrator_create_in` + `setup_master` + `setup_slave` + `run` allocates master/slave engine objects, session managers, `net_buf_pool` instances (`pool`, `slave_pool`), and starts `audit_timer` on `loop`.
- Calling `bus_orchestrator_destroy(ctx)` must detach all active `engines[i]` (without dereferencing caller-owned `priv->io` in `_slave_detach`), stop and close `audit_timer`, unref `pool` and `slave_pool`, and clear `g_active_orch` if equal to `ctx`.
- Expected post-condition after `uv_run(loop, UV_RUN_DEFAULT)`:
  - `uv_loop_close(&loop) == 0` (no leaked/unclosed libuv handles such as `audit_timer` or engine timers).
  - Under ASAN/LSAN (`detect_leaks=1`), `0` leaked bytes and `0` heap-use-after-free errors even when caller-owned `io_handle_t` is destroyed before `bus_orchestrator_destroy`.
  - Under baseline (before fix), `bus_orchestrator_destroy` leaves `audit_timer`, `engines[]`, and `pool`/`slave_pool` open/allocated, causing `uv_loop_close(&loop) == UV_EBUSY` (non-zero) and LSAN leak failure.

## Merged-Tree Verification Evidence (`feat/dcu` @ `56c3ca51`)

1. **Clean Release Build & Warning Gate**:
   - `python3 ci/scripts/check_build_warnings.py --log /tmp/dcu-b1-build.log` → `0 warnings` (rc=0)
2. **Full Test Suite**:
   - `ctest --test-dir build --output-on-failure -j$(nproc)` → `380/380 passed` (rc=0)
3. **Mutation Guard (Red under mutation / Green after restore — verified in Release build without ASAN)**:
   - `python3 ci/scripts/run_mutation_guards.py --build build --only DCU-P3-11-orch-destroy-teardown`
   - Mutated (`modules/edge_io/src/bus_orchestrator.c`): `build rc=0, test rc=8` (RED)
   - Restored: `build rc=0, test rc=0` (GREEN)
4. **ASAN + LSAN + UBSAN**:
   - `ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build-asan -R "^test_bus_orchestrator$"` → `Passed` (0 leaks, 0 UAF, `uv_loop_close == 0`)
5. **Repo Guards & Doc References**:
   - `python3 ci/scripts/run_guard_selftest.py` → `PASS`
   - `python3 ci/scripts/check_doc_references.py --root .` → `PASS`
