## Parent

Reference: /workspaces/edgeos/EdgeC/GRID_CORE_PRD.md

## What to build

Implement the `secure_grid_app_container` by enhancing the existing `stage_wasm.c`. Power grids require deploying third-party analytics apps (e.g., Topology Identification, Line-Loss Calculation) dynamically without reflashing the core firmware. This stage must load external WebAssembly binaries while enforcing strict execution constraints: memory must be bound to a specific `k_mem_slab` quota, and execution time must be limited to prevent starvation of critical control threads (like FTU fault isolation).

## Acceptance criteria

- [ ] `secure_grid_app_container` successfully loads and executes external `.wasm` binaries.
- [ ] Memory allocations initiated by the WASM app are strictly confined to a predefined quota.
- [ ] A watchdog mechanism interrupts the WASM execution if it exceeds a configured time limit.
- [ ] The WASM app can securely subscribe to DataHub events (e.g., voltage sags) and publish calculation results back to the DataHub.
- [ ] A test asserts that an infinite loop within the WASM module does not crash the core EdgeC operating system.

## Blocked by

None - can start immediately