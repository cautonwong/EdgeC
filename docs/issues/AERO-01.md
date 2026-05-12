## Parent

Reference: /workspaces/edgeos/EdgeC/AERO_EDGE_PRD.md

## What to build

Implement the `zephyr_hal_integration` module. This requires adapting the EdgeC foundational layers (DataHub, StreamEngine) to operate purely within the Zephyr RTOS ecosystem. It involves stripping out `libuv` event loops in favor of native Zephyr threads and Kconfig primitives, and ensuring all memory allocations are strictly deterministic and bound to pre-allocated static pools at boot time (zero dynamic allocation during flight).

## Acceptance criteria

- [ ] All `libuv` dependencies are successfully conditionally excluded for the Zephyr build target.
- [ ] EdgeC threading utilizes native `k_thread_create` when compiled for Zephyr.
- [ ] Memory allocation in DataHub and StreamEngine is verified to use bounded, static memory pools (`k_mem_slab` or static arrays) instead of `malloc/calloc`.
- [ ] A Zephyr RTOS target can successfully boot, instantiate a basic pipeline, and execute a mathematically pure control loop without throwing memory faults.

## Blocked by

None - can start immediately