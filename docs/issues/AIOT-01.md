## Parent

Reference: /workspaces/edgeos/EdgeC/AIOT_FUSION_PRD.md

## What to build

Implement the `sensor_fusion_ekf_stage` within the StreamEngine. This stage must subscribe to high-frequency IMU telemetry (e.g., from a CAN driver) and low-frequency GPS coordinates (e.g., from a Serial driver). It will wrap the `control_ekf_predict` and `control_ekf_correct` functions to perform real-time sensor fusion, outputting a highly accurate and continuous positional estimate even if GPS updates are dropped.

## Acceptance criteria

- [ ] `sensor_fusion_ekf_stage` is created and registered in the factory.
- [ ] The stage correctly maps high-frequency events to the EKF prediction step.
- [ ] The stage correctly maps low-frequency events to the EKF correction step.
- [ ] Output coordinates are published back to the DataHub.
- [ ] A simulated test confirms that the estimated trajectory remains smooth and bounded when GPS updates are artificially halted for 10 seconds.

## Blocked by

None - can start immediately