## Parent

Reference: /workspaces/edgeos/EdgeC/GRID_CORE_PRD.md

## What to build

Develop the `automated_protocol_bridge` to seamlessly connect Southbound (e.g., DLT698/645) and Northbound (e.g., IEC104) protocols. This module will leverage the `meta_list` injected by the Semantic Compiler to create a zero-code routing table. When a DLT driver writes a value to the DataHub, this bridge must automatically identify the corresponding Northbound protocol (e.g., IEC104 IOA) and trigger the Spontaneous (`IEC_COT_SPONT`) upload logic without requiring custom C code for each deployment.

## Acceptance criteria

- [ ] `automated_protocol_bridge` utilizes the `meta_list` to form a bidirectional mapping.
- [ ] Southbound data updates in the DataHub automatically trigger Northbound spontaneous reports.
- [ ] Northbound control commands (e.g., IEC104 C_SC_NA_1) are automatically routed to the correct Southbound driver.
- [ ] An AFK test injects a mocked DLT698 response and verifies that an exact, corresponding IEC104 Spontaneous frame is generated on the simulated uplink.

## Blocked by

None - can start immediately