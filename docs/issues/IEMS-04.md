## Parent

Reference: /workspaces/edgeos/IEMS_PRD.md

## What to build

Complete an end-to-end integration test demonstrating a dispatch command traversing the full EdgeC stack. A mock IEC 104 Master sends a remote control command (C_SC_NA_1 or C_DC_NA_1), which is parsed by the IEC 104 slave driver and published to the DataHub. This state change triggers the MPC Dispatcher Stage to recalculate the optimal strategy. Finally, the resulting power target must be correctly encoded into a Modbus FC 16 (Write Multiple Registers) frame by the Modbus slave driver for the simulated PCS.

## Acceptance criteria

- [ ] A test orchestrator script/binary is created to mock both an IEC 104 master and a Modbus master.
- [ ] The IEC 104 slave correctly routes the control command through the DataHub to the `mpc_dispatcher_stage`.
- [ ] The `mpc_dispatcher_stage` outputs a new target value.
- [ ] The Modbus slave driver correctly receives the new shadow value and encodes an FC 16 frame upon request from the mock Modbus master.
- [ ] The entire data flow is verified to be non-blocking and memory-leak free.

## Blocked by

- [IEMS-03] 预测控制调度算子 (MPC Dispatcher Stage)