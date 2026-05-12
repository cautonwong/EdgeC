## Parent

Reference: /workspaces/edgeos/EdgeC/ROBOT_PRD.md

## What to build

Implement the `kinematics_solver_stage` within the StreamEngine. This stage must intercept desired XYZ coordinates (end-effector position) from the DataHub. It will utilize the math lexicon's 4x4 matrix capabilities to perform Inverse Kinematics (IK) calculations based on configured Denavit-Hartenberg (DH) parameters, outputting the required target angles for each joint.

## Acceptance criteria

- [ ] A new `kinematics_solver_stage` is created and registered.
- [ ] The stage successfully parses DH parameters from its configuration.
- [ ] The stage performs matrix multiplication to solve IK for a standard 6-axis or SCARA configuration.
- [ ] Output joint angles are correctly published as `PIPEMSG_TYPE_SCALAR` events.
- [ ] A standalone test verifies IK calculations against a known analytical solution with an error margin of < 0.001 degrees.

## Blocked by

None - can start immediately