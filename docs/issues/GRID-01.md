## Parent

Reference: /workspaces/edgeos/EdgeC/GRID_CORE_PRD.md

## What to build

Implement the `dcu_dynamic_poller` to handle Data Concentrator Unit (DCU) scenarios. Unlike static setups, a DCU must poll a dynamic list of smart meters. This module will read a list of meter addresses (e.g., DLT645 or DLT698) from the `SQLite` persistence layer at startup or upon update. It must dynamically generate and schedule `edge_req_t` tasks to poll these meters concurrently over available serial/HPLC channels without blocking the main event loop.

## Acceptance criteria

- [ ] `dcu_dynamic_poller` can query the SQLite database for a list of active meter addresses.
- [ ] It dynamically instantiates polling tasks (`edge_req_t`) for the discovered meters.
- [ ] Polling is executed asynchronously via the `bus_orchestrator` to prevent head-of-line blocking.
- [ ] A simulation with 1,000 mock meter addresses successfully completes a full read cycle within a defined timeout period.

## Blocked by

None - can start immediately