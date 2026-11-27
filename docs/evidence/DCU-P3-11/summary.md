# DCU-P3-11 Summary: Fix `bus_orchestrator_destroy` Resource Leaks

## Root Cause
`bus_orchestrator_destroy` in `modules/edge_io/src/bus_orchestrator.c` previously only unref'd `ctx->slave_pool` and cleared `g_active_orch`. It never:
1. Detached and freed `ctx->engines[0..max_buses-1]` (created by `engine_create_master` / `engine_create_slave`, along with their `session_manager_t` and driver session contexts).
2. Stopped and closed `ctx->audit_timer` on `ctx->loop`.
3. Unref'd `ctx->pool` (the shared buffer pool allocated in `bus_orchestrator_create_in`).

Additionally, `_slave_detach` in `modules/edge_io/src/engines/slave_engine.c` dereferenced `priv->io->ops->enable_read(priv->io, false)` during engine detach, which causes a use-after-free when callers (e.g., `iems_north_iec104_close`) destroy the `io_handle_t` before calling `bus_orchestrator_destroy`.

## Changes
- `modules/edge_io/src/bus_orchestrator.c`:
  - `bus_orchestrator_destroy` now iterates `0..min(ctx->max_buses, ARBITER_MAX_BUSES)-1`, invokes `detach` on each non-NULL engine, and clears `engines[i]` and `io_handles[i]`.
  - Stops and closes `ctx->audit_timer` if `ctx->loop` is non-NULL, `audit_timer.type == UV_TIMER`, and not already closing.
  - Unrefs and nulls `ctx->pool` in addition to `ctx->slave_pool`.
  - Clears `g_active_orch` if equal to `ctx`.
- `modules/edge_io/src/engines/slave_engine.c`:
  - `_slave_detach` clears `priv->io = NULL` without dereferencing caller-owned `priv->io`, matching `_master_detach`.
- `tests/test_bus_orchestrator.c`:
  - Added `test_bus_orchestrator_destroy_releases_engines_and_pools` covering `bus_orchestrator_create_hub` (`create_in`) + `setup_master` + `setup_slave` + `run` + `destroy(NULL)` + idempotent `destroy(ctx)` + `uv_run` + `uv_loop_close == 0`.
- `ci/mutation_guards.tsv`:
  - Registered `DCU-P3-11-orch-destroy-teardown` to verify `test_bus_orchestrator_destroy_releases_engines_and_pools` fails when engine teardown is removed.

## Verification
- ASAN/LSAN (`detect_leaks=1`): `test_bus_orchestrator` passed 2/2 with 0 leaked bytes.
- ASAN `ctest` (`edge_io`, `dcu`, `iems`): 84/84 passed.
- Clean build + `check_build_warnings.py`: 0 new warnings.
- `run_guard_selftest.py`: 291/291 refutation tests passed, all live guards passed.
- `run_mutation_guards.py --only DCU-P3-11-orch-destroy-teardown`: red under mutation (`rc=8`), green after restore (`rc=0`).
- `/no-comments` (`comment-sicko`): 0 comments or workarounds in changed lines.
