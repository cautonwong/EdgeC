## Parent

Reference: /workspaces/edgeos/EdgeC/ROBOT_PRD.md

## What to build

Extend the existing CAN driver to fully support the CANopen CiA 402 drive profile (`canopen_cia402_driver`). This is required to integrate standard industrial servo drives. The driver must operate in both Master mode (to issue Controlword and Target Position updates to downstream drives) and Slave mode (to receive high-level trajectories from a Manufacturing Execution System).

## Acceptance criteria

- [ ] The CAN driver parses and encodes standard CiA 402 objects (0x6040 Controlword, 0x6041 Statusword, 0x607A Target Position).
- [ ] Slave mode is implemented, capable of receiving PDOs (Process Data Objects) and updating the DataHub.
- [ ] Master mode is implemented, capable of translating DataHub changes into outgoing PDOs for the servos.
- [ ] An integration test verifies bidirectional PDO exchange simulating a state machine transition (e.g., Switch On Disabled -> Ready to Switch On -> Switched On -> Operation Enabled).

## Blocked by

None - can start immediately