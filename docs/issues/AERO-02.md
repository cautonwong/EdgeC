## Parent

Reference: /workspaces/edgeos/EdgeC/AERO_EDGE_PRD.md

## What to build

Develop the `ahrs_quaternion_stage`. This involves extending the `edge_linalg` library to natively support 4D Quaternion operations (multiplication, normalization, conversion to/from rotation matrices). The StreamEngine stage will ingest raw gyroscope rates, compute the quaternion updates, and fuse them with low-rate star tracker orientation matrices using the existing Extended Kalman Filter (EKF) functions, outputting a Gimbal Lock-free 3D orientation.

## Acceptance criteria

- [ ] `edge_linalg` is extended with a robust suite of Quaternion math functions.
- [ ] `ahrs_quaternion_stage` is implemented and integrated into the stream topology.
- [ ] An AFK test verifies that sequential 90-degree rotations across different axes using Quaternions perfectly match the expected analytic end-state, proving the absence of Gimbal Lock.
- [ ] EKF integration successfully corrects the high-frequency quaternion drift using low-frequency absolute orientation updates.

## Blocked by

None - can start immediately