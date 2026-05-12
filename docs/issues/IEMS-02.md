## Parent

Reference: /workspaces/edgeos/IEMS_PRD.md

## What to build

Create the `ekf_soc_estimator_stage` within the StreamEngine. This stage subscribes to battery cell voltage and current data from the DataHub. It will utilize the math lexicon's `control_ekf_predict` and `control_ekf_correct` functions to maintain an Equivalent Circuit Model (ECM) of the battery, updating its state covariance matrix in real-time to output an accurate State of Charge (SOC).

## Acceptance criteria

- [ ] The `ekf_soc_estimator_stage` is implemented and can be instantiated via the stage factory.
- [ ] The stage correctly maps input streams to the EKF prediction and correction matrices.
- [ ] The stage outputs the filtered SOC value back to the DataHub.
- [ ] A test using `stage_transfer_function` simulating an RC-circuit is created to verify closed-loop EKF convergence without physical hardware.

## Blocked by

None - can start immediately