## Parent

Reference: /workspaces/edgeos/EdgeC/EDGE_GUARD_PRD.md

## What to build

Implement the `dpi_validation_engine` within the Execution Plane. This module acts as a logical gatekeeper for all industrial protocol traffic. It must intercept incoming Slave requests (in `handle_rx`) and outgoing Master writes. By cross-referencing with the semantic contract, it must block any command that attempts to write values outside of the "Safety Range" (min/max) defined for that specific tag, effectively preventing range-based logic bomb attacks.

## Acceptance criteria

- [ ] A DPI interception hook is added to the generic driver executor.
- [ ] The engine correctly parses min/max constraints from the Semantic Contract's `meta_list`.
- [ ] Any write command violating these constraints returns a protocol-specific exception (e.g., Modbus Exception 03) and is blocked.
- [ ] A security event is published to the DataHub for every blocked attempt.
- [ ] A fuzzing test script demonstrates that 100% of out-of-range write attempts are successfully mitigated.

## Blocked by

None - can start immediately